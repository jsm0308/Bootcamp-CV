# Week 2 RGB-D Point Cloud Checkpoints

## 이 작업이 하는 일

이 과제는 일반 RGB 사진 한 장을 진짜 3D로 복원하는 작업이 아니다.

입력은 RGB-D 데이터다. 즉 같은 장면에 대해 다음 두 파일이 같이 있다.

- RGB image: 색상 사진
- depth image: 각 픽셀이 카메라에서 몇 m 떨어져 있는지 담은 16-bit depth map

TUM `freiburg1_xyz` 데이터셋에는 RGB 이미지 798장과 depth 이미지 798장이 이미 들어 있다. 이 depth는 우리가 추정한 값이 아니라 Kinect 센서로 기록된 값이다.

작업의 핵심은 다음이다.

```text
2D pixel (u, v) + depth Z + camera intrinsics
-> 3D point (X, Y, Z)
-> point cloud file
```

공식:

```text
X = (u - cx) * Z / fx
Y = (v - cy) * Z / fy
Z = depth
```

`depth=0`은 물체가 0m 앞에 있다는 뜻이 아니라 결측값이다. 그래서 0, 음수, NaN, Infinity는 point cloud에서 제외한다.

## 현재 검증한 것

### 데이터 검증

- `data/external/tum/rgbd_dataset_freiburg1_xyz/rgb`: PNG 798장
- `data/external/tum/rgbd_dataset_freiburg1_xyz/depth`: PNG 798장
- `rgb.txt`: RGB timestamp와 파일명 목록
- `depth.txt`: depth timestamp와 파일명 목록
- RGB-depth timestamp matching 결과: 797쌍

첫 실행 프레임:

- RGB: `rgb/1305031102.175304.png`
- Depth: `depth/1305031102.160407.png`
- timestamp 차이: 0.0148968697초

### 수식 검증

Synthetic 3x3 depth map으로 정답이 손계산 가능한 case를 검증했다.

```text
depth =
1 1 1
1 2 1
1 1 0

K = fx=1, fy=1, cx=1, cy=1
```

검증 결과:

- 유효 depth: 8개
- 결측 depth: 1개
- 중심 픽셀 `(1,1), z=2` -> `(0,0,2)`
- 오른쪽 픽셀 `(2,1), z=1` -> `(1,0,1)`
- `depth=0` 픽셀은 point cloud에서 제외

### 2D -> 3D -> 2D round-trip 검증

아래 sample에서 2D pixel을 3D point로 바꾼 뒤 다시 2D로 projection했다.

| input pixel/depth | 3D point | reprojected pixel | error |
|---|---|---|---|
| `(100, 50, 0.8m)` | `(-0.3381, -0.3180, 0.8)` | `(100, 50)` | 0 px |
| `(320, 240, 1.2m)` | `(0.0032, -0.0355, 1.2)` | `(320, 240)` | 0 px |
| `(500, 350, 2.4m)` | `(0.8416, 0.4400, 2.4)` | `(500, 350)` | 0 px |

이 검증은 "변환 공식과 역투영 공식이 서로 맞는가"를 보는 것이다.

### 실제 데이터 실행 검증

첫 TUM 프레임 실행 결과:

- 해상도: 640x480
- 전체 픽셀: 307,200개
- 유효 depth 픽셀: 229,875개
- 결측 depth 픽셀: 77,325개
- 유효 depth 비율: 74.83%
- 생성 point 수: 57,474개
- depth 범위: 0.903m ~ 3.717m
- 평균 depth: 1.427m

생성 파일:

- `outputs/point_cloud_sample.ply`
- `outputs/point_cloud_sample.csv`
- `outputs/depth_preview.bmp`
- `outputs/projection_preview.bmp`
- `outputs/metrics.json`

### C++/OpenCV/GoogleTest 검증

설치 및 빌드 상태:

- Visual Studio Build Tools 설치 확인: `C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat`
- MSVC 확인: `cl.exe` 버전 19.44
- CMake 확인: `C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
- OpenCV prebuilt 확인: `tools/opencv/opencv/build/x64/vc16/lib/OpenCVConfig.cmake`
- GoogleTest: CMake FetchContent로 다운로드/빌드

검증 결과:

- C++ configure 성공
- C++ build 성공
- GoogleTest 7개 통과, 실패 0개
- C++ 실제 TUM RGB-D demo 실행 성공

C++ 실행 결과:

- RGB 프레임: 798장
- Depth 프레임: 798장
- RGB/depth pair: 797쌍
- 유효 depth 픽셀: 229,875개
- 결측 depth 픽셀: 77,325개
- 생성 point 수: 57,474개
- depth 범위: 0.9026m ~ 3.7174m
- 평균 depth: 1.42718m

C++ 산출물:

- `outputs/cpp/point_cloud_sample_cpp.ply`
- `outputs/cpp/point_cloud_sample_cpp.csv`
- `outputs/cpp/depth_preview_cpp.png`
- `outputs/cpp/projection_preview_cpp.png`
- `outputs/cpp/point_cloud_preview_cpp.png`
- `outputs/cpp/metrics_cpp.json`

## 체크포인트 루프

### Checkpoint 0. 환경 확인

목표:

- Node, CMake, Visual Studio Build Tools, vcpkg 존재 여부 확인

명령:

```powershell
.\scripts\00_check_env.ps1
```

통과 기준:

- JS 검증용 Node 존재
- C++ 검증용 CMake + Visual Studio Build Tools 존재

현재 상태:

- Node는 존재
- MSVC, CMake, OpenCV가 설치되어 있고 C++ 빌드가 통과한 상태

### Checkpoint 1. JS unit test

목표:

- 수식, invalid depth filtering, timestamp association, PLY writer 검증

명령:

```powershell
.\scripts\01_test_js.ps1
```

통과 기준:

- `node --test` 6/6 pass

### Checkpoint 2. 실제 RGB-D demo 실행

목표:

- TUM RGB-D 한 프레임에서 point cloud와 preview 생성

명령:

```powershell
.\scripts\02_run_demo_js.ps1
```

통과 기준:

- `outputs/metrics.json`
- `outputs/point_cloud_sample.ply`
- `outputs/depth_preview.bmp`
- `outputs/projection_preview.bmp`

### Checkpoint 3. 출력 검증

목표:

- 결과 파일 존재, metrics 값 sanity check

명령:

```powershell
.\scripts\03_validate_outputs.ps1
```

통과 기준:

- RGB/depth frame count > 0
- valid depth ratio > 0
- sampled points > 0
- 필수 output 파일 존재

### Checkpoint 4. C++ build + GoogleTest

목표:

- C++/OpenCV/GoogleTest 제출용 코드 실제 빌드

명령:

```powershell
.\scripts\04_build_cpp.ps1
```

현재 예상:

- MSVC, CMake, OpenCV, GoogleTest가 준비되면 빌드와 테스트를 수행한다.
- 현재 상태: 통과.

통과 기준:

- `rgbd_pointcloud_cli.exe` 생성
- `test_rgbd_pointcloud_cpp.exe` 생성
- GoogleTest 7/7 pass

### Checkpoint 5. C++ 실제 RGB-D demo 실행

목표:

- 빌드된 C++ 실행 파일로 TUM RGB-D 데이터를 처리한다.

명령:

```powershell
.\scripts\05_run_demo_cpp.ps1
```

통과 기준:

- C++ stdout에 frame count, valid depth, sampled point 출력
- `outputs/cpp/point_cloud_sample_cpp.ply` 생성
- `outputs/cpp/depth_preview_cpp.png` 생성

### Checkpoint 6. C++ output 검증

목표:

- C++ 산출물 파일이 존재하고 비어 있지 않은지 검증한다.

명령:

```powershell
.\scripts\06_validate_cpp_outputs.ps1
```

통과 기준:

- `CPP_OUTPUT_VALIDATION=PASS`
- PLY header에 vertex count 존재

## 밤샘 자동 루프 기준

밤새 돌릴 때는 아래 순서로만 반복한다.

```text
1. JS test
2. JS demo run
3. JS output validation
4. C++ build + GoogleTest
5. C++ demo run
6. C++ output validation
7. report update
```

금지:

- 이 과제와 무관한 파일 수정
- 위키/system 파일 수정
- 관리자 권한 설치 자동 진행
- 환경변수 영구 수정
- 삭제 자동 진행

완료 기준:

- JS test/demo/validation 통과
- C++ build/GoogleTest 통과
- C++ 실제 RGB-D demo 통과
