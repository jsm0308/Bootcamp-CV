#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace rgbd {

struct CameraIntrinsics {
  double fx = 517.3;
  double fy = 516.5;
  double cx = 318.6;
  double cy = 255.3;
};

struct TimedPath {
  double timestamp = 0.0;
  std::string relative_path;
};

struct AssociatedPair {
  TimedPath rgb;
  TimedPath depth;
  double delta_seconds = 0.0;
};

struct PointXYZRGB {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  unsigned char r = 255;
  unsigned char g = 255;
  unsigned char b = 255;
  int u = 0;
  int v = 0;
};

struct CloudStats {
  int width = 0;
  int height = 0;
  int total_pixels = 0;
  int valid_depth_pixels = 0;
  int invalid_depth_pixels = 0;
  int sampled_points = 0;
  int sample_step = 1;
  double valid_depth_ratio = 0.0;
  float z_min = 0.0f;
  float z_max = 0.0f;
  double z_mean = 0.0;
};

struct CloudResult {
  std::vector<PointXYZRGB> points;
  CloudStats stats;
};

void ValidateIntrinsics(const CameraIntrinsics& k) {
  if (!std::isfinite(k.fx) || !std::isfinite(k.fy) || !std::isfinite(k.cx) ||
      !std::isfinite(k.cy)) {
    throw std::invalid_argument("Camera intrinsics must be finite");
  }
  if (k.fx <= 0.0 || k.fy <= 0.0) {
    throw std::invalid_argument("Camera focal lengths fx and fy must be positive");
  }
}

bool IsValidDepth(float z) { return std::isfinite(z) && z > 0.0f; }

cv::Point3f PixelToCamera(int u, int v, float z, const CameraIntrinsics& k) {
  ValidateIntrinsics(k);
  if (!IsValidDepth(z)) {
    throw std::invalid_argument("Depth must be positive and finite");
  }
  return cv::Point3f(static_cast<float>((u - k.cx) * z / k.fx),
                     static_cast<float>((v - k.cy) * z / k.fy), z);
}

cv::Point2f CameraToPixel(const cv::Point3f& point, const CameraIntrinsics& k) {
  ValidateIntrinsics(k);
  if (!IsValidDepth(point.z)) {
    throw std::invalid_argument("Point z must be positive and finite");
  }
  return cv::Point2f(static_cast<float>(k.fx * point.x / point.z + k.cx),
                     static_cast<float>(k.fy * point.y / point.z + k.cy));
}

std::vector<TimedPath> LoadTumList(const std::filesystem::path& file_path) {
  std::ifstream input(file_path);
  if (!input) {
    throw std::runtime_error("Failed to open TUM list: " + file_path.string());
  }

  std::vector<TimedPath> rows;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream iss(line);
    TimedPath row;
    if (iss >> row.timestamp >> row.relative_path) {
      rows.push_back(row);
    }
  }
  return rows;
}

std::vector<AssociatedPair> AssociateStreams(const std::vector<TimedPath>& rgb_rows,
                                             const std::vector<TimedPath>& depth_rows,
                                             double max_delta_seconds = 0.03) {
  std::vector<AssociatedPair> pairs;
  size_t j = 0;
  for (const auto& rgb : rgb_rows) {
    while (j < depth_rows.size() &&
           depth_rows[j].timestamp < rgb.timestamp - max_delta_seconds) {
      ++j;
    }

    int best_index = -1;
    double best_delta = std::numeric_limits<double>::infinity();
    for (size_t candidate = j;
         candidate < depth_rows.size() &&
         depth_rows[candidate].timestamp <= rgb.timestamp + max_delta_seconds;
         ++candidate) {
      const double delta = std::abs(depth_rows[candidate].timestamp - rgb.timestamp);
      if (delta < best_delta) {
        best_delta = delta;
        best_index = static_cast<int>(candidate);
      }
    }

    if (best_index >= 0) {
      pairs.push_back({rgb, depth_rows[static_cast<size_t>(best_index)], best_delta});
      j = static_cast<size_t>(best_index) + 1;
    }
  }
  return pairs;
}

cv::Mat LoadTumDepthMeters(const std::filesystem::path& depth_png_path, double depth_scale = 5000.0) {
  cv::Mat raw = cv::imread(depth_png_path.string(), cv::IMREAD_UNCHANGED);
  if (raw.empty()) {
    throw std::runtime_error("Failed to read depth image: " + depth_png_path.string());
  }
  if (raw.type() != CV_16UC1) {
    throw std::runtime_error("Expected CV_16UC1 depth PNG");
  }

  cv::Mat depth_m(raw.rows, raw.cols, CV_32FC1);
  for (int v = 0; v < raw.rows; ++v) {
    for (int u = 0; u < raw.cols; ++u) {
      const unsigned short value = raw.at<unsigned short>(v, u);
      depth_m.at<float>(v, u) = value == 0 ? 0.0f : static_cast<float>(value / depth_scale);
    }
  }
  return depth_m;
}

CloudResult DepthToPointCloud(const cv::Mat& depth_m, const CameraIntrinsics& k,
                              const cv::Mat& rgb = cv::Mat(), int max_points = 60000,
                              int forced_sample_step = 0) {
  ValidateIntrinsics(k);
  if (depth_m.empty() || depth_m.type() != CV_32FC1) {
    throw std::invalid_argument("depth_m must be a non-empty CV_32FC1 matrix");
  }
  if (!rgb.empty() &&
      (rgb.rows != depth_m.rows || rgb.cols != depth_m.cols || rgb.type() != CV_8UC3)) {
    throw std::invalid_argument("rgb must be CV_8UC3 and match depth resolution");
  }

  CloudResult result;
  result.stats.width = depth_m.cols;
  result.stats.height = depth_m.rows;
  result.stats.total_pixels = depth_m.rows * depth_m.cols;

  double z_sum = 0.0;
  float z_min = std::numeric_limits<float>::infinity();
  float z_max = -std::numeric_limits<float>::infinity();
  for (int v = 0; v < depth_m.rows; ++v) {
    for (int u = 0; u < depth_m.cols; ++u) {
      const float z = depth_m.at<float>(v, u);
      if (IsValidDepth(z)) {
        ++result.stats.valid_depth_pixels;
        z_min = std::min(z_min, z);
        z_max = std::max(z_max, z);
        z_sum += z;
      } else {
        ++result.stats.invalid_depth_pixels;
      }
    }
  }

  result.stats.valid_depth_ratio =
      static_cast<double>(result.stats.valid_depth_pixels) / result.stats.total_pixels;
  result.stats.z_min = std::isfinite(z_min) ? z_min : 0.0f;
  result.stats.z_max = std::isfinite(z_max) ? z_max : 0.0f;
  result.stats.z_mean =
      result.stats.valid_depth_pixels > 0 ? z_sum / result.stats.valid_depth_pixels : 0.0;

  const int effective_max = max_points <= 0 ? result.stats.valid_depth_pixels : max_points;
  const int auto_step = std::max(
      1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(result.stats.valid_depth_pixels) /
                                             std::max(1, effective_max)))));
  result.stats.sample_step = forced_sample_step > 0 ? forced_sample_step : auto_step;

  for (int v = 0; v < depth_m.rows; v += result.stats.sample_step) {
    for (int u = 0; u < depth_m.cols; u += result.stats.sample_step) {
      const float z = depth_m.at<float>(v, u);
      if (!IsValidDepth(z)) {
        continue;
      }
      const cv::Point3f xyz = PixelToCamera(u, v, z, k);
      PointXYZRGB point;
      point.x = xyz.x;
      point.y = xyz.y;
      point.z = xyz.z;
      point.u = u;
      point.v = v;
      if (!rgb.empty()) {
        const cv::Vec3b bgr = rgb.at<cv::Vec3b>(v, u);
        point.r = bgr[2];
        point.g = bgr[1];
        point.b = bgr[0];
      }
      result.points.push_back(point);
      if (max_points > 0 && static_cast<int>(result.points.size()) >= max_points) {
        result.stats.sampled_points = static_cast<int>(result.points.size());
        return result;
      }
    }
  }

  result.stats.sampled_points = static_cast<int>(result.points.size());
  return result;
}

void WritePlyAscii(const std::filesystem::path& file_path, const std::vector<PointXYZRGB>& points) {
  std::ofstream output(file_path);
  if (!output) {
    throw std::runtime_error("Failed to write PLY: " + file_path.string());
  }
  output << "ply\nformat ascii 1.0\n";
  output << "element vertex " << points.size() << "\n";
  output << "property float x\nproperty float y\nproperty float z\n";
  output << "property uchar red\nproperty uchar green\nproperty uchar blue\n";
  output << "end_header\n";
  output << std::fixed << std::setprecision(6);
  for (const auto& p : points) {
    output << p.x << ' ' << p.y << ' ' << p.z << ' ' << static_cast<int>(p.r) << ' '
           << static_cast<int>(p.g) << ' ' << static_cast<int>(p.b) << '\n';
  }
}

void WriteCsv(const std::filesystem::path& file_path, const std::vector<PointXYZRGB>& points) {
  std::ofstream output(file_path);
  if (!output) {
    throw std::runtime_error("Failed to write CSV: " + file_path.string());
  }
  output << "x,y,z,r,g,b,u,v\n";
  output << std::fixed << std::setprecision(6);
  for (const auto& p : points) {
    output << p.x << ',' << p.y << ',' << p.z << ',' << static_cast<int>(p.r) << ','
           << static_cast<int>(p.g) << ',' << static_cast<int>(p.b) << ',' << p.u << ',' << p.v
           << '\n';
  }
}

void WriteDepthPreview(const std::filesystem::path& file_path, const cv::Mat& depth_m) {
  std::vector<float> valid;
  valid.reserve(static_cast<size_t>(depth_m.rows * depth_m.cols));
  for (int v = 0; v < depth_m.rows; ++v) {
    for (int u = 0; u < depth_m.cols; ++u) {
      const float z = depth_m.at<float>(v, u);
      if (IsValidDepth(z)) {
        valid.push_back(z);
      }
    }
  }
  std::sort(valid.begin(), valid.end());
  const float lo = valid.empty() ? 0.0f : valid[static_cast<size_t>(valid.size() * 0.02)];
  const float hi = valid.empty() ? 1.0f : valid[static_cast<size_t>(valid.size() * 0.98)];
  const float range = std::max(1e-6f, hi - lo);

  cv::Mat gray(depth_m.rows, depth_m.cols, CV_8UC1, cv::Scalar(0));
  for (int v = 0; v < depth_m.rows; ++v) {
    for (int u = 0; u < depth_m.cols; ++u) {
      const float z = depth_m.at<float>(v, u);
      if (!IsValidDepth(z)) {
        continue;
      }
      const float normalized = std::clamp((z - lo) / range, 0.0f, 1.0f);
      gray.at<unsigned char>(v, u) = static_cast<unsigned char>(255.0f * (1.0f - normalized));
    }
  }
  cv::Mat color;
  cv::applyColorMap(gray, color, cv::COLORMAP_TURBO);
  cv::imwrite(file_path.string(), color);
}

void WriteProjectionPreview(const std::filesystem::path& file_path, int width, int height,
                            const std::vector<PointXYZRGB>& points, const CameraIntrinsics& k) {
  cv::Mat image(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
  for (const auto& p : points) {
    const cv::Point2f uv = CameraToPixel(cv::Point3f(p.x, p.y, p.z), k);
    const int u = static_cast<int>(std::round(uv.x));
    const int v = static_cast<int>(std::round(uv.y));
    if (u < 0 || u >= width || v < 0 || v >= height) {
      continue;
    }
    image.at<cv::Vec3b>(v, u) = cv::Vec3b(p.b, p.g, p.r);
  }
  cv::imwrite(file_path.string(), image);
}

double ComputeMaxReprojectionError(const std::vector<PointXYZRGB>& points,
                                   const CameraIntrinsics& k) {
  double max_error = 0.0;
  for (const auto& p : points) {
    const cv::Point2f uv = CameraToPixel(cv::Point3f(p.x, p.y, p.z), k);
    const double du = static_cast<double>(uv.x) - p.u;
    const double dv = static_cast<double>(uv.y) - p.v;
    max_error = std::max(max_error, std::hypot(du, dv));
  }
  return max_error;
}

void WritePointCloudPreview(const std::filesystem::path& file_path,
                            const std::vector<PointXYZRGB>& points) {
  if (points.empty()) {
    throw std::invalid_argument("Point cloud preview requires at least one point");
  }

  constexpr int width = 1200;
  constexpr int height = 700;
  constexpr int margin = 50;
  constexpr int gap = 50;
  cv::Mat canvas(height, width, CV_8UC3, cv::Scalar(24, 24, 24));

  const cv::Rect front_panel(margin, 70, (width - 2 * margin - gap) / 2, height - 120);
  const cv::Rect top_panel(front_panel.x + front_panel.width + gap, front_panel.y,
                           front_panel.width, front_panel.height);

  auto draw_view = [&](const cv::Rect& panel, bool top_view, const std::string& title) {
    double x_min = std::numeric_limits<double>::infinity();
    double x_max = -std::numeric_limits<double>::infinity();
    double y_min = std::numeric_limits<double>::infinity();
    double y_max = -std::numeric_limits<double>::infinity();

    for (const auto& p : points) {
      const double x = p.x;
      const double y = top_view ? p.z : p.y;
      if (!std::isfinite(x) || !std::isfinite(y)) {
        continue;
      }
      x_min = std::min(x_min, x);
      x_max = std::max(x_max, x);
      y_min = std::min(y_min, y);
      y_max = std::max(y_max, y);
    }

    if (!std::isfinite(x_min) || !std::isfinite(y_min)) {
      throw std::runtime_error("Point cloud preview has no finite points");
    }
    const double x_range = std::max(1e-9, x_max - x_min);
    const double y_range = std::max(1e-9, y_max - y_min);

    cv::rectangle(canvas, panel, cv::Scalar(80, 80, 80), 1);
    cv::putText(canvas, title, cv::Point(panel.x, 42), cv::FONT_HERSHEY_SIMPLEX, 0.7,
                cv::Scalar(235, 235, 235), 2, cv::LINE_AA);

    for (const auto& p : points) {
      const double x = p.x;
      const double y = top_view ? p.z : p.y;
      if (!std::isfinite(x) || !std::isfinite(y)) {
        continue;
      }
      const int px = panel.x + static_cast<int>((x - x_min) / x_range * (panel.width - 1));
      const int py = panel.y + static_cast<int>((y - y_min) / y_range * (panel.height - 1));
      if (px >= panel.x && px < panel.x + panel.width && py >= panel.y &&
          py < panel.y + panel.height) {
        canvas.at<cv::Vec3b>(py, px) = cv::Vec3b(p.b, p.g, p.r);
      }
    }
  };

  draw_view(front_panel, false, "Front view (X-Y)");
  draw_view(top_panel, true, "Top view (X-Z)");
  cv::putText(canvas, "Metric RGB-D point cloud", cv::Point(margin, height - 20),
              cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(180, 180, 180), 1, cv::LINE_AA);

  if (!cv::imwrite(file_path.string(), canvas)) {
    throw std::runtime_error("Failed to write point cloud preview: " + file_path.string());
  }
}

void WriteMetricsJson(const std::filesystem::path& file_path, const CloudStats& stats,
                      size_t rgb_frames, size_t depth_frames, size_t associated_pairs,
                      double pair_delta_seconds, double max_reprojection_error_px) {
  std::ofstream output(file_path);
  if (!output) {
    throw std::runtime_error("Failed to write metrics JSON: " + file_path.string());
  }

  output << std::fixed << std::setprecision(8);
  output << "{\n";
  output << "  \"rgb_frames\": " << rgb_frames << ",\n";
  output << "  \"depth_frames\": " << depth_frames << ",\n";
  output << "  \"associated_pairs\": " << associated_pairs << ",\n";
  output << "  \"pair_delta_seconds\": " << pair_delta_seconds << ",\n";
  output << "  \"width\": " << stats.width << ",\n";
  output << "  \"height\": " << stats.height << ",\n";
  output << "  \"valid_depth_pixels\": " << stats.valid_depth_pixels << ",\n";
  output << "  \"invalid_depth_pixels\": " << stats.invalid_depth_pixels << ",\n";
  output << "  \"valid_depth_ratio\": " << stats.valid_depth_ratio << ",\n";
  output << "  \"sampled_points\": " << stats.sampled_points << ",\n";
  output << "  \"z_min_m\": " << stats.z_min << ",\n";
  output << "  \"z_max_m\": " << stats.z_max << ",\n";
  output << "  \"z_mean_m\": " << stats.z_mean << ",\n";
  output << "  \"max_reprojection_error_px\": " << max_reprojection_error_px << "\n";
  output << "}\n";
}

}  // namespace rgbd

#ifndef RGBD_POINTCLOUD_LIBRARY
int main(int argc, char** argv) {
  std::filesystem::path dataset_dir = "data/external/tum/rgbd_dataset_freiburg1_xyz";
  std::filesystem::path output_dir = "outputs";
  int frame_index = 0;
  int max_points = 60000;
  const rgbd::CameraIntrinsics k;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--dataset" && i + 1 < argc) {
      dataset_dir = argv[++i];
    } else if (arg == "--output" && i + 1 < argc) {
      output_dir = argv[++i];
    } else if (arg == "--frame-index" && i + 1 < argc) {
      frame_index = std::stoi(argv[++i]);
    } else if (arg == "--max-points" && i + 1 < argc) {
      max_points = std::stoi(argv[++i]);
    }
  }

  std::filesystem::create_directories(output_dir);
  const auto rgb_rows = rgbd::LoadTumList(dataset_dir / "rgb.txt");
  const auto depth_rows = rgbd::LoadTumList(dataset_dir / "depth.txt");
  const auto pairs = rgbd::AssociateStreams(rgb_rows, depth_rows);

  if (frame_index < 0 || frame_index >= static_cast<int>(pairs.size())) {
    throw std::runtime_error("Frame index out of range");
  }
  const auto& pair = pairs[static_cast<size_t>(frame_index)];
  cv::Mat rgb_bgr = cv::imread((dataset_dir / pair.rgb.relative_path).string(), cv::IMREAD_COLOR);
  cv::Mat depth_m = rgbd::LoadTumDepthMeters(dataset_dir / pair.depth.relative_path);
  if (rgb_bgr.empty()) {
    throw std::runtime_error("Failed to read RGB image");
  }

  const auto cloud = rgbd::DepthToPointCloud(depth_m, k, rgb_bgr, max_points);
  const double max_reprojection_error = rgbd::ComputeMaxReprojectionError(cloud.points, k);
  rgbd::WritePlyAscii(output_dir / "point_cloud_sample_cpp.ply", cloud.points);
  rgbd::WriteCsv(output_dir / "point_cloud_sample_cpp.csv", cloud.points);
  rgbd::WriteDepthPreview(output_dir / "depth_preview_cpp.png", depth_m);
  rgbd::WriteProjectionPreview(output_dir / "projection_preview_cpp.png", depth_m.cols, depth_m.rows,
                               cloud.points, k);
  rgbd::WritePointCloudPreview(output_dir / "point_cloud_preview_cpp.png", cloud.points);
  rgbd::WriteMetricsJson(output_dir / "metrics_cpp.json", cloud.stats, rgb_rows.size(),
                         depth_rows.size(), pairs.size(), pair.delta_seconds,
                         max_reprojection_error);

  std::cout << "rgb_frames=" << rgb_rows.size() << "\n";
  std::cout << "depth_frames=" << depth_rows.size() << "\n";
  std::cout << "associated_pairs=" << pairs.size() << "\n";
  std::cout << "valid_depth_pixels=" << cloud.stats.valid_depth_pixels << "\n";
  std::cout << "invalid_depth_pixels=" << cloud.stats.invalid_depth_pixels << "\n";
  std::cout << "sampled_points=" << cloud.stats.sampled_points << "\n";
  std::cout << "z_min=" << cloud.stats.z_min << "\n";
  std::cout << "z_max=" << cloud.stats.z_max << "\n";
  std::cout << "z_mean=" << cloud.stats.z_mean << "\n";
  std::cout << "pair_delta_seconds=" << pair.delta_seconds << "\n";
  std::cout << "max_reprojection_error_px=" << max_reprojection_error << "\n";
  return 0;
}
#endif
