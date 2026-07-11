"use strict";

const fs = require("fs");
const os = require("os");
const path = require("path");
const test = require("node:test");
const assert = require("node:assert/strict");

const {
  associateStreams,
  cameraToPixel,
  depthToPointCloud,
  isValidDepth,
  parseTumList,
  pixelToCamera,
  validateIntrinsics,
  writePlyAscii,
} = require("./rgbd_pointcloud");

test("golden 3x3 depth map converts known pixels into known 3D points", () => {
  const depth = Float32Array.from([1, 1, 1, 1, 2, 1, 1, 1, 0]);
  const k = { fx: 1, fy: 1, cx: 1, cy: 1 };

  const result = depthToPointCloud(depth, 3, 3, k, null, { sampleStep: 1 });
  assert.equal(result.stats.validDepthPixels, 8);
  assert.equal(result.stats.invalidDepthPixels, 1);
  assert.equal(result.points.length, 8);

  const center = result.points.find((p) => p.u === 1 && p.v === 1);
  assert.deepEqual(
    { x: center.x, y: center.y, z: center.z },
    { x: 0, y: 0, z: 2 },
  );

  const right = result.points.find((p) => p.u === 2 && p.v === 1);
  assert.deepEqual(
    { x: right.x, y: right.y, z: right.z },
    { x: 1, y: 0, z: 1 },
  );

  const left = result.points.find((p) => p.u === 0 && p.v === 1);
  assert.deepEqual(
    { x: left.x, y: left.y, z: left.z },
    { x: -1, y: 0, z: 1 },
  );
});

test("invalid depth values are filtered before point cloud creation", () => {
  const depth = Float32Array.from([0, -1, Number.NaN, Number.POSITIVE_INFINITY, 1.5, 2.0]);
  const k = { fx: 2, fy: 2, cx: 0, cy: 0 };

  assert.equal(isValidDepth(0), false);
  assert.equal(isValidDepth(-1), false);
  assert.equal(isValidDepth(Number.NaN), false);
  assert.equal(isValidDepth(Number.POSITIVE_INFINITY), false);
  assert.equal(isValidDepth(1.5), true);

  const result = depthToPointCloud(depth, 3, 2, k, null, { sampleStep: 1 });
  assert.equal(result.stats.validDepthPixels, 2);
  assert.equal(result.stats.invalidDepthPixels, 4);
  assert.equal(result.points.length, 2);
});

test("2D to 3D to 2D projection round trip preserves pixel location", () => {
  const k = { fx: 517.3, fy: 516.5, cx: 318.6, cy: 255.3 };
  const samples = [
    { u: 100, v: 50, z: 0.8 },
    { u: 320, v: 240, z: 1.2 },
    { u: 500, v: 350, z: 2.4 },
  ];

  for (const sample of samples) {
    const point = pixelToCamera(sample.u, sample.v, sample.z, k);
    const pixel = cameraToPixel(point, k);
    assert.ok(Math.abs(pixel.u - sample.u) < 1e-9);
    assert.ok(Math.abs(pixel.v - sample.v) < 1e-9);
  }
});

test("camera intrinsics reject invalid focal lengths", () => {
  assert.throws(() => validateIntrinsics({ fx: 0, fy: 1, cx: 0, cy: 0 }), /focal/);
  assert.throws(() => validateIntrinsics({ fx: 1, fy: -1, cx: 0, cy: 0 }), /focal/);
});

test("TUM timestamp lists are parsed and associated by nearest timestamp", () => {
  const rgb = parseTumList(`
    # timestamp filename
    1.000 rgb/a.png
    1.040 rgb/b.png
    1.080 rgb/c.png
  `);
  const depth = parseTumList(`
    # timestamp filename
    0.990 depth/a.png
    1.055 depth/b.png
    1.200 depth/c.png
  `);

  const pairs = associateStreams(rgb, depth, 0.03);
  assert.equal(pairs.length, 2);
  assert.equal(pairs[0].a.path, "rgb/a.png");
  assert.equal(pairs[0].b.path, "depth/a.png");
  assert.equal(pairs[1].a.path, "rgb/b.png");
  assert.equal(pairs[1].b.path, "depth/b.png");
});

test("PLY writer stores the declared vertex count", () => {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), "rgbd-pointcloud-test-"));
  const plyPath = path.join(tmpDir, "sample.ply");
  writePlyAscii(plyPath, [
    { x: 0, y: 0, z: 1, r: 255, g: 0, b: 0 },
    { x: 1, y: 0, z: 1, r: 0, g: 255, b: 0 },
  ]);

  const text = fs.readFileSync(plyPath, "utf8");
  assert.match(text, /element vertex 2/);
  assert.match(text, /255 0 0/);
});
