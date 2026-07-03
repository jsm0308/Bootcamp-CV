import subprocess
import sys
subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "../requirements.txt"])

import pandas as pd
from datasets import load_dataset

# ====== Food101 데이터셋 다운로드 ======
ds = load_dataset("ethz/food101", split="train")
label_names = ds.features["label"].names

# ====== pandas DataFrame으로 변환 ======
rows = []
for i in range(100):  # 일단 100개만
    sample = ds[i]
    rows.append({
        "index": i,
        "label_id": sample["label"],
        "food": label_names[sample["label"]],
        "width": sample["image"].width,
        "height": sample["image"].height,
    })

df = pd.DataFrame(rows)

# ====== pandas로 데이터 탐색 ======
print("===== head (5) =====")
print(df.head())

print("\n===== info =====")
print(df.info())

print("\n===== 기술통계 =====")
print(df[["width", "height"]].describe())

print(f"\n===== 클래스별 분포 (상위 10) =====")
print(df["food"].value_counts().head(10))
