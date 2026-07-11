# Week 2: Tested RGB-D to Point Cloud Converter

이 과제는 RGB-D 데이터의 depth map을 카메라 내부 파라미터와 결합해 metric 3D point cloud로 변환하고, 핵심 픽셀 처리 로직을 unit test로 검증한다.

## 데이터

사용 데이터는 TUM RGB-D Dataset `freiburg1_xyz`이다.

- RGB 프레임: 798장
- Depth 프레임: 798장
- 해상도: 640x480
- Depth 형식: 16-bit grayscale PNG, `depth_m = raw / 5000`
- 선택 이유: RGB와 depth가 이미 정렬된 Kinect 데이터라 2D -> 3D 변환 실습에 적합하다.

## 실행 및 검증

```powershell
.\scripts\04_build_cpp.ps1
.\scripts\05_run_demo_cpp.ps1
.\scripts\06_validate_cpp_outputs.ps1
```

첫 번째 스크립트가 C++ 빌드와 GoogleTest 실행을 함께 수행한다. 두 번째는 실제 TUM 프레임을 변환하고, 세 번째는 파일 존재 여부와 수치 범위를 검증한다.

## 산출물

- `outputs/cpp/point_cloud_sample_cpp.ply`: CloudCompare, MeshLab 등에서 열 수 있는 3D point cloud
- `outputs/cpp/point_cloud_sample_cpp.csv`: XYZRGB와 원본 픽셀 좌표 확인용 CSV
- `outputs/cpp/depth_preview_cpp.png`: depth map 컬러 시각화
- `outputs/cpp/projection_preview_cpp.png`: 3D point를 2D로 재투영한 검증 이미지
- `outputs/cpp/point_cloud_preview_cpp.png`: 정면(X-Y), 상면(X-Z) point cloud 시각화
- `outputs/cpp/metrics_cpp.json`: 데이터, depth, sampling, 재투영 오차 지표
- `SUBMISSION_WEEK2_RGBD_POINTCLOUD.pptx`: 제출용 5장 PPT

## 현재 검증 상태

- MSVC/CMake 설치 확인 완료
- OpenCV 4.12.0 prebuilt 연결 완료
- GoogleTest FetchContent 빌드 완료
- C++ build 성공
- GoogleTest 7/7 통과
- C++ 실제 TUM RGB-D demo 실행 성공
- C++ 산출물 검증 통과: `CPP_OUTPUT_VALIDATION=PASS`
- 유효 depth 비율: 74.83%
- point cloud: 57,474 points
- 최대 2D -> 3D -> 2D 재투영 오차: 0.00001907 px

GoogleTest는 계산 결과, 예외 처리, timestamp association, 파일 출력, 시각화 생성을 검증한다. gMock은 외부 장치·네트워크·서비스처럼 대체할 협력 객체가 없는 순수 계산 중심 구조이므로 사용하지 않았다.

JS 파일은 C++ 환경 설치 전 알고리즘을 교차 검증한 초기 구현이다. 최종 제출 기준 구현은 C++/OpenCV/GoogleTest이다.

## 핵심 공식

```text
X = (u - cx) * Z / fx
Y = (v - cy) * Z / fy
Z = depth
```

`depth=0`, 음수, NaN, Infinity는 실제 물체가 아니라 결측값이므로 point cloud에서 제외한다.
