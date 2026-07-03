import subprocess
import sys
subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "../requirements.txt"])

import os
import cv2
import numpy as np
from datasets import load_dataset

ds = load_dataset("ethz/food101", split="train")
label_names = ds.features["label"].names
output_dir = "data"

for i in range(5):
    sample = ds[i]
    label = label_names[sample["label"]]
    bgr = cv2.cvtColor(np.array(sample["image"]), cv2.COLOR_RGB2BGR)
    cv2.imwrite(f"{output_dir}/{i+1:02d}_{label}_raw.jpg", bgr)

print(f"원본 이미지 5장이 '{output_dir}/'에 저장되었습니다.")
