# CV 부트캠프 2주차 최종 보고서

## 1. 주제

**Unit Test 구성 및 2D -> 3D 변환 알고리즘 설계**

이번 과제는 단순히 이미지를 보기 좋게 3D처럼 만드는 것이 아니라, RGB-D 데이터의 depth map을 이용해 실제 거리 단위의 point cloud를 생성하는 것을 목표로 한다. 핵심은 픽셀 좌표 `(u, v)`와 depth `Z`, 카메라 내부 파라미터 `(fx, fy, cx, cy)`가 주어졌을 때 3D 좌표 `(X, Y, Z)`를 정확하게 계산하고, 그 계산을 unit test로 검증하는 것이다.

## 2. 사용 데이터

사용 데이터는 **TUM RGB-D Dataset - freiburg1_xyz**이다.

- 공식 데이터 페이지: https://cvg.cit.tum.de/data/datasets/rgbd-dataset/download
- 파일 형식 설명: https://cvg.cit.tum.de/data/datasets/rgbd-dataset/file_formats
- RGB 프레임 수: 798장
- Depth 프레임 수: 798장
- RGB/depth association 결과: 797쌍
- 해상도: 640x480
- 압축 파일 크기: 약 448MB
- Depth 형식: 16-bit grayscale PNG
- Depth 변환: `depth_m = raw_depth / 5000`

이 데이터는 RGB와 depth가 같이 제공되므로, 단일 RGB 사진에서 임의로 높이를 만드는 pseudo 3D가 아니라 metric 3D 변환을 구현할 수 있다.

## 3. 알고리즘 설계

카메라 모델은 pinhole camera model을 사용했다.

```text
X = (u - cx) * Z / fx
Y = (v - cy) * Z / fy
Z = depth
```

여기서 `(u, v)`는 이미지 픽셀 좌표, `Z`는 depth meter 값, `(fx, fy, cx, cy)`는 카메라 내부 파라미터이다. TUM Freiburg1 기본값으로 `fx=517.3`, `fy=516.5`, `cx=318.6`, `cy=255.3`을 사용했다.

Depth 값이 `0`인 픽셀은 물체가 0m 앞에 있다는 뜻이 아니라 결측값이다. 그래서 `0`, 음수, NaN, Infinity는 모두 제외한다. 이 처리를 하지 않으면 잘못된 점들이 카메라 원점 주변에 몰려 point cloud가 망가진다.

## 4. Unit Test 구성

테스트는 실행 가능한 형태로 구성했다.

- 3x3 synthetic depth map golden test
- depth=0 결측값 제거 테스트
- 음수/NaN/Infinity 제거 테스트
- 2D -> 3D -> 2D 재투영 round-trip 테스트
- 잘못된 카메라 focal length 검증 테스트
- TUM RGB/depth timestamp association 테스트
- PLY export vertex count 테스트
- point cloud 시각화 파일 생성 테스트

Golden test 예시는 다음과 같다.

```text
depth =
1 1 1
1 2 1
1 1 0

K: fx=1, fy=1, cx=1, cy=1
```

기대 결과는 중심 픽셀 `(1,1), z=2`가 `(0,0,2)`가 되고, 오른쪽 픽셀 `(2,1), z=1`은 `(1,0,1)`이 되는 것이다. 마지막 `depth=0` 픽셀은 제외되어 유효 point 수는 8개가 된다.

## 5. 구현 산출물

구현 파일은 작게 유지했다.

- `src/rgbd_pointcloud.cpp`: OpenCV 기반 제출용 구현
- `tests/test_rgbd_pointcloud.cpp`: GoogleTest 제출용 unit test
- `CMakeLists.txt`: CMake 빌드 설정
- `src/rgbd_pointcloud.js`: RGB-D 로딩, depth 변환, point cloud 생성, PLY/CSV/시각화 export
- `tests/test_rgbd_pointcloud.js`: unit test
- `README.md`: 실행 방법
- `docs/REPORT.md`: 중간 보고서
- `outputs/point_cloud_sample.ply`: point cloud 결과
- `outputs/depth_preview.bmp`: depth 시각화
- `outputs/projection_preview.bmp`: 3D point를 다시 이미지 평면에 투영한 검증 결과
- `outputs/depth_preview.pgm`: depth 시각화 원본 포맷
- `outputs/projection_preview.ppm`: projection 원본 포맷
- `outputs/metrics.json`: 데이터 품질 및 실행 지표

처음에는 현재 PC에 C++ 컴파일러, CMake, GoogleTest 실행 경로가 잡혀 있지 않아 JS 구현으로 먼저 검증했다. 이후 Visual Studio Build Tools, CMake, OpenCV prebuilt, GoogleTest FetchContent 구성을 완료하여 C++/OpenCV/GoogleTest 기준으로도 로컬 빌드와 테스트를 수행했다.

실제 실행 결과는 다음과 같다.

- C++ GoogleTest: 7개 통과, 실패 0개
- 선택 프레임: `rgb/1305031102.175304.png`, `depth/1305031102.160407.png`
- RGB-depth timestamp 차이: 약 0.0149초
- 전체 픽셀: 307,200개
- 유효 depth 픽셀: 229,875개
- 결측 depth 픽셀: 77,325개
- 유효 depth 비율: 약 74.83%
- 샘플링된 point 수: 57,474개
- depth 범위: 약 0.903m ~ 3.717m
- 평균 depth: 약 1.427m
- 최대 2D -> 3D -> 2D 재투영 오차: 0.00001907px

C++ 실행 산출물은 다음과 같다.

- `outputs/cpp/point_cloud_sample_cpp.ply`
- `outputs/cpp/point_cloud_sample_cpp.csv`
- `outputs/cpp/depth_preview_cpp.png`
- `outputs/cpp/projection_preview_cpp.png`
- `outputs/cpp/point_cloud_preview_cpp.png`
- `outputs/cpp/metrics_cpp.json`

GoogleTest는 순수 계산 함수와 입출력 결과를 직접 검증한다. gMock은 외부 장치, 네트워크, 서비스처럼 대체해야 할 협력 객체가 현재 구조에 없어서 사용하지 않았다. 불필요한 mock 계층을 추가하기보다 답을 미리 아는 synthetic golden case와 실제 산출물 검증을 사용했다.

## 6. 평가 기준

이번 과제의 평가는 다음 지표로 본다.

- Unit test pass rate: 100%
- Golden 3x3 좌표 오차: 0에 가까워야 함
- 2D -> 3D -> 2D round-trip 오차: 매우 작아야 함
- 유효 depth filtering: `depth=0` 및 비정상 값 제외
- 실제 TUM 프레임에서 point cloud 파일 생성
- depth preview와 projection preview로 결과 확인

## 7. 한계와 확장

현재 구현은 한 장의 RGB-D 프레임에서 point cloud를 만든다. 따라서 여러 각도에서 본 물체를 하나의 정밀한 3D 모델로 합치는 수준은 아니다. 다음 단계로는 여러 프레임의 camera pose를 이용해 point cloud를 누적하고, 다른 시점 이미지로 재투영해 geometry consistency를 평가할 수 있다.

Physical AI 관점에서는 이 과제가 로봇의 perception pipeline 중 **depth 기반 공간 인식**의 가장 작은 단위가 된다. 이후에는 물체 거리 추정, grasp 후보 영역 추정, 충돌 위험 영역 제거, 로봇 손/그리퍼 주변의 3D occupancy 확인으로 확장할 수 있다.

## 8. PPT 구성안

1. 문제 정의: 왜 단일 RGB가 아니라 RGB-D가 필요한가
2. 알고리즘: pinhole camera model과 depth filtering
3. 테스트: synthetic golden case와 round-trip 검증
4. 결과: TUM 데이터 point cloud, depth preview, projection preview
5. 확장: multi-view fusion, grasp perception, Physical AI 연결
