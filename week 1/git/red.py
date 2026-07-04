import cv2
import numpy as np

# 이미지 로드
image = cv2.imread('sample.jpg')
if image is None:
    print("오류: 'sample.jpg'를 읽을 수 없습니다.")
    exit()

# BGR에서 HSV 색상 공간으로 변환
hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

# 빨간색 범위 지정 (두 개의 범위를 설정해야 함)
lower_red1 = np.array([0, 120, 70])
upper_red1 = np.array([10, 255, 255])
lower_red2 = np.array([170, 120, 70])
upper_red2 = np.array([180, 255, 255])

# 마스크 생성
mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
mask = cv2.bitwise_or(mask1, mask2)  # 두 개의 마스크를 합침

# 원본 이미지에서 빨간색 부분만 추출
result = cv2.bitwise_and(image, image, mask=mask)

# 결과 이미지 저장
cv2.imwrite('red_filtered.jpg', result)
print("결과 저장 완료: red_filtered.jpg")
print("끝")
