#include "SheetStructureAnalyzer.h"
#include <iostream>
#include <algorithm>

SheetStructureAnalyzer::SheetStructureAnalyzer()
    : sheetHeight(1100), sheetWidth(850) {
}

void SheetStructureAnalyzer::setSheetDimensions(int height, int width) {
    sheetHeight = height;
    sheetWidth = width;
}

cv::Rect SheetStructureAnalyzer::calculateMultipleChoiceRegion(
    int questionNumber,
    int optionsPerQuestion) {
    
    int y = DEFAULT_MC_START_Y + questionNumber * (DEFAULT_MC_HEIGHT + DEFAULT_MC_SPACING);
    int x = 50; // Left margin
    int width = sheetWidth - 100; // Account for margins
    int height = DEFAULT_MC_HEIGHT;
    
    return cv::Rect(x, y, width, height);
}

cv::Rect SheetStructureAnalyzer::calculateFillInBlankRegion(int questionNumber) {
    int y = 500 + questionNumber * (DEFAULT_FILL_HEIGHT + DEFAULT_MC_SPACING);
    int x = 50;
    int width = sheetWidth - 100;
    int height = DEFAULT_FILL_HEIGHT;
    
    return cv::Rect(x, y, width, height);
}

std::vector<int> SheetStructureAnalyzer::detectSeparatorLines(const cv::Mat& image) {
    std::vector<int> linePositions;
    
    // Convert to grayscale
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // Apply threshold
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // Detect horizontal lines using morphological operations
    cv::Mat horizontal = binary.clone();
    int horizontalSize = horizontal.cols / 30;
    cv::Mat horizontalStructure = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(horizontalSize, 1)
    );
    cv::erode(horizontal, horizontal, horizontalStructure);
    cv::dilate(horizontal, horizontal, horizontalStructure);
    
    // Find contours of horizontal lines
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(horizontal, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Extract Y-coordinates of lines
    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        if (rect.width > image.cols / 2) { // Only consider long lines
            linePositions.push_back(rect.y);
        }
    }
    
    // Sort positions
    std::sort(linePositions.begin(), linePositions.end());
    
    return linePositions;
}

std::vector<QuestionRegion> SheetStructureAnalyzer::detectMultipleChoiceRegions(
    const cv::Mat& image,
    int numQuestions,
    int optionsPerQuestion) {
    
    std::vector<QuestionRegion> regions;
    
    for (int i = 0; i < numQuestions; i++) {
        cv::Rect region = calculateMultipleChoiceRegion(i, optionsPerQuestion);
        
        // Validate region
        if (region.y + region.height <= image.rows &&
            region.x + region.width <= image.cols) {
            regions.emplace_back(
                i + 1,
                region,
                QuestionRegion::MULTIPLE_CHOICE,
                optionsPerQuestion
            );
        }
    }
    
    return regions;
}

std::vector<QuestionRegion> SheetStructureAnalyzer::detectFillInBlankRegions(
    const cv::Mat& image,
    int numQuestions) {
    
    std::vector<QuestionRegion> regions;
    
    for (int i = 0; i < numQuestions; i++) {
        cv::Rect region = calculateFillInBlankRegion(i);
        
        // Validate region
        if (region.y + region.height <= image.rows &&
            region.x + region.width <= image.cols) {
            regions.emplace_back(
                i + 1,
                region,
                QuestionRegion::FILL_IN_BLANK,
                0
            );
        }
    }
    
    return regions;
}

std::vector<QuestionRegion> SheetStructureAnalyzer::detectTrueFalseRegions(
    const cv::Mat& image,
    int numQuestions) {
    
    std::vector<QuestionRegion> regions;
    
    // True/False questions are essentially multiple choice with 2 options
    for (int i = 0; i < numQuestions; i++) {
        cv::Rect region = calculateMultipleChoiceRegion(i, 2);
        region.y = 50 + i * (DEFAULT_MC_HEIGHT + DEFAULT_MC_SPACING);
        
        // Validate region
        if (region.y + region.height <= image.rows &&
            region.x + region.width <= image.cols) {
            regions.emplace_back(
                i + 1,
                region,
                QuestionRegion::TRUE_FALSE,
                2
            );
        }
    }
    
    return regions;
}

std::vector<QuestionRegion> SheetStructureAnalyzer::analyzeSheet(const cv::Mat& image) {
    std::vector<QuestionRegion> allRegions;
    
    // Example: Detect 10 multiple choice questions
    auto mcRegions = detectMultipleChoiceRegions(image, 10, 5);
    allRegions.insert(allRegions.end(), mcRegions.begin(), mcRegions.end());
    
    // Example: Detect 5 fill-in-the-blank questions
    auto fillRegions = detectFillInBlankRegions(image, 5);
    allRegions.insert(allRegions.end(), fillRegions.begin(), fillRegions.end());
    
    // Example: Detect 5 True/False questions
    auto tfRegions = detectTrueFalseRegions(image, 5);
    allRegions.insert(allRegions.end(), tfRegions.begin(), tfRegions.end());
    
    std::cout << "Toplam " << allRegions.size() << " soru bölgesi tespit edildi" << std::endl;
    
    return allRegions;
}

cv::Mat SheetStructureAnalyzer::visualizeRegions(
    const cv::Mat& image,
    const std::vector<QuestionRegion>& regions) {
    
    cv::Mat visualization = image.clone();
    
    // Ensure color image
    if (visualization.channels() == 1) {
        cv::cvtColor(visualization, visualization, cv::COLOR_GRAY2BGR);
    }
    
    for (const auto& qRegion : regions) {
        cv::Scalar color;
        std::string label;
        
        switch (qRegion.type) {
            case QuestionRegion::MULTIPLE_CHOICE:
                color = cv::Scalar(0, 255, 0); // Green
                label = "MC " + std::to_string(qRegion.questionNumber);
                break;
            case QuestionRegion::FILL_IN_BLANK:
                color = cv::Scalar(255, 0, 0); // Blue
                label = "Fill " + std::to_string(qRegion.questionNumber);
                break;
            case QuestionRegion::TRUE_FALSE:
                color = cv::Scalar(0, 165, 255); // Orange
                label = "T/F " + std::to_string(qRegion.questionNumber);
                break;
        }
        
        // Draw rectangle
        cv::rectangle(visualization, qRegion.region, color, 2);
        
        // Draw label
        cv::putText(
            visualization,
            label,
            cv::Point(qRegion.region.x, qRegion.region.y - 5),
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            color,
            2
        );
    }
    
    return visualization;
}
