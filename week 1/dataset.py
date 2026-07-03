import subprocess
import sys

# 필요한 패키지 자동 설치
subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])

from datasets import load_dataset

# ====== Food101 데이터셋 다운로드 ======
ds = load_dataset("ethz/food101", split="train")

# ====== 기본 정보 확인 ======
print(f"데이터셋 크기: {len(ds)}장")
print(f"클래스 수: {len(ds.features['label'].names)}개")

# ====== 샘플 하나 확인 ======
sample = ds[0]
label_name = ds.features['label'].names[sample['label']]
print(f"\n[첫 번째 샘플]")
print(f"  음식 이름: {label_name}")
print(f"  이미지 크기: {sample['image'].size}")
