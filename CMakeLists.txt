cmake_minimum_required(VERSION 3.15)
project(OMR_System VERSION 1.0.0 LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(OpenCV 4.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/camera/CameraManager.cpp
    src/preprocessing/PerspectiveCorrector.cpp
    src/preprocessing/ImageEnhancer.cpp
    src/detection/BubbleDetector.cpp
    src/detection/HandwritingDetector.cpp
    src/detection/SheetStructureAnalyzer.cpp
    src/ocr/OCRProcessor.cpp
    src/grading/AnswerKey.cpp
    src/grading/AnswerComparator.cpp
    src/grading/ScoreCalculator.cpp
    src/output/ResultDisplayer.cpp
    src/output/FileWriter.cpp
)

# Create executable
add_executable(OMR_System ${SOURCES})

# Link libraries
target_link_libraries(OMR_System
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)

# Add include directories for Tesseract
target_include_directories(OMR_System PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(OMR_System PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target
install(TARGETS OMR_System DESTINATION bin)

# OCR Test utility
add_executable(ocr_test src/ocr_test.cpp)

target_link_libraries(ocr_test
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)

target_include_directories(ocr_test PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Manual Crop utility
add_executable(manual_crop src/manual_crop.cpp)

target_link_libraries(manual_crop
    ${OpenCV_LIBS}
)

# Silent Crop - Pencere açmadan kırp
add_executable(silent_crop src/silent_crop.cpp)

target_link_libraries(silent_crop
    ${OpenCV_LIBS}
)

# Image Preview utility
add_executable(image_preview src/image_preview.cpp)

target_link_libraries(image_preview
    ${OpenCV_LIBS}
)

# Auto Text Detector - Yazı bölgelerini otomatik bul
add_executable(auto_text_detector src/auto_text_detector.cpp)

target_link_libraries(auto_text_detector
    ${OpenCV_LIBS}
)

# Simple Line Detector - Projeksiyon profili ile satır bulma
add_executable(simple_line_detector src/simple_line_detector.cpp)

target_link_libraries(simple_line_detector
    ${OpenCV_LIBS}
)

# Handwriting Reader - Tam otomatik el yazısı okuyucu
add_executable(handwriting_reader src/handwriting_reader.cpp)

target_link_libraries(handwriting_reader
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)

target_include_directories(handwriting_reader PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Live Reader - Canlı kamera ile el yazısı okuma
add_executable(live_reader src/live_reader.cpp)

target_link_libraries(live_reader
    ${OpenCV_LIBS}
)

# Photo Reader - Telefon fotoğraflarını oku (Bird's Eye View + Manuel Seçim)
add_executable(photo_reader src/photo_reader.cpp)

target_link_libraries(photo_reader
    ${OpenCV_LIBS}
)

# Exam Grader - Sınav değerlendirme sistemi (El yazısı + True/False + Çoktan seçmeli)
add_executable(exam_grader src/exam_grader.cpp)

target_link_libraries(exam_grader
    ${OpenCV_LIBS}
)

# Smart Exam Grader - AKILLI SINAV DEĞERLENDİRME (kağıt tespiti + dinamik ROI + kamera)
add_executable(smart_exam_grader src/smart_exam_grader.cpp)

target_link_libraries(smart_exam_grader
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)

target_include_directories(smart_exam_grader PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Final Grader - GERÇEK KAĞITLAR İÇİN (page1.png + page2.png)
add_executable(final_grader src/final_grader.cpp)

target_link_libraries(final_grader
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)

target_include_directories(final_grader PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# ROI Selector - 4 NOKTA İLE DİKDÖRTGEN SEÇİMİ
add_executable(roi_selector src/roi_selector.cpp)

target_link_libraries(roi_selector
    ${OpenCV_LIBS}
)

# Camera Exam Grader - KAMERA İLE CANLI SINAV DEĞERLENDİRME
add_executable(camera_exam_grader src/camera_exam_grader.cpp)

target_link_libraries(camera_exam_grader
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)

target_include_directories(camera_exam_grader PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Test Manual Transform - MANUEL PERSPECTIVE TRANSFORM TESTİ
add_executable(test_manual_transform src/test_manual_transform.cpp)
target_link_libraries(test_manual_transform ${OpenCV_LIBS})

# Test Corrected Image - DÜZELTİLMİŞ GÖRÜNTÜYü FINAL_GRADER İLE TEST ET
add_executable(test_corrected_image src/test_corrected_image.cpp)
target_link_libraries(test_corrected_image 
    ${OpenCV_LIBS}
    ${TESSERACT_LDFLAGS}
    ${LEPTONICA_LDFLAGS}
)
target_include_directories(test_corrected_image PRIVATE
    ${TESSERACT_INCLUDE_DIRS}
    ${LEPTONICA_INCLUDE_DIRS}
)

# Compare Images - ORİJİNAL vs MANUEL DÜZELTİLMİŞ KARŞILAŞTIRMA
add_executable(compare_images src/compare_images.cpp)
target_link_libraries(compare_images ${OpenCV_LIBS})

# ROI Comparison - ROI BÖLGE KARŞILAŞTIRMASI
add_executable(roi_comparison src/roi_comparison.cpp)
target_link_libraries(roi_comparison ${OpenCV_LIBS})

# Print configuration
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libs: ${OpenCV_LIBS}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
