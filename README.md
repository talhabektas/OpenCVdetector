# 📝 OMR System - Optical Mark Recognition for Exam Grading

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenCV](https://img.shields.io/badge/OpenCV-4.x-green.svg)](https://opencv.org/)
[![Tesseract](https://img.shields.io/badge/Tesseract-5.x-orange.svg)](https://github.com/tesseract-ocr/tesseract)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

> An intelligent exam grading system that automatically evaluates multiple-choice, true/false, and handwritten fill-in-the-blank questions using real-time camera processing.


---

## 🎯 Features

### ✨ Core Capabilities

- **📷 Real-time Camera Processing** - Live paper detection and capture
- **🤖 Automatic Paper Detection** - Adaptive white mask with various lighting conditions
- **📐 Perspective Correction** - Bird's-eye view transformation for tilted papers
- **✍️ Handwriting Recognition** - OCR for fill-in-the-blank questions (Turkish support)
- **⚪ Bubble Detection** - Multiple-choice and True/False answer recognition
- **📊 Automatic Grading** - Instant scoring with detailed feedback
- **💾 Result Export** - TXT and CSV file generation with timestamps
- **🎨 Visual Overlay** - Real-time grading results on video feed

### 🎮 User Modes

#### Automatic Mode (Recommended)
- Show paper to camera
- System detects and stabilizes (15 frames)
- Automatically captures both pages
- Instant grading with visual overlay

#### Manual Mode
- Press '1' to capture Page 1 (Handwriting + T/F)
- Press '2' to capture Page 2 (Multiple Choice)
- Press 'g' to grade
- Press 'm' for manual corner selection

---

## 🏗️ Architecture

### OOP Design with SOLID Principles

```
┌─────────────────────────────────────────┐
│  camera_exam_grader_refactored.cpp     │
│  Main Program (Camera + UI Control)    │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  ExamGrader (Orchestrator)             │
│  - Coordinates all components          │
│  - Manages 2-page capture workflow     │
└─┬───────┬───────┬───────┬──────────────┘
  │       │       │       │
  ▼       ▼       ▼       ▼
┌────┐  ┌────┐  ┌────┐  ┌────┐
│IMG │  │PAP │  │BUB │  │OCR │
│PRE │  │DET │  │DET │  │PRO │
└────┘  └────┘  └────┘  └────┘
```

### Module Overview

| Module | Responsibility | Key Features |
|--------|---------------|--------------|
| **ImagePreprocessor** | Image enhancement | White balance, CLAHE, bilateral filter |
| **PaperDetector** | Paper localization | Adaptive HSV thresholds, contour detection |
| **BubbleDetector** | Bubble recognition | Fill ratio analysis, multi-option detection |
| **OCRProcessor** | Handwriting OCR | Tesseract LSTM, Turkish language support |
| **PerspectiveCorrector** | Geometry correction | Homography, 4-point transform |
| **ExamGrader** | Grade orchestration | Answer comparison, scoring, overlay |

---

## 🔧 Technical Details

### Image Processing Pipeline

#### 1. Paper Detection
```
Camera Frame → HSV Conversion → Adaptive White Mask
    → Morphological Operations → Canny Edge Detection
    → Contour Detection → Quadrilateral Approximation
    → Validation → 4 Corner Points
```

#### 2. Perspective Correction
```
4 Corners → Order Points (TL, TR, BR, BL)
    → Perspective Transform Matrix
    → Warp to 850x1100 (Page 1) / 1232x1782 (Page 2)
```

#### 3. Handwriting OCR Pipeline
```
ROI Extraction → White Balance (Gray World Algorithm)
    → Grayscale → CLAHE (3.0 clip, 8×8 tiles)
    → Bilateral Filter → 3.08× Upscaling
    → OTSU Threshold → Tesseract LSTM
    → Post-processing → Normalized Text
```

#### 4. Bubble Detection
```
ROI Extraction → Grayscale → OTSU Threshold
    → Morphological Cleaning → Count Non-Zero Pixels
    → Fill Ratio Calculation → Threshold Comparison
    → Marked/Unmarked Decision
```

### Adaptive Algorithms

#### Brightness-Based HSV Thresholds
| Environment | Avg Brightness | HSV V (Min) | HSV S (Max) |
|-------------|---------------|-------------|-------------|
| Very Dark | < 80 | 100 | 80 |
| Dark | 80-120 | 130 | 60 |
| Normal | 120-180 | 150 | 50 |
| Bright | > 180 | 200 | 40 |

#### Fill Ratio Thresholds
- **True/False:** 12% (X marks)
- **Multiple Choice:** 25% minimum + 10% significant difference OR 30% absolute

---

## 📊 Grading System

### Question Types & Points

```
📝 Handwriting Questions:  5 × 12 points = 60 points
   - Exact match: 12 points
   - High similarity (>80%): 12 points
   - Good match (>60%): 8 points
   - Partial match (>50%): 6 points

✅ True/False:  2 × 5 points = 10 points

🔘 Multiple Choice:  2 × 5 points = 10 points

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 TOTAL: 80 points (100 scale)
```

### Answer Comparison (Fuzzy Matching)

Uses **Levenshtein Distance** for handwriting tolerance:
- Distance ≤ 2: 10 points
- Distance = 3: 8 points
- Distance = 4: 6 points
- Distance ≥ 5: 0 points

---

## 🚀 Getting Started

### Prerequisites

```bash
# macOS
brew install opencv tesseract tesseract-lang

# Ubuntu/Debian
sudo apt-get install libopencv-dev tesseract-ocr libtesseract-dev

# Install Turkish language data
sudo apt-get install tesseract-ocr-tur  # Linux
brew install tesseract-lang            # macOS
```

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/talhabektas/OpenCVdetector.git
cd OpenCVdetector

# Create build directory
cmake -S . -B build

# Build the project
cmake --build build -j4

# Run the application
cd build
./camera_exam_grader_refactored
```

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.15)
project(OMR_System VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV 4.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TESSERACT REQUIRED tesseract)

# ... (see CMakeLists.txt for full configuration)
```

---

## 🎮 Usage

### Keyboard Controls

| Key | Action | Mode |
|-----|--------|------|
| `a` | Toggle automatic/manual mode | Both |
| `1` | Capture Page 1 | Manual |
| `2` | Capture Page 2 | Manual |
| `g` | Grade exam | Manual |
| `r` | Reset pages | Both |
| `m` | Manual corner selection | Both |
| `h` | Show help | Both |
| `ESC` | Save & exit | Both |

### Workflow

#### Automatic Mode (Recommended)
1. Launch: `./camera_exam_grader_refactored`
2. Show Page 1 to camera (hold steady for 15 frames)
3. System auto-captures and saves
4. Show Page 2 to camera
5. System auto-captures, grades, and displays results
6. Press ESC to save and exit

#### Manual Mode
1. Press `a` to switch to manual mode
2. Press `1` to capture Page 1
3. Press `2` to capture Page 2
4. Press `g` to grade
5. View results overlay
6. Press ESC to save and exit

---

## 📁 Project Structure

```
OMR-System/
├── include/                    # Header files (.h)
│   ├── AnswerComparator.h
│   ├── AnswerKey.h
│   ├── BubbleDetector.h
│   ├── CameraManager.h
│   ├── ExamGrader.h          # Main orchestrator
│   ├── FileWriter.h
│   ├── HandwritingDetector.h
│   ├── ImageEnhancer.h
│   ├── ImagePreprocessor.h   # Critical for OCR
│   ├── OCRProcessor.h        # Tesseract wrapper
│   ├── PaperDetector.h       # Adaptive white mask
│   ├── PerspectiveCorrector.h
│   ├── ResultDisplayer.h
│   ├── ScoreCalculator.h
│   └── constants.h           # All magic numbers
│
├── src/                        # Implementation files (.cpp)
│   ├── camera/
│   │   └── CameraManager.cpp
│   ├── detection/
│   │   ├── BubbleDetector.cpp
│   │   ├── HandwritingDetector.cpp
│   │   ├── PaperDetector.cpp
│   │   └── SheetStructureAnalyzer.cpp
│   ├── grading/
│   │   ├── AnswerComparator.cpp
│   │   ├── AnswerKey.cpp
│   │   ├── ExamGrader.cpp    # 818 lines - orchestrator
│   │   └── ScoreCalculator.cpp
│   ├── ocr/
│   │   └── OCRProcessor.cpp
│   ├── output/
│   │   ├── FileWriter.cpp
│   │   └── ResultDisplayer.cpp
│   ├── preprocessing/
│   │   ├── ImageEnhancer.cpp
│   │   ├── ImagePreprocessor.cpp  # 330 lines - critical
│   │   └── PerspectiveCorrector.cpp
│   ├── camera_exam_grader_refactored.cpp  # Main program
│   └── main.cpp              # Alternative main (non-refactored)
│
├── build/                      # Build artifacts
├── CMakeLists.txt             # CMake configuration
└── README.md                  # This file
```

---

## 📝 Output Format

### Text File (exam_result_[timestamp].txt)

```
========================================
       EXAM GRADING RESULTS
========================================

Date: Sun Dec 21 14:30:25 2025
Student: [Optional]

========================================
HANDWRITING QUESTIONS (60 points)
========================================
Q1: "ankara" (Correct: "ankara") → 12 points ✓
Q2: "atatürk" (Correct: "atatürk") → 12 points ✓
Q3: "asya avrupa" (Correct: "asya avrupa") → 12 points ✓
Q4: "arjantin" (Correct: "arjantin") → 12 points ✓
Q5: "sakarya" (Correct: "sakarya") → 12 points ✓
Subtotal: 60/60

========================================
TRUE/FALSE QUESTIONS (10 points)
========================================
Q1: FALSE (Correct: FALSE) → 5 points ✓
Q2: TRUE (Correct: TRUE) → 5 points ✓
Subtotal: 10/10

========================================
MULTIPLE CHOICE QUESTIONS (10 points)
========================================
Q1: C (Correct: C) → 5 points ✓
Q2: D (Correct: D) → 5 points ✓
Subtotal: 10/10

========================================
TOTAL SCORE: 80/100
========================================

✓ PASSED (≥80 points)

========================================
   OMR System - OOP Refactored v1.0
========================================
```

---

## 🔬 Algorithms Used

### Computer Vision
- **Canny Edge Detection** - Paper boundary detection
- **HoughCircles** - Circle detection (legacy bubble method)
- **CLAHE** - Contrast Limited Adaptive Histogram Equalization
- **Bilateral Filter** - Edge-preserving noise reduction
- **Morphological Operations** - Opening/Closing for noise removal
- **Perspective Transform** - 4-point homography
- **OTSU Thresholding** - Automatic binary conversion

### Machine Learning
- **Tesseract LSTM** - Neural network for OCR
- **Gray World Algorithm** - White balance correction
- **Levenshtein Distance** - String similarity for fuzzy matching

### Statistical Analysis
- **Fill Ratio Analysis** - Bubble marking detection
- **Confidence Scoring** - OCR reliability measurement
- **Adaptive Thresholding** - Brightness-based parameter adjustment

---

## ⚡ Performance

| Metric | Value |
|--------|-------|
| Real-time Processing | 30 FPS |
| Perspective Transform | < 50ms |
| OCR Processing (5 questions) | ~2-3 seconds |
| Total Grading Time | ~3-4 seconds |
| Memory Usage | ~75 MB |
| Handwriting Accuracy | 85%+ |
| Bubble Detection Accuracy | 95%+ |

---

## 🐛 Debug Features

### Debug Image Output

The system saves debug images in the `build/` directory:

```
build/
├── captured_page1.jpg              # Raw captured Page 1
├── captured_page2.jpg              # Raw captured Page 2
├── corrected_page1.jpg             # Perspective-corrected Page 1
├── corrected_page2.jpg             # Perspective-corrected Page 2
│
├── debug_hw_q1.jpg                 # Handwriting ROI Q1-Q5
├── debug_hw_q2.jpg
├── debug_hw_q3.jpg
├── debug_hw_q4.jpg
├── debug_hw_q5.jpg
│
├── debug_ocr_preprocessed_0.jpg    # OCR preprocessing Q1-Q5
├── debug_ocr_preprocessed_1.jpg
├── debug_ocr_preprocessed_2.jpg
├── debug_ocr_preprocessed_3.jpg
├── debug_ocr_preprocessed_4.jpg
│
├── debug_mc_q1_A.jpg               # Multiple choice bubbles
├── debug_mc_q1_B.jpg
├── debug_mc_q1_C.jpg
├── debug_mc_q1_D.jpg
└── ... (Q2 options)
```

---

## 🔧 Configuration

### Constants (include/constants.h)

```cpp
// Page Dimensions
constexpr int TEMPLATE_WIDTH = 850;
constexpr int TEMPLATE_HEIGHT = 1100;

// Fill Thresholds
constexpr double TF_FILL_THRESHOLD = 0.12;      // True/False
constexpr double MC_FILL_THRESHOLD = 0.60;      // Multiple Choice

// OCR Settings
constexpr double OCR_RESIZE_FACTOR = 3.0;
constexpr double CLAHE_CLIP_LIMIT = 2.0;
constexpr int CLAHE_TILE_SIZE = 8;

// Camera Settings
constexpr int REQUIRED_STABLE_FRAMES = 15;
constexpr int COOLDOWN_FRAMES = 90;
```

### Customization

To modify ROI coordinates for your exam template, edit:
- **Handwriting:** `ExamGrader::HW_ROI_COORDS` in `ExamGrader.cpp`
- **True/False:** `ExamGrader::TF_Q1_TRUE`, `TF_Q1_FALSE`, etc.
- **Multiple Choice:** `ExamGrader::MC_Q1_ROIS`, `MC_Q2_ROIS`

---

## 🧪 Testing

### Unit Tests (Planned)
```bash
# Build with tests
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build

# Run tests
cd build
ctest --verbose
```

### Sample Data
Test images available in `scanned exam/` directory.

---

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Follow SOLID principles and clean code practices
4. Add unit tests for new features
5. Update documentation
6. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
7. Push to the branch (`git push origin feature/AmazingFeature`)
8. Open a Pull Request

### Code Style

- **C++ Standard:** C++17
- **Naming Convention:**
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Constants: `UPPER_CASE`
  - Member variables: `camelCase_` (trailing underscore for private)
- **Comments:** Doxygen-style documentation
- **Line Length:** Max 100 characters

---

## 📚 Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| OpenCV | 4.x+ | Computer vision and image processing |
| Tesseract | 5.x+ | OCR engine for handwriting recognition |
| Leptonica | 1.x+ | Image processing library (Tesseract dependency) |
| CMake | 3.15+ | Build system |
| C++ Standard Library | C++17 | Smart pointers, containers, algorithms |

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---



## 🙏 Acknowledgments

- OpenCV community for excellent documentation
- Tesseract OCR team for the powerful OCR engine
- Stack Overflow community for troubleshooting help
- Our university professors for guidance and support

---

## 📧 Contact

For questions, issues, or suggestions:

- **Email:** [mehmettalha.bektas@gmail.com]

---

## 📊 Project Stats

- **Total Lines of Code:** ~3,200
- **Number of Classes:** 16
- **Number of Modules:** 6 layers
- **Documentation:** 100% header files documented
- **Test Coverage:** 
- **Performance:** 30 FPS real-time processing

---

<div align="center">

Made with ❤️ by the OMR System Team

**Star ⭐ this repository if you find it helpful!**

</div>
