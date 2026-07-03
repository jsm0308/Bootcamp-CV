import subprocess
import sys
import os
import cv2
import numpy as np
from datasets import load_dataset

subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "../requirements.txt"])

# ============================================================
#  1. 데이터 로드
# ============================================================
ds = load_dataset("ethz/food101", split="train")
label_names = ds.features["label"].names

# ============================================================
#  2. 전처리 함수
# ============================================================

def resize_224(image):
    """이미지를 224×224로 크기 조정"""
    return cv2.resize(image, (224, 224))

def to_grayscale_and_normalize(image):
    """Grayscale 변환 + [0, 1] 정규화"""
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    return gray / 255.0

def remove_noise(image):
    """GaussianBlur로 노이즈 제거"""
    return cv2.GaussianBlur(image, (5, 5), 0)

def horizontal_flip(image):
    """좌우 반전"""
    return cv2.flip(image, 1)

def rotate_15(image):
    """15도 회전"""
    h, w = image.shape[:2]
    M = cv2.getRotationMatrix2D((w / 2, h / 2), 15, 1.0)
    return cv2.warpAffine(image, M, (w, h))

def brightness_change(image, delta=30):
    """밝기 변화 (±delta)"""
    bright = image.astype(np.int16) + delta
    return np.clip(bright, 0, 255).astype(np.uint8)

# ============================================================
#  3. 이상치 탐지 (Outlier Detection)
# ============================================================

def is_too_dark(image, threshold=30):
    """평균 밝기가 threshold 미만이면 True"""
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    return np.mean(gray) < threshold

def is_object_too_small(image, min_area_ratio=0.1):
    """가장 큰 객체가 전체 면적의 min_area_ratio 미만이면 True"""
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    _, binary = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        return True
    h, w = image.shape[:2]
    max_area = max(cv2.contourArea(c) for c in contours)
    return max_area < (h * w * min_area_ratio)

def check_outlier(image):
    """이미지가 이상치면 이유를 반환, 아니면 None"""
    if is_too_dark(image):
        return "너무 어두움"
    if is_object_too_small(image):
        return "객체가 너무 작음"
    return None

# ============================================================
#  4. 전체 전처리 파이프라인
# ============================================================

def full_pipeline(bgr_image):
    """
    하나의 이미지에 대해 모든 전처리 + 증강을 수행.
    반환값: {
        "preprocessed":  최종 전처리 이미지 (grayscale, 224x224, blurred, normalized),
        "augmented":     [좌우반전, 회전, 밝기변화] 이미지 리스트
    }
    """
    # Step 1: 크기 조정
    resized = resize_224(bgr_image)

    # Step 2~3: Grayscale + Normalize + Blur (순서대로)
    processed = to_grayscale_and_normalize(resized)      # float [0,1]
    processed = remove_noise(processed)                   # grayscale에 blur

    # Step 4: 증강 (원본 BGR 기준으로 적용 후, 동일 전처리 적용)
    flipped   = to_grayscale_and_normalize(remove_noise(resize_224(horizontal_flip(resized))))
    rotated   = to_grayscale_and_normalize(remove_noise(resize_224(rotate_15(resized))))
    brighter  = to_grayscale_and_normalize(remove_noise(resize_224(brightness_change(resized))))

    return {
        "preprocessed": processed,
        "augmented": [flipped, rotated, brighter]
    }

# ============================================================
#  5. 메인 실행
# ============================================================
if __name__ == "__main__":
    output_dir = "preprocessed_samples"
    os.makedirs(output_dir, exist_ok=True)

    saved = 0
    idx = 0

    while saved < 5 and idx < len(ds):
        sample = ds[idx]
        pil_image = sample["image"]
        label = label_names[sample["label"]]

        # PIL → OpenCV BGR
        bgr = cv2.cvtColor(np.array(pil_image), cv2.COLOR_RGB2BGR)

        # 이상치 검사
        reason = check_outlier(bgr)
        if reason:
            print(f"[SKIP #{idx}] {label} — {reason}")
            idx += 1
            continue

        # 전처리 파이프라인 실행
        result = full_pipeline(bgr)

        # 원본 + 증강본 저장
        base = f"{saved + 1:02d}_{label}"

        # 전처리된 원본 저장 (grayscale → 0~255로 변환해서 저장)
        cv2.imwrite(f"{output_dir}/{base}_preprocessed.jpg",
                    (result["preprocessed"] * 255).astype(np.uint8))

        # 증강본 저장
        aug_names = ["flip", "rotate", "bright"]
        for aug_img, aug_name in zip(result["augmented"], aug_names):
            cv2.imwrite(f"{output_dir}/{base}_{aug_name}.jpg",
                        (aug_img * 255).astype(np.uint8))

        print(f"[SAVED #{saved + 1}] {label} → {output_dir}/")
        saved += 1
        idx += 1

    print(f"\n완료: {saved}개의 이미지가 '{output_dir}/'에 저장되었습니다.")
