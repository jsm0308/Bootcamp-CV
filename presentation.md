---
marp: true
theme: gaia
size: 16:9
paginate: true
backgroundColor: #fff
color: #333
---

<!-- _class: lead -->

# 1차 과제 결과 보고서
## Git 기반 협업 & 픽셀 단위 이미지 처리

**코멘토 직무부트캠프 Computer Vision**
베어곰 멘토 | 2026.07

---

<!-- _class: lead -->

# Part 1
## Git 브랜치 전략 & 코드 관리

---

# Git 워크플로우

- **저장소**: `jsm0308/Bootcamp-CV`
- **브랜치 전략**: `feature/image-processing` → PR → `main` 병합
- **모든 변경사항을 feature 브랜치에서 작업 후 PR로 리뷰**

### 사용한 Git 명령어

```
git branch feature/image-processing
git checkout feature/image-processing
git add <파일>
git commit -m "메시지"
git push origin feature/image-processing
```

### 결과

- PR 생성 → `main` 브랜치에 Fast-forward 병합 완료
- 실습용 `feature/final-cleanup` 브랜치 추가 생성

---

# 폴더 구조

```
week 1/
├── requirements.txt          ← 공통 패키지
├── git/                      ← 핵심 과제
│   ├── red.py                # 빨간색 검출
│   ├── sample.jpg            # 테스트 이미지
│   └── red_filtered.jpg      # 결과물
└── huggingface/              ← 추가 과제
    ├── dataset.py             # 데이터 로드 + 분석
    ├── image_preprocessing.py # 전처리 파이프라인
    ├── data/                  # 원본 샘플
    └── preprocessed_samples/  # 결과물
```

---

<!-- _class: lead -->

# Part 2
## OpenCV 픽셀 처리
### 빨간색(Red) 영역 검출

---

# 빨간색 검출: 접근 방식

### 왜 HSV인가?

RGB는 조명 변화에 민감하다.  
HSV는 **색상(Hue)**, 채도(Saturation), 명도(Value)를 분리해서  
조명 변화에 강한 색상 검출이 가능하다.

### 빨간색의 특수성

HSV에서 빨간색은 0°에서 180°로 원형 순환한다.  
따라서 **두 개의 범위**를 합쳐야 한다:

| 범위 | H (색상) | S (채도) | V (명도) |
|---|---|---|---|
| Lower Red 1 | 0 | 120 | 70 |
| Upper Red 1 | 10 | 255 | 255 |
| Lower Red 2 | 170 | 120 | 70 |
| Upper Red 2 | 180 | 255 | 255 |

---

# 빨간색 검출: 코드 (red.py)

```python
import cv2, numpy as np
image = cv2.imread('sample.jpg')

hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

lower_red1 = np.array([0,   120, 70])
upper_red1 = np.array([10,  255, 255])
lower_red2 = np.array([170, 120, 70])
upper_red2 = np.array([180, 255, 255])

mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
mask  = cv2.bitwise_or(mask1, mask2)

result = cv2.bitwise_and(image, image, mask=mask)
cv2.imwrite('red_filtered.jpg', result)
```

*`mask1 + mask2` 대신 `cv2.bitwise_or`를 사용해 8비트 오버플로우 방지*

---

# 빨간색 검출: 결과

| 원본 | 검출 결과 |
|---|---|
| ![](week%201/git/sample.jpg) | ![](week%201/git/red_filtered.jpg) |

좌측 원본 이미지에서 빨간색 영역만 우측처럼 추출된다.
초록색, 파란색 영역은 마스킹 되어 검은색으로 표시된다.

---

<!-- _class: lead -->

# Part 3
## HuggingFace 데이터셋
### Food101 로드 & pandas 분석

---

# 데이터셋 로드 (dataset.py)

### HuggingFace `ethz/food101`

```python
from datasets import load_dataset
import pandas as pd

ds = load_dataset("ethz/food101", split="train")
```

| 항목 | 값 |
|---|---|
| 전체 데이터 | 75,750장 |
| 클래스 수 | 101종 |
| 이미지 크기 | 평균 502 × 476 px |
| 클래스 예시 | beignets, apple_pie, sushi, bibimbap... |

### pandas로 변환한 이유

`df.head()`, `df.describe()`, `value_counts()`로 즉시 데이터 분포 파악 가능.
실무에서 EDA(탐색적 데이터 분석)의 표준 방식.

---

<!-- _class: lead -->

# Part 4
## 이미지 전처리 파이프라인
### Resize → Grayscale → Blur → Augmentation

---

# 전처리 파이프라인 (image_preprocessing.py)

```
원본 이미지 (BGR, any size)
    │
    ├─ 1. resize_224()      → 224 × 224
    ├─ 2. Grayscale 변환    → 1채널
    ├─ 3. [0, 1] Normalize  → 정규화
    ├─ 4. GaussianBlur(5×5) → 노이즈 제거
    │
    └─ 5. 증강 (각각 개별 저장)
         ├─ horizontal_flip  → 좌우 반전
         ├─ rotate_15°       → 15도 회전
         └─ brightness ±30   → 밝기 변화
```

각 단계는 독립 함수로 모듈화.  
`full_pipeline()`이 이들을 순서대로 호출한다.

---

# 전처리 함수 상세

| 함수 | 역할 | OpenCV API |
|---|---|---|
| `resize_224()` | 224×224 크기 조정 | `cv2.resize` |
| `to_grayscale_and_normalize()` | 흑백 + 정규화 | `cv2.cvtColor`, `/255.0` |
| `remove_noise()` | 가우시안 블러 | `cv2.GaussianBlur` |
| `horizontal_flip()` | 좌우 반전 | `cv2.flip` |
| `rotate_15()` | 15도 회전 | `cv2.getRotationMatrix2D`, `cv2.warpAffine` |
| `brightness_change()` | 밝기 ±30 | NumPy clip |

**의존성: OpenCV + NumPy만 사용**  
(추가 라이브러리 없이 기본 함수로 구현)

---

# 이상치 탐지 (Outlier Detection)

### 1. 너무 어두운 이미지 제거
```python
def is_too_dark(image, threshold=30):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    return np.mean(gray) < threshold
```
평균 픽셀 밝기가 30 미만 → 제외

### 2. 객체가 너무 작은 이미지 제거
```python
def is_object_too_small(image, min_area_ratio=0.1):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    _, binary = cv2.threshold(gray, 0, 255, cv2.THRESH_OTSU)
    contours, _ = cv2.findContours(binary, ...)
    max_contour / (h × w) < 0.1 → 제외
```
OTSU 임계값 → 가장 큰 윤곽선 면적이 전체의 10% 미만 → 제외

---

# 전처리 결과 샘플

| 원본 | 전처리 | Flip | Rotate | Bright |
|---|---|---|---|---|
| ![](week%201/huggingface/data/01_beignets_raw.jpg) | ![](week%201/huggingface/preprocessed_samples/01_beignets_preprocessed.jpg) | ![](week%201/huggingface/preprocessed_samples/01_beignets_flip.jpg) | ![](week%201/huggingface/preprocessed_samples/01_beignets_rotate.jpg) | ![](week%201/huggingface/preprocessed_samples/01_beignets_bright.jpg) |

5개 샘플 × 4종 = **20장**의 전처리 완료 이미지 생성

---

<!-- _class: lead -->

# 결과물 체크리스트

---

# 제출 항목 요약

| 항목 | 상태 | 파일 경로 |
|---|---|---|
| Git 저장소 구성 | ✅ | `https://github.com/jsm0308/Bootcamp-CV` |
| Branch / PR / Merge | ✅ | `feature/image-processing` |
| 빨간색 검출 코드 | ✅ | `week 1/git/red.py` |
| Food101 데이터 로드 | ✅ | `week 1/huggingface/dataset.py` |
| 전처리 파이프라인 | ✅ | `week 1/huggingface/image_preprocessing.py` |
| 크기 조정 (224×224) | ✅ | `resize_224()` |
| Grayscale + Normalize | ✅ | `to_grayscale_and_normalize()` |
| Blur 노이즈 제거 | ✅ | `remove_noise()` |
| 데이터 증강 (3종) | ✅ | flip, rotate, brightness |
| 이상치 탐지 (2종) | ✅ | dark, small object |
| 전처리 결과물 (20장) | ✅ | `preprocessed_samples/` |
| PPT 보고서 | ✅ | `presentation.pptx` |

---

# 배운 점

1. **HSV가 RGB보다 실용적이다**  
   조명 변화에 강하고, 사람이 생각하는 "색상"에 가깝다.

2. **`cv2.bitwise_or` vs `+` 연산자**  
   uint8에서 255+255=510으로 오버플로우. 비트 연산이 정확하다.

3. **pandas로 EDA 먼저, 전처리는 그 다음**  
   데이터 분포(크기, 클래스)를 먼저 파악하고 전처리 전략을 세운다.

4. **Git 브랜치 전략은 혼자여도 의미 있다**  
   실수 복구가 쉽고, 작업 단위가 명확해진다.

5. **OpenCV + NumPy만으로도 강력한 파이프라인**  
   추가 라이브러리 의존 없이 기본 함수 조합으로 충분하다.
