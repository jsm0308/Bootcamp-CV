"use strict";

const fs = require("fs");
const path = require("path");
const zlib = require("zlib");

const PNG_SIGNATURE = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);

const TUM_FR1_INTRINSICS = {
  fx: 517.3,
  fy: 516.5,
  cx: 318.6,
  cy: 255.3,
};

function parseArgs(argv) {
  const args = {};
  for (let i = 0; i < argv.length; i += 1) {
    const token = argv[i];
    if (!token.startsWith("--")) continue;
    const key = token.slice(2);
    const next = argv[i + 1];
    if (next === undefined || next.startsWith("--")) {
      args[key] = true;
    } else {
      args[key] = next;
      i += 1;
    }
  }
  return args;
}

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function readText(filePath) {
  return fs.readFileSync(filePath, "utf8");
}

function parseTumList(text) {
  return text
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0 && !line.startsWith("#"))
    .map((line) => {
      const [timestamp, relPath] = line.split(/\s+/);
      return { timestamp: Number(timestamp), path: relPath };
    })
    .filter((row) => Number.isFinite(row.timestamp) && row.path);
}

function associateStreams(aRows, bRows, maxDeltaSeconds = 0.03) {
  const pairs = [];
  let j = 0;
  for (const a of aRows) {
    while (j < bRows.length && bRows[j].timestamp < a.timestamp - maxDeltaSeconds) {
      j += 1;
    }

    let bestIndex = -1;
    let bestDelta = Infinity;
    for (
      let candidate = j;
      candidate < bRows.length && bRows[candidate].timestamp <= a.timestamp + maxDeltaSeconds;
      candidate += 1
    ) {
      const delta = Math.abs(bRows[candidate].timestamp - a.timestamp);
      if (delta < bestDelta) {
        bestDelta = delta;
        bestIndex = candidate;
      }
    }

    if (bestIndex >= 0) {
      pairs.push({ a, b: bRows[bestIndex], delta: bestDelta });
      j = bestIndex + 1;
    }
  }
  return pairs;
}

function channelCount(colorType) {
  switch (colorType) {
    case 0:
      return 1;
    case 2:
      return 3;
    case 4:
      return 2;
    case 6:
      return 4;
    default:
      throw new Error(`Unsupported PNG color type: ${colorType}`);
  }
}

function paethPredictor(a, b, c) {
  const p = a + b - c;
  const pa = Math.abs(p - a);
  const pb = Math.abs(p - b);
  const pc = Math.abs(p - c);
  if (pa <= pb && pa <= pc) return a;
  if (pb <= pc) return b;
  return c;
}

function decodePng(buffer) {
  if (!buffer.subarray(0, 8).equals(PNG_SIGNATURE)) {
    throw new Error("Invalid PNG signature");
  }

  let offset = 8;
  let width = 0;
  let height = 0;
  let bitDepth = 0;
  let colorType = 0;
  let interlace = 0;
  const idatChunks = [];

  while (offset < buffer.length) {
    const length = buffer.readUInt32BE(offset);
    const type = buffer.toString("ascii", offset + 4, offset + 8);
    const dataStart = offset + 8;
    const dataEnd = dataStart + length;
    const data = buffer.subarray(dataStart, dataEnd);

    if (type === "IHDR") {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      bitDepth = data[8];
      colorType = data[9];
      interlace = data[12];
    } else if (type === "IDAT") {
      idatChunks.push(data);
    } else if (type === "IEND") {
      break;
    }

    offset = dataEnd + 4;
  }

  if (interlace !== 0) {
    throw new Error("Interlaced PNG is not supported");
  }
  if (![8, 16].includes(bitDepth)) {
    throw new Error(`Unsupported PNG bit depth: ${bitDepth}`);
  }

  const channels = channelCount(colorType);
  const bytesPerSample = bitDepth / 8;
  const bytesPerPixel = channels * bytesPerSample;
  const stride = width * bytesPerPixel;
  const inflated = zlib.inflateSync(Buffer.concat(idatChunks));
  const pixels = Buffer.alloc(height * stride);

  let sourceOffset = 0;
  for (let y = 0; y < height; y += 1) {
    const filterType = inflated[sourceOffset];
    sourceOffset += 1;
    const row = inflated.subarray(sourceOffset, sourceOffset + stride);
    sourceOffset += stride;

    const outputRowOffset = y * stride;
    const previousRowOffset = (y - 1) * stride;

    for (let x = 0; x < stride; x += 1) {
      const raw = row[x];
      const left = x >= bytesPerPixel ? pixels[outputRowOffset + x - bytesPerPixel] : 0;
      const up = y > 0 ? pixels[previousRowOffset + x] : 0;
      const upLeft =
        y > 0 && x >= bytesPerPixel ? pixels[previousRowOffset + x - bytesPerPixel] : 0;

      let value;
      if (filterType === 0) {
        value = raw;
      } else if (filterType === 1) {
        value = raw + left;
      } else if (filterType === 2) {
        value = raw + up;
      } else if (filterType === 3) {
        value = raw + Math.floor((left + up) / 2);
      } else if (filterType === 4) {
        value = raw + paethPredictor(left, up, upLeft);
      } else {
        throw new Error(`Unsupported PNG filter type: ${filterType}`);
      }
      pixels[outputRowOffset + x] = value & 0xff;
    }
  }

  return { width, height, bitDepth, colorType, channels, data: pixels };
}

function decodeDepthMeters(png, depthScale = 5000) {
  if (png.colorType !== 0 || png.bitDepth !== 16) {
    throw new Error(
      `Expected 16-bit grayscale depth PNG, got colorType=${png.colorType}, bitDepth=${png.bitDepth}`,
    );
  }

  const depth = new Float32Array(png.width * png.height);
  for (let i = 0; i < depth.length; i += 1) {
    const byteIndex = i * 2;
    const raw = (png.data[byteIndex] << 8) | png.data[byteIndex + 1];
    depth[i] = raw === 0 ? 0 : raw / depthScale;
  }
  return depth;
}

function decodeRgb8(png) {
  const rgb = new Uint8Array(png.width * png.height * 3);

  if (png.colorType === 2 && png.bitDepth === 8) {
    rgb.set(png.data);
    return rgb;
  }

  if (png.colorType === 6 && png.bitDepth === 8) {
    for (let i = 0, j = 0; i < png.data.length; i += 4, j += 3) {
      rgb[j] = png.data[i];
      rgb[j + 1] = png.data[i + 1];
      rgb[j + 2] = png.data[i + 2];
    }
    return rgb;
  }

  if (png.colorType === 0 && png.bitDepth === 8) {
    for (let i = 0, j = 0; i < png.data.length; i += 1, j += 3) {
      rgb[j] = png.data[i];
      rgb[j + 1] = png.data[i];
      rgb[j + 2] = png.data[i];
    }
    return rgb;
  }

  throw new Error(`Unsupported RGB PNG format: colorType=${png.colorType}, bitDepth=${png.bitDepth}`);
}

function validateIntrinsics(k) {
  for (const key of ["fx", "fy", "cx", "cy"]) {
    if (!Number.isFinite(k[key])) {
      throw new Error(`Invalid camera intrinsic ${key}: ${k[key]}`);
    }
  }
  if (k.fx <= 0 || k.fy <= 0) {
    throw new Error("Camera focal lengths fx and fy must be positive");
  }
}

function isValidDepth(z) {
  return Number.isFinite(z) && z > 0;
}

function pixelToCamera(u, v, z, k) {
  validateIntrinsics(k);
  if (!isValidDepth(z)) {
    throw new Error(`Invalid depth: ${z}`);
  }
  return {
    x: ((u - k.cx) * z) / k.fx,
    y: ((v - k.cy) * z) / k.fy,
    z,
  };
}

function cameraToPixel(point, k) {
  validateIntrinsics(k);
  if (!isValidDepth(point.z)) {
    throw new Error(`Invalid point z: ${point.z}`);
  }
  return {
    u: (k.fx * point.x) / point.z + k.cx,
    v: (k.fy * point.y) / point.z + k.cy,
  };
}

function depthToPointCloud(depth, width, height, k, rgb = null, options = {}) {
  validateIntrinsics(k);
  if (depth.length !== width * height) {
    throw new Error(`Depth size mismatch: ${depth.length} vs ${width}x${height}`);
  }
  if (rgb && rgb.length !== width * height * 3) {
    throw new Error(`RGB size mismatch: ${rgb.length} vs ${width}x${height}x3`);
  }

  let validCount = 0;
  let invalidCount = 0;
  let zMin = Infinity;
  let zMax = -Infinity;
  let zSum = 0;
  for (let i = 0; i < depth.length; i += 1) {
    const z = depth[i];
    if (isValidDepth(z)) {
      validCount += 1;
      zMin = Math.min(zMin, z);
      zMax = Math.max(zMax, z);
      zSum += z;
    } else {
      invalidCount += 1;
    }
  }

  const maxPoints =
    options.maxPoints === undefined || options.maxPoints === null
      ? Number.POSITIVE_INFINITY
      : Number(options.maxPoints);
  const sampleStep =
    options.sampleStep || Math.max(1, Math.ceil(Math.sqrt(validCount / Math.max(1, maxPoints))));

  const points = [];
  for (let v = 0; v < height; v += sampleStep) {
    for (let u = 0; u < width; u += sampleStep) {
      const index = v * width + u;
      const z = depth[index];
      if (!isValidDepth(z)) continue;
      const point = pixelToCamera(u, v, z, k);
      const rgbIndex = index * 3;
      points.push({
        x: point.x,
        y: point.y,
        z: point.z,
        r: rgb ? rgb[rgbIndex] : 255,
        g: rgb ? rgb[rgbIndex + 1] : 255,
        b: rgb ? rgb[rgbIndex + 2] : 255,
        u,
        v,
      });
      if (points.length >= maxPoints) {
        break;
      }
    }
    if (points.length >= maxPoints) {
      break;
    }
  }

  return {
    points,
    stats: {
      width,
      height,
      totalPixels: width * height,
      validDepthPixels: validCount,
      invalidDepthPixels: invalidCount,
      validDepthRatio: validCount / (width * height),
      sampledPoints: points.length,
      sampleStep,
      zMin: Number.isFinite(zMin) ? zMin : null,
      zMax: Number.isFinite(zMax) ? zMax : null,
      zMean: validCount > 0 ? zSum / validCount : null,
    },
  };
}

function writePlyAscii(filePath, points) {
  const header = [
    "ply",
    "format ascii 1.0",
    `element vertex ${points.length}`,
    "property float x",
    "property float y",
    "property float z",
    "property uchar red",
    "property uchar green",
    "property uchar blue",
    "end_header",
  ].join("\n");
  const body = points
    .map((p) =>
      [
        p.x.toFixed(6),
        p.y.toFixed(6),
        p.z.toFixed(6),
        Math.round(p.r),
        Math.round(p.g),
        Math.round(p.b),
      ].join(" "),
    )
    .join("\n");
  fs.writeFileSync(filePath, `${header}\n${body}\n`, "utf8");
}

function writeCsv(filePath, points) {
  const rows = ["x,y,z,r,g,b,u,v"];
  for (const p of points) {
    rows.push(
      [
        p.x.toFixed(6),
        p.y.toFixed(6),
        p.z.toFixed(6),
        Math.round(p.r),
        Math.round(p.g),
        Math.round(p.b),
        p.u,
        p.v,
      ].join(","),
    );
  }
  fs.writeFileSync(filePath, `${rows.join("\n")}\n`, "utf8");
}

function writeDepthPreviewPgm(filePath, depth, width, height) {
  const valid = [];
  for (const z of depth) {
    if (isValidDepth(z)) valid.push(z);
  }
  valid.sort((a, b) => a - b);
  const lo = valid.length ? valid[Math.floor(valid.length * 0.02)] : 0;
  const hi = valid.length ? valid[Math.floor(valid.length * 0.98)] : 1;
  const range = Math.max(1e-6, hi - lo);
  const out = Buffer.alloc(width * height);
  for (let i = 0; i < depth.length; i += 1) {
    const z = depth[i];
    if (!isValidDepth(z)) {
      out[i] = 0;
    } else {
      const normalized = Math.max(0, Math.min(1, (z - lo) / range));
      out[i] = Math.round(255 * (1 - normalized));
    }
  }
  fs.writeFileSync(filePath, Buffer.concat([Buffer.from(`P5\n${width} ${height}\n255\n`), out]));
}

function writeBmp24(filePath, width, height, rgbBuffer) {
  if (rgbBuffer.length !== width * height * 3) {
    throw new Error(`RGB buffer size mismatch: ${rgbBuffer.length} vs ${width}x${height}x3`);
  }
  const rowSize = Math.ceil((width * 3) / 4) * 4;
  const pixelDataSize = rowSize * height;
  const fileSize = 54 + pixelDataSize;
  const header = Buffer.alloc(54, 0);
  header.write("BM", 0, "ascii");
  header.writeUInt32LE(fileSize, 2);
  header.writeUInt32LE(54, 10);
  header.writeUInt32LE(40, 14);
  header.writeInt32LE(width, 18);
  header.writeInt32LE(height, 22);
  header.writeUInt16LE(1, 26);
  header.writeUInt16LE(24, 28);
  header.writeUInt32LE(pixelDataSize, 34);

  const data = Buffer.alloc(pixelDataSize, 0);
  for (let y = 0; y < height; y += 1) {
    const sourceY = height - 1 - y;
    for (let x = 0; x < width; x += 1) {
      const src = (sourceY * width + x) * 3;
      const dst = y * rowSize + x * 3;
      data[dst] = rgbBuffer[src + 2];
      data[dst + 1] = rgbBuffer[src + 1];
      data[dst + 2] = rgbBuffer[src];
    }
  }
  fs.writeFileSync(filePath, Buffer.concat([header, data]));
}

function depthPreviewRgb(depth, width, height) {
  const valid = [];
  for (const z of depth) {
    if (isValidDepth(z)) valid.push(z);
  }
  valid.sort((a, b) => a - b);
  const lo = valid.length ? valid[Math.floor(valid.length * 0.02)] : 0;
  const hi = valid.length ? valid[Math.floor(valid.length * 0.98)] : 1;
  const range = Math.max(1e-6, hi - lo);
  const out = Buffer.alloc(width * height * 3);
  for (let i = 0; i < depth.length; i += 1) {
    const z = depth[i];
    const dst = i * 3;
    if (!isValidDepth(z)) {
      out[dst] = 0;
      out[dst + 1] = 0;
      out[dst + 2] = 0;
    } else {
      const normalized = Math.max(0, Math.min(1, (z - lo) / range));
      const intensity = Math.round(255 * (1 - normalized));
      out[dst] = intensity;
      out[dst + 1] = intensity;
      out[dst + 2] = intensity;
    }
  }
  return out;
}

function writeDepthPreviewBmp(filePath, depth, width, height) {
  writeBmp24(filePath, width, height, depthPreviewRgb(depth, width, height));
}

function writeProjectionPreviewPpm(filePath, width, height, points, k) {
  const image = Buffer.alloc(width * height * 3, 0);
  for (const p of points) {
    const px = cameraToPixel(p, k);
    const u = Math.round(px.u);
    const v = Math.round(px.v);
    if (u < 0 || u >= width || v < 0 || v >= height) continue;
    const index = (v * width + u) * 3;
    image[index] = Math.round(p.r);
    image[index + 1] = Math.round(p.g);
    image[index + 2] = Math.round(p.b);
  }
  fs.writeFileSync(filePath, Buffer.concat([Buffer.from(`P6\n${width} ${height}\n255\n`), image]));
}

function projectionPreviewRgb(width, height, points, k) {
  const image = Buffer.alloc(width * height * 3, 0);
  for (const p of points) {
    const px = cameraToPixel(p, k);
    const u = Math.round(px.u);
    const v = Math.round(px.v);
    if (u < 0 || u >= width || v < 0 || v >= height) continue;
    const index = (v * width + u) * 3;
    image[index] = Math.round(p.r);
    image[index + 1] = Math.round(p.g);
    image[index + 2] = Math.round(p.b);
  }
  return image;
}

function writeProjectionPreviewBmp(filePath, width, height, points, k) {
  writeBmp24(filePath, width, height, projectionPreviewRgb(width, height, points, k));
}

function loadTumFrame(datasetDir, frameIndex, depthScale) {
  const rgbRows = parseTumList(readText(path.join(datasetDir, "rgb.txt")));
  const depthRows = parseTumList(readText(path.join(datasetDir, "depth.txt")));
  const pairs = associateStreams(rgbRows, depthRows);
  if (!pairs.length) {
    throw new Error("No RGB/depth pairs found");
  }
  if (frameIndex < 0 || frameIndex >= pairs.length) {
    throw new Error(`Frame index out of range: ${frameIndex}. Available: 0..${pairs.length - 1}`);
  }

  const pair = pairs[frameIndex];
  const rgbPng = decodePng(fs.readFileSync(path.join(datasetDir, pair.a.path)));
  const depthPng = decodePng(fs.readFileSync(path.join(datasetDir, pair.b.path)));

  if (rgbPng.width !== depthPng.width || rgbPng.height !== depthPng.height) {
    throw new Error(
      `RGB/depth resolution mismatch: ${rgbPng.width}x${rgbPng.height} vs ${depthPng.width}x${depthPng.height}`,
    );
  }

  return {
    rgbRows,
    depthRows,
    pairs,
    pair,
    width: depthPng.width,
    height: depthPng.height,
    rgb: decodeRgb8(rgbPng),
    depth: decodeDepthMeters(depthPng, depthScale),
  };
}

function runCli() {
  const args = parseArgs(process.argv.slice(2));
  const datasetDir =
    args.dataset || path.join(__dirname, "data", "external", "tum", "rgbd_dataset_freiburg1_xyz");
  const outputDir = args.output || path.join(__dirname, "outputs");
  const frameIndex = Number(args["frame-index"] || 0);
  const maxPoints = Number(args["max-points"] || 60000);
  const depthScale = Number(args["depth-scale"] || 5000);
  const intrinsics = {
    fx: Number(args.fx || TUM_FR1_INTRINSICS.fx),
    fy: Number(args.fy || TUM_FR1_INTRINSICS.fy),
    cx: Number(args.cx || TUM_FR1_INTRINSICS.cx),
    cy: Number(args.cy || TUM_FR1_INTRINSICS.cy),
  };

  ensureDir(outputDir);
  const frame = loadTumFrame(datasetDir, frameIndex, depthScale);
  const cloud = depthToPointCloud(frame.depth, frame.width, frame.height, intrinsics, frame.rgb, {
    maxPoints,
  });

  const plyPath = path.join(outputDir, "point_cloud_sample.ply");
  const csvPath = path.join(outputDir, "point_cloud_sample.csv");
  const depthPreviewPath = path.join(outputDir, "depth_preview.pgm");
  const depthPreviewBmpPath = path.join(outputDir, "depth_preview.bmp");
  const projectionPreviewPath = path.join(outputDir, "projection_preview.ppm");
  const projectionPreviewBmpPath = path.join(outputDir, "projection_preview.bmp");
  const metricsPath = path.join(outputDir, "metrics.json");

  writePlyAscii(plyPath, cloud.points);
  writeCsv(csvPath, cloud.points);
  writeDepthPreviewPgm(depthPreviewPath, frame.depth, frame.width, frame.height);
  writeDepthPreviewBmp(depthPreviewBmpPath, frame.depth, frame.width, frame.height);
  writeProjectionPreviewPpm(projectionPreviewPath, frame.width, frame.height, cloud.points, intrinsics);
  writeProjectionPreviewBmp(
    projectionPreviewBmpPath,
    frame.width,
    frame.height,
    cloud.points,
    intrinsics,
  );

  const metrics = {
    project: "Tested RGB-D to Point Cloud Converter",
    datasetDir,
    frameIndex,
    rgbFrames: frame.rgbRows.length,
    depthFrames: frame.depthRows.length,
    associatedPairs: frame.pairs.length,
    selectedRgb: frame.pair.a.path,
    selectedDepth: frame.pair.b.path,
    timestampDeltaSeconds: frame.pair.delta,
    intrinsics,
    depthScale,
    ...cloud.stats,
    outputs: {
      ply: plyPath,
      csv: csvPath,
      depthPreview: depthPreviewPath,
      depthPreviewBmp: depthPreviewBmpPath,
      projectionPreview: projectionPreviewPath,
      projectionPreviewBmp: projectionPreviewBmpPath,
      metrics: metricsPath,
    },
  };

  fs.writeFileSync(metricsPath, `${JSON.stringify(metrics, null, 2)}\n`, "utf8");
  console.log(JSON.stringify(metrics, null, 2));
}

if (require.main === module) {
  runCli();
}

module.exports = {
  TUM_FR1_INTRINSICS,
  parseTumList,
  associateStreams,
  decodePng,
  decodeDepthMeters,
  decodeRgb8,
  validateIntrinsics,
  isValidDepth,
  pixelToCamera,
  cameraToPixel,
  depthToPointCloud,
  writePlyAscii,
  writeCsv,
  writeDepthPreviewPgm,
  writeDepthPreviewBmp,
  writeProjectionPreviewPpm,
  writeProjectionPreviewBmp,
  loadTumFrame,
};
