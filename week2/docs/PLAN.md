# Week 2 Plan: Tested RGB-D to Point Cloud Converter

## 1. Project Goal

Build a small OpenCV project that converts RGB-D data into a 3D point cloud and verifies the geometry with unit tests.

This project does not claim to reconstruct true 3D from a single RGB image. The main path uses metric depth maps. Food101 images from week 1 are only used as a pseudo-depth demo and limitation comparison.

## 2. Core Question

- Given a pixel coordinate `(u, v)`, a depth value `Z`, and camera intrinsics `(fx, fy, cx, cy)`, can we correctly compute the 3D point `(X, Y, Z)`?
- Can we prove the conversion with synthetic ground-truth tests?
- Can we visualize the output as depth maps, projections, and point cloud files?

## 3. Why Depth Is Needed

A 2D image alone only gives pixel positions and color. It does not tell how far the object is from the camera.

Point cloud generation needs depth:

```text
pixel: (u, v)
depth: Z
intrinsics: fx, fy, cx, cy

X = (u - cx) * Z / fx
Y = (v - cy) * Z / fy
Z = depth
```

## 4. Data Plan

### A. Synthetic Depth Maps - required

Used for unit tests because the correct answer is known.

- `tiny_3x3`: hand-computable golden case
- `constant_plane`: all pixels have the same depth
- `sloped_plane_x`: depth increases along x
- `center_bump`: center area is closer/higher
- `invalid_holes`: includes zero, negative, NaN, and infinity depth

### B. TUM RGB-D Dataset - real RGB-D demo candidate

Use one small sequence such as `fr1/xyz` if download is acceptable. It contains registered RGB/depth images from Kinect, 640x480, and official camera/depth format information.

### C. Week 1 Food101 Images - pseudo-depth comparison only

Use existing Food101 images only to show why grayscale height maps are not real metric 3D.

## 5. Pseudo 3D vs Metric 3D

| Type | Input | Meaning | Role |
|---|---|---|---|
| Pseudo 3D | RGB/grayscale | brightness is treated as height | visual demo only |
| Metric 3D | RGB + depth map | Z is actual camera distance | main project |

## 6. Why 3D -> 2D Projection Is Needed

Projection is a verification step. If a pixel is converted to 3D and projected back to 2D, it should return to the original pixel location.

```text
2D pixel + depth -> 3D point -> projected 2D pixel
expected: projected pixel is almost the same as original pixel
```

This proves that the camera geometry is implemented correctly.

## 7. Why Depth=0 Must Be Skipped

In many RGB-D datasets, depth value 0 means missing/no data, not an object at zero meters.

If depth=0 is used directly:

- many false points collapse at the camera origin
- point cloud shape becomes corrupted
- averages, bounding boxes, and visualization become wrong
- robot-distance interpretation becomes unsafe

Therefore zero, negative, NaN, and infinity depth must be filtered out.

## 8. Minimal File Plan

Today: only this planning file.

Implementation later should stay compact:

```text
week2/
  README.md
  CMakeLists.txt
  src/
    rgbd_pointcloud.cpp
    rgbd_pointcloud.js
  tests/
    test_rgbd_pointcloud.cpp
    test_rgbd_pointcloud.js
  docs/
    PLAN.md
    CHECKPOINTS.md
    REPORT.md
  scripts/
  outputs/
  submission/
```

No large file explosion. If the instructor expects a simpler format, the implementation can be reduced to one main source file plus one test file.

## 9. Unit Test Plan

### Golden 3x3 Case

```text
depth =
1 1 1
1 2 1
1 1 0

K: fx=1, fy=1, cx=1, cy=1
```

Expected:

- center pixel `(1,1), z=2` -> `(0,0,2)`
- right pixel `(2,1), z=1` -> `(1,0,1)`
- left pixel `(0,1), z=1` -> `(-1,0,1)`
- bottom-right depth `0` is skipped
- valid point count is 8

Other tests:

- invalid camera focal length fails validation
- invalid depth values are skipped
- point count matches valid depth count
- 3D -> 2D projection round-trip error is below threshold
- PLY/CSV export has correct point count

## 10. Evaluation Criteria

- Unit test pass rate: 100%
- Golden case coordinate error: near zero
- Round-trip projection error: below small epsilon
- Invalid depth filtering: correct valid point count
- Visualization: depth colormap, point cloud file, projection image
- Report: clearly explains pseudo 3D vs metric 3D

## 11. PPT Plan

1. Problem and motivation
2. RGB-D to 3D geometry
3. Unit test design
4. Results and visualization
5. Limitations and Physical AI extension

## 12. Extension

Later, a real object can be captured from multiple views. A separate validation can compare whether projected/reconstructed geometry is consistent across views. That becomes a multi-view reconstruction or RGB-D fusion problem, which is beyond this week but is a strong next step.
