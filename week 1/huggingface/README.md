# Bootcamp-CV

코멘토 직무부트캠프 Computer Vision — 베어곰 멘토

**기간**: 2026년 7월, 5주 과정

**구조**: ZOOM 실시간 세션 3회 + 과제 4회

---

## 과제

| 차수 | 주제 | 핵심 기술 |
|---|---|---|
| 1차 | Git + OpenCV 픽셀 처리 | Git Flow, HSV 필터링, HuggingFace 전처리 |
| 2차 | Unit Test + 2D→3D 변환 | pytest, Depth Map, Point Cloud |
| 3차 | YOLOv8 모델링 + 시각화 | Ultralytics, 객체 탐지, Matplotlib |
| 4차 | 최종 프로젝트 | AI/CV 기반 제품 개발 |

---

## 1차 과제 진행 현황

### 핵심 요청

| 항목 | 상태 |
|---|---|
| GitHub 저장소 생성 및 로컬 연동 | ✅ 완료 |
| `feature/image-processing` 브랜치 생성 및 작업 | ✅ 완료 |
| 빨간색 픽셀 감지 및 필터링 코드 (`red.py`) | ✅ 완료 |
| `git commit`, `git push`, PR 생성, `main` 병합 | ✅ 완료 |
| HuggingFace Food101 데이터셋 로드 (`dataset.py`) | ✅ 완료 |
| 전처리 파이프라인 (`image_preprocessing.py`) | ✅ 완료 |
| 전처리 결과물 5장 저장 (`preprocessed_samples/`) | ✅ 완료 |
| PPT 4페이지 이내 결과 요약 | ❌ 미완료 |

### 추가 요청 (전처리 상세)

| 단계 | 항목 | 상태 |
|---|---|---|
| 크기 조정 | 224×224 resize | ✅ |
| 색상 변환 | Grayscale + [0,1] Normalize | ✅ |
| 노이즈 제거 | GaussianBlur (5×5) | ✅ |
| 데이터 증강 | 좌우반전, 15도 회전, 밝기 ±30 | ✅ |
| 이상치 탐지 | 평균 밝기 < 30 제거 | ✅ |
| 이상치 탐지 | 객체 면적 < 10% 제거 (OTSU threshold 기반) | ✅ |

### 프로젝트 구조

```
week 1/
├── requirements.txt          ← 공통 패키지 의존성
├── git/                      ← 핵심 과제: OpenCV 픽셀 처리
│   ├── red.py                # HSV 빨간색 검출
│   ├── sample.jpg            # 테스트 이미지
│   └── red_filtered.jpg      # 결과물
└── huggingface/              ← 추가 과제: 데이터셋 전처리
    ├── dataset.py             # Food101 로드 + pandas 탐색
    ├── image_preprocessing.py # 전처리 파이프라인
    ├── save_raw.py            # 원본 이미지 5장 저장
    ├── data/                  # 원본 Food101 이미지 5장
    └── preprocessed_samples/  # 전처리 + 증강 결과물 20장
```

### 제출 항목

| 파일 | 역할 |
|---|---|
| `git/red.py` | 빨간색 HSV 검출 → `red_filtered.jpg` 저장 |
| `huggingface/dataset.py` | HuggingFace `ethz/food101` 데이터셋 로드 + pandas 분석 |
| `huggingface/image_preprocessing.py` | 전체 전처리 파이프라인 (resize → grayscale → blur → augment → outlier) |
| `huggingface/save_raw.py` | Food101 원본 이미지 5장 저장 |
| `huggingface/preprocessed_samples/` | 전처리 + 증강된 이미지 20장 (5개 샘플 × 4종) |
| `huggingface/data/` | 원본 Food101 이미지 5장 |
| `requirements.txt` | 패키지 의존성 (datasets, opencv-python, numpy, pandas) |

---

## 참고 자료

| 주제 | 링크 |
|---|---|
| Git 기본 사용법 | [Pro Git Book](https://git-scm.com/book/ko/v2) |
| OpenCV 공식 문서 | [OpenCV Docs](https://docs.opencv.org/) |
| YOLO 객체 탐지 | [Ultralytics YOLO Docs](https://docs.ultralytics.com/) |
| 데이터 증강 | [Albumentations](https://albumentations.ai/) |
| pytest 공식 문서 | [pytest Docs](https://docs.pytest.org/) |
| Depth Map 생성 | [OpenCV Depth Estimation](https://docs.opencv.org/master/dd/d53/tutorial_py_depthmap.html) |
| Point Cloud (Open3D) | [Open3D Docs](https://www.open3d.org/) |
| Stereo Vision | [Stereo Vision with OpenCV](https://docs.opencv.org/master/dd/d53/tutorial_py_depthmap.html) |
| Torchvision Object Detection | [Torchvision Docs](https://pytorch.org/vision/stable/models.html) |
| HuggingFace ViT | [Hugging Face ViT](https://huggingface.co/google/vit-base-patch16-224) |
| Git Flow | [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/) |
| AI Project Planning | [Google ML Rules](https://developers.google.com/machine-learning/guides/rules-of-ml) |
| FastAPI | [FastAPI Docs](https://fastapi.tiangolo.com/) |
| Python 패키징 가이드 | [PyPA Packaging Guide](https://packaging.python.org/) |

---

## 자료

- `1주차 직무에센스 강의자료_베어곰_최종.pdf` — 강의 슬라이드 30장
- `차시별 업무요청서_베어곰_최종.pdf` — 4개 과제 업무요청서 23장
