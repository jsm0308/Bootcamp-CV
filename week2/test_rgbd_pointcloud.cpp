#define RGBD_POINTCLOUD_LIBRARY
#include "rgbd_pointcloud.cpp"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>

namespace {

TEST(RgbdPointCloud, Golden3x3DepthMap) {
  const cv::Mat depth =
      (cv::Mat_<float>(3, 3) << 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 1.0f, 1.0f, 1.0f, 0.0f);
  const rgbd::CameraIntrinsics k{1.0, 1.0, 1.0, 1.0};

  const auto result = rgbd::DepthToPointCloud(depth, k, cv::Mat(), 0, 1);
  EXPECT_EQ(result.stats.valid_depth_pixels, 8);
  EXPECT_EQ(result.stats.invalid_depth_pixels, 1);
  EXPECT_EQ(result.points.size(), 8);

  const auto find_point = [&](int u, int v) {
    return *std::find_if(result.points.begin(), result.points.end(), [&](const auto& point) {
      return point.u == u && point.v == v;
    });
  };

  const auto center = find_point(1, 1);
  EXPECT_FLOAT_EQ(center.x, 0.0f);
  EXPECT_FLOAT_EQ(center.y, 0.0f);
  EXPECT_FLOAT_EQ(center.z, 2.0f);

  const auto right = find_point(2, 1);
  EXPECT_FLOAT_EQ(right.x, 1.0f);
  EXPECT_FLOAT_EQ(right.y, 0.0f);
  EXPECT_FLOAT_EQ(right.z, 1.0f);
}

TEST(RgbdPointCloud, InvalidDepthValuesAreSkipped) {
  const cv::Mat depth =
      (cv::Mat_<float>(2, 3) << 0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(),
       std::numeric_limits<float>::infinity(), 1.5f, 2.0f);
  const rgbd::CameraIntrinsics k{2.0, 2.0, 0.0, 0.0};

  const auto result = rgbd::DepthToPointCloud(depth, k, cv::Mat(), 0, 1);
  EXPECT_EQ(result.stats.valid_depth_pixels, 2);
  EXPECT_EQ(result.stats.invalid_depth_pixels, 4);
  EXPECT_EQ(result.points.size(), 2);
}

TEST(RgbdPointCloud, ProjectionRoundTripPreservesPixel) {
  const rgbd::CameraIntrinsics k{517.3, 516.5, 318.6, 255.3};
  const std::vector<cv::Point3f> points = {
      rgbd::PixelToCamera(100, 50, 0.8f, k),
      rgbd::PixelToCamera(320, 240, 1.2f, k),
      rgbd::PixelToCamera(500, 350, 2.4f, k),
  };
  const std::vector<cv::Point2f> expected = {
      cv::Point2f(100.0f, 50.0f),
      cv::Point2f(320.0f, 240.0f),
      cv::Point2f(500.0f, 350.0f),
  };

  for (size_t i = 0; i < points.size(); ++i) {
    const cv::Point2f pixel = rgbd::CameraToPixel(points[i], k);
    EXPECT_NEAR(pixel.x, expected[i].x, 1e-4);
    EXPECT_NEAR(pixel.y, expected[i].y, 1e-4);
  }
}

TEST(RgbdPointCloud, RejectsInvalidFocalLength) {
  EXPECT_THROW(rgbd::ValidateIntrinsics({0.0, 1.0, 0.0, 0.0}), std::invalid_argument);
  EXPECT_THROW(rgbd::ValidateIntrinsics({1.0, -1.0, 0.0, 0.0}), std::invalid_argument);
}

TEST(RgbdPointCloud, AssociatesTimestampListsOneToOne) {
  const std::vector<rgbd::TimedPath> rgb_rows = {
      {1.000, "rgb/a.png"},
      {1.040, "rgb/b.png"},
      {1.080, "rgb/c.png"},
  };
  const std::vector<rgbd::TimedPath> depth_rows = {
      {0.990, "depth/a.png"},
      {1.055, "depth/b.png"},
      {1.200, "depth/c.png"},
  };

  const auto pairs = rgbd::AssociateStreams(rgb_rows, depth_rows, 0.03);
  ASSERT_EQ(pairs.size(), 2);
  EXPECT_EQ(pairs[0].rgb.relative_path, "rgb/a.png");
  EXPECT_EQ(pairs[0].depth.relative_path, "depth/a.png");
  EXPECT_EQ(pairs[1].rgb.relative_path, "rgb/b.png");
  EXPECT_EQ(pairs[1].depth.relative_path, "depth/b.png");
}

TEST(RgbdPointCloud, PlyWriterStoresVertexCount) {
  const auto temp_path = std::filesystem::temp_directory_path() / "rgbd_pointcloud_test.ply";
  rgbd::WritePlyAscii(temp_path,
                      {{0.0f, 0.0f, 1.0f, 255, 0, 0, 0, 0},
                       {1.0f, 0.0f, 1.0f, 0, 255, 0, 1, 0}});

  std::ifstream input(temp_path);
  std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  EXPECT_NE(text.find("element vertex 2"), std::string::npos);
  EXPECT_NE(text.find("255 0 0"), std::string::npos);
}

TEST(RgbdPointCloud, PointCloudPreviewCreatesReadableImage) {
  const auto temp_path = std::filesystem::temp_directory_path() / "rgbd_pointcloud_preview_test.png";
  const std::vector<rgbd::PointXYZRGB> points = {
      {-0.2f, -0.1f, 0.8f, 255, 0, 0, 0, 0},
      {0.0f, 0.1f, 1.0f, 0, 255, 0, 1, 1},
      {0.2f, 0.0f, 1.2f, 0, 0, 255, 2, 2},
  };

  rgbd::WritePointCloudPreview(temp_path, points);
  const cv::Mat preview = cv::imread(temp_path.string(), cv::IMREAD_COLOR);
  ASSERT_FALSE(preview.empty());
  EXPECT_EQ(preview.cols, 1200);
  EXPECT_EQ(preview.rows, 700);
  std::filesystem::remove(temp_path);
}

}  // namespace
