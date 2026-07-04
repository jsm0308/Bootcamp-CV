import cv2
import numpy as np


def detect_red_pixels(input_path, output_path):
    image = cv2.imread(input_path)
    if image is None:
        raise FileNotFoundError(f"이미지를 불러올 수 없습니다: {input_path}")

    # ponytail: HSV 공간에서 빨간색은 Hue 0°(≈360°) 경계에 걸려 있어
    # 범위를 두 조각(0~10 + 170~180)으로 나눠야 전체 빨간색을 잡을 수 있다
    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

    lower_red_near_0 = np.array([0, 120, 70])
    upper_red_near_0 = np.array([10, 255, 255])
    lower_red_near_180 = np.array([170, 120, 70])
    upper_red_near_180 = np.array([180, 255, 255])

    mask_low = cv2.inRange(hsv, lower_red_near_0, upper_red_near_0)
    mask_high = cv2.inRange(hsv, lower_red_near_180, upper_red_near_180)
    red_mask = cv2.bitwise_or(mask_low, mask_high)

    # ponytail: bitwise_and는 mask가 1인 픽셀만 통과시키고 나머지는 0(검정)으로 만든다
    red_only = cv2.bitwise_and(image, image, mask=red_mask)

    cv2.imwrite(output_path, red_only)
    print(f"결과 저장 완료: {output_path}")


if __name__ == "__main__":
    detect_red_pixels("sample.jpg", "red_filtered.jpg")
