"""
1차 과제 PPT 생성기 — python-pptx (모든 텍스트 편집 가능)
"""
from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN
import os

prs = Presentation()
prs.slide_width = Inches(13.333)
prs.slide_height = Inches(7.5)

def add_slide(prs):
    return prs.slides.add_slide(prs.slide_layouts[6])

def add_title(slide, text, top=0.5, color="333333"):
    txBox = slide.shapes.add_textbox(Inches(1), Inches(top), Inches(11), Inches(1.2))
    tf = txBox.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = text
    p.font.size = Pt(40)
    p.font.bold = True
    p.font.color.rgb = RGBColor(0x33, 0x33, 0x33)
    return tf

def add_subtitle(slide, text, top=1.8):
    txBox = slide.shapes.add_textbox(Inches(1), Inches(top), Inches(11), Inches(0.8))
    tf = txBox.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = text
    p.font.size = Pt(22)
    p.font.color.rgb = RGBColor(0x66, 0x66, 0x66)
    return tf

def add_textbox(slide, left, top, width, height):
    txBox = slide.shapes.add_textbox(Inches(left), Inches(top), Inches(width), Inches(height))
    txBox.text_frame.word_wrap = True
    return txBox.text_frame

def add_para(tf, text, size=20, bold=False, color="333333", align=None):
    p = tf.add_paragraph()
    p.text = text
    p.font.size = Pt(size)
    p.font.bold = bold
    r, g, b = int(color[0:2], 16), int(color[2:4], 16), int(color[4:6], 16)
    p.font.color.rgb = RGBColor(r, g, b)
    if align:
        p.alignment = align
    return p

def add_image(slide, path, left, top, width=None, height=None):
    if not os.path.exists(path):
        print(f"  [WARN] image not found: {path}")
        return None
    if width:
        return slide.shapes.add_picture(path, Inches(left), Inches(top), Inches(width))
    if height:
        return slide.shapes.add_picture(path, Inches(left), Inches(top), height=Inches(height))
    return slide.shapes.add_picture(path, Inches(left), Inches(top))

def add_bg(slide, color):
    fill = slide.background.fill
    fill.solid()
    r, g, b = int(color[0:2], 16), int(color[2:4], 16), int(color[4:6], 16)
    fill.fore_color.rgb = RGBColor(r, g, b)

# ============================================================
slide = add_slide(prs)
add_bg(slide, "2c3e50")
tf = add_textbox(slide, 1, 1.5, 11, 3)
add_para(tf, "1차 과제 결과 보고서", size=44, bold=True, color="FFFFFF")
add_para(tf, "Git 기반 협업 & 픽셀 단위 이미지 처리", size=26, color="BBBBBB")
add_para(tf, "", size=12)
add_para(tf, "코멘토 직무부트캠프 Computer Vision", size=18, color="999999")
add_para(tf, "베어곰 멘토  |  2026.07", size=18, color="999999")

# ---- Part 1 section ----
slide = add_slide(prs)
add_bg(slide, "34495e")
tf = add_textbox(slide, 1, 2.5, 11, 2)
add_para(tf, "Part 1. Git 브랜치 전략 & 코드 관리", size=38, bold=True, color="FFFFFF")
add_para(tf, "Git Flow · Branch · PR · Merge", size=22, color="BBBBBB")

# ---- Git Workflow ----
slide = add_slide(prs)
add_title(slide, "Git 워크플로우")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
add_para(tf, "[저장소] jsm0308/Bootcamp-CV", size=20)
add_para(tf, "[브랜치] feature/image-processing -> PR -> main 병합", size=20)
add_para(tf, "", size=10)
add_para(tf, "사용한 명령어", size=22, bold=True)
for cmd in [
    "git branch feature/image-processing",
    "git checkout feature/image-processing",
    "git add <file>",
    'git commit -m "message"',
    "git push origin feature/image-processing",
]:
    add_para(tf, f"  {cmd}", size=18, color="555555")
add_para(tf, "", size=10)
add_para(tf, "[결과] PR 생성 -> Fast-forward merge 완료", size=20, color="27ae60")

# ---- Folder Structure ----
slide = add_slide(prs)
add_title(slide, "프로젝트 구조")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
add_para(tf, "week 1/", size=20, bold=True)
for line, b in [
    ("├── requirements.txt          <- 공통 패키지", False),
    ("├── git/                      <- 핵심 과제", True),
    ("│   ├── red.py                 # HSV 빨간색 검출", False),
    ("│   └── red_filtered.jpg       # 결과물", False),
    ("└── huggingface/              <- 추가 과제", True),
    ("    ├── dataset.py             # Food101 + pandas", False),
    ("    ├── image_preprocessing.py # 전처리 파이프라인", False),
    ("    ├── data/                  # 원본 5장", False),
    ("    └── preprocessed_samples/  # 전처리+증강 20장", False),
]:
    add_para(tf, line, size=17 if b else 16, bold=b, color="333333" if b else "555555")

# ---- Part 2 section ----
slide = add_slide(prs)
add_bg(slide, "34495e")
tf = add_textbox(slide, 1, 2.5, 11, 2)
add_para(tf, "Part 2. OpenCV 픽셀 처리", size=38, bold=True, color="FFFFFF")
add_para(tf, "HSV 색상 공간 · 빨간색(Red) 영역 검출", size=22, color="BBBBBB")

# ---- HSV approach ----
slide = add_slide(prs)
add_title(slide, "왜 HSV인가?")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
add_para(tf, "RGB는 조명 변화에 민감. HSV는 Hue/Saturation/Value를 분리하여 조명 변화에 강함.", size=20)
add_para(tf, "", size=8)
add_para(tf, "빨간색은 HSV 0도 와 180도 경계에 걸려 있으므로 두 범위를 합쳐야 함.", size=20, bold=True)
add_para(tf, "", size=8)
add_para(tf, "Lower Red 1: [0, 120, 70]  ->  Upper Red 1: [10, 255, 255]", size=18, color="c0392b")
add_para(tf, "Lower Red 2: [170, 120, 70] -> Upper Red 2: [180, 255, 255]", size=18, color="c0392b")
add_para(tf, "", size=8)
add_para(tf, "mask1 + mask2 대신 cv2.bitwise_or() 사용 (uint8 overflow 방지)", size=18, color="e74c3c")

# ---- Red code ----
slide = add_slide(prs)
add_title(slide, "코드 (red.py)")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
for line in [
    "import cv2, numpy as np",
    "image = cv2.imread('sample.jpg')",
    "hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)",
    "",
    "lower_red1 = np.array([0, 120, 70])",
    "upper_red1 = np.array([10, 255, 255])",
    "lower_red2 = np.array([170, 120, 70])",
    "upper_red2 = np.array([180, 255, 255])",
    "",
    "mask1 = cv2.inRange(hsv, lower_red1, upper_red1)",
    "mask2 = cv2.inRange(hsv, lower_red2, upper_red2)",
    "mask  = cv2.bitwise_or(mask1, mask2)",
    "result = cv2.bitwise_and(image, image, mask=mask)",
    "cv2.imwrite('red_filtered.jpg', result)",
]:
    add_para(tf, line, size=17, color="444444")

# ---- Red result ----
slide = add_slide(prs)
add_title(slide, "결과")
add_para(add_textbox(slide, 2, 2.0, 4, 0.5), "원본", size=18, bold=True, align=PP_ALIGN.CENTER)
add_para(add_textbox(slide, 7.5, 2.0, 4, 0.5), "검출 결과", size=18, bold=True, align=PP_ALIGN.CENTER)
add_image(slide, "week 1/git/sample.jpg", 2, 2.5, width=4)
add_image(slide, "week 1/git/red_filtered.jpg", 7.5, 2.5, width=4)
add_para(add_textbox(slide, 1.5, 6, 10, 0.5), "빨간색만 추출. 초록색/파란색은 마스킹되어 검은색으로 표시.", size=18, color="555555")

# ---- Part 3 section ----
slide = add_slide(prs)
add_bg(slide, "34495e")
tf = add_textbox(slide, 1, 2.5, 11, 2)
add_para(tf, "Part 3. HuggingFace 데이터셋", size=38, bold=True, color="FFFFFF")
add_para(tf, "Food101 로드 · pandas EDA", size=22, color="BBBBBB")

# ---- Food101 ----
slide = add_slide(prs)
add_title(slide, "Food101 데이터셋 (dataset.py)")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
add_para(tf, "from datasets import load_dataset", size=18, color="555555")
add_para(tf, 'ds = load_dataset("ethz/food101", split="train")', size=18, color="555555")
add_para(tf, "", size=8)
add_para(tf, "전체  75,750장", size=20)
add_para(tf, "클래스  101종 (apple_pie, sushi, bibimbap, beignets...)", size=20)
add_para(tf, "이미지 크기  평균 502 x 476 px", size=20)
add_para(tf, "", size=8)
add_para(tf, "100개 샘플 -> pandas DataFrame -> df.head(), df.describe()", size=18, color="555555")
add_para(tf, "실무에서 EDA(탐색적 데이터 분석)의 표준 방식.", size=18, color="555555")

# ---- Part 4 section ----
slide = add_slide(prs)
add_bg(slide, "34495e")
tf = add_textbox(slide, 1, 2.0, 11, 3)
add_para(tf, "Part 4. 이미지 전처리 파이프라인", size=38, bold=True, color="FFFFFF")
add_para(tf, "Resize -> Grayscale -> Blur", size=22, color="BBBBBB")
add_para(tf, "Augmentation -> Outlier Detection", size=22, color="BBBBBB")

# ---- Pipeline ----
slide = add_slide(prs)
add_title(slide, "전처리 파이프라인")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
add_para(tf, "1. resize_224()           -> 224 x 224", size=20)
add_para(tf, "2. Grayscale 변환        -> 1채널", size=20)
add_para(tf, "3. [0, 1] Normalize      -> 정규화", size=20)
add_para(tf, "4. GaussianBlur(5x5)     -> 노이즈 제거", size=20)
add_para(tf, "", size=8)
add_para(tf, "5. 데이터 증강 (각각 저장)", size=20, bold=True, color="e74c3c")
add_para(tf, "   - horizontal_flip       좌우 반전", size=18, color="555555")
add_para(tf, "   - rotate_15             15도 회전", size=18, color="555555")
add_para(tf, "   - brightness +/-30       밝기 변화", size=18, color="555555")
add_para(tf, "", size=8)
add_para(tf, "의존성: OpenCV + NumPy 만 사용", size=22, bold=True, color="27ae60")

# ---- Outlier ----
slide = add_slide(prs)
add_title(slide, "이상치 탐지 (Outlier Detection)")
tf = add_textbox(slide, 1.5, 2.0, 5, 4)
add_para(tf, "1) 너무 어두운 이미지", size=20, bold=True, color="e74c3c")
add_para(tf, "cv2.cvtColor -> GRAY", size=16, color="555555")
add_para(tf, "np.mean(gray) < 30 -> skip", size=16, color="555555")
add_para(tf, "", size=10)
add_para(tf, "2) 객체 너무 작은 이미지", size=20, bold=True, color="e74c3c")
add_para(tf, "cv2.threshold(OTSU) -> binary", size=16, color="555555")
add_para(tf, "cv2.findContours -> max_area", size=16, color="555555")
add_para(tf, "max_area / (h*w) < 0.1 -> skip", size=16, color="555555")

# ---- Samples ----
slide = add_slide(prs)
add_title(slide, "전처리 샘플")
raw_paths = [
    "week 1/huggingface/data/01_beignets_raw.jpg",
    "week 1/huggingface/data/02_beignets_raw.jpg",
]
proc_paths = [
    "week 1/huggingface/preprocessed_samples/01_beignets_preprocessed.jpg",
    "week 1/huggingface/preprocessed_samples/02_beignets_preprocessed.jpg",
]
for i in range(2):
    x = 2 + i * 5
    add_image(slide, raw_paths[i], x, 1.8, width=2)
    add_image(slide, proc_paths[i], x, 4.3, width=2)
    add_para(add_textbox(slide, x, 1.3, 2, 0.4), f"original #{i+1}", size=14, color="555555", align=PP_ALIGN.CENTER)
    add_para(add_textbox(slide, x, 3.8, 2, 0.4), f"processed #{i+1}", size=14, color="555555", align=PP_ALIGN.CENTER)
add_para(add_textbox(slide, 1.5, 6.3, 10, 0.5), "5 samples x 4 types (original/flip/rotate/bright) = 20 images", size=18, color="555555")

# ---- Checklist ----
slide = add_slide(prs)
add_title(slide, "제출 항목 체크리스트")
tf = add_textbox(slide, 1.5, 2.0, 10, 4.5)
for item in [
    "Git 저장소 구성",
    "Branch / PR / Merge",
    "red.py 빨간색 검출",
    "dataset.py Food101 로드 + pandas",
    "image_preprocessing.py 전처리 파이프라인",
    "Resize 224x224",
    "Grayscale + Normalize",
    "GaussianBlur 노이즈 제거",
    "데이터 증강 3종 (flip, rotate, bright)",
    "이상치 탐지 2종 (dark, small object)",
    "전처리 결과물 20장",
    "PPT 보고서 (본 문서)",
]:
    add_para(tf, f"  [O] {item}", size=18, color="27ae60")

# ---- Lessons ----
slide = add_slide(prs)
add_bg(slide, "2c3e50")
tf = add_textbox(slide, 1.5, 1.0, 10, 5.5)
add_para(tf, "배운 점", size=36, bold=True, color="FFFFFF")
add_para(tf, "", size=10)
for line, is_title in [
    ("1. HSV가 RGB보다 실용적이다", True),
    ("   조명 변화에 강하고, 인간이 인식하는 '색상'에 가깝다.", False),
    ("", False),
    ("2. cv2.bitwise_or vs + 연산자", True),
    ("   uint8에서 255+255=510 overflow. 비트 연산이 정확하다.", False),
    ("", False),
    ("3. pandas로 EDA 먼저, 전처리는 그 다음", True),
    ("   데이터 분포를 먼저 파악하고 전처리 전략을 세운다.", False),
    ("", False),
    ("4. Git 브랜치 전략은 혼자여도 의미 있다", True),
    ("   실수 복구가 쉽고, 작업 단위가 명확해진다.", False),
    ("", False),
    ("5. OpenCV + NumPy만으로 충분한 파이프라인", True),
    ("   추가 라이브러리 의존 없이 기본 함수 조합으로 구현했다.", False),
]:
    if line == "":
        add_para(tf, "", size=8)
    else:
        add_para(tf, line, size=18 if is_title else 16, bold=is_title,
                 color="FFFFFF" if is_title else "AAAAAA")

prs.save("presentation.pptx")
print("DONE: presentation.pptx (python-pptx, fully editable)")
