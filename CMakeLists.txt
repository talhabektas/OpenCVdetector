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

# Print configuration
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libs: ${OpenCV_LIBS}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
