#include "BubbleDetector.h"
#include <iostream>
#include <algorithm>

BubbleDetector::BubbleDetector(double fillThreshold)
    : fillThreshold(fillThreshold) {
    
    if (fillThreshold < 0.0 || fillThreshold > 1.0) {
        std::cerr << "Uyarı: Fill threshold 0.0-1.0 arasında olmalı. 0.6 kullanılıyor." << std::endl;
        this->fillThreshold = 0.6;
    }
}

std::vector<cv::Vec3f> BubbleDetector::detectBubbles(
    const cv::Mat& image,
    int minRadius,
    int maxRadius) {
    
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // Apply Gaussian blur
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
    
    // Detect circles using HoughCircles
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(
        blurred,
        circles,
        cv::HOUGH_GRADIENT,
        1,                          // dp: inverse ratio of accumulator resolution
        gray.rows / 16,            // minDist: minimum distance between centers
        50,                         // param1: upper threshold for Canny
        30,                         // param2: threshold for center detection
        minRadius,
        maxRadius
    );
    
    return circles;
}

cv::Mat BubbleDetector::extractBubbleROI(
    const cv::Mat& image,
    const cv::Point& center,
    int radius) {
    
    // Define bounding rectangle
    int x = std::max(0, center.x - radius);
    int y = std::max(0, center.y - radius);
    int width = std::min(image.cols - x, 2 * radius);
    int height = std::min(image.rows - y, 2 * radius);
    
    cv::Rect boundingRect(x, y, width, height);
    cv::Mat roi = image(boundingRect).clone();
    
    // Create circular mask
    cv::Mat mask = cv::Mat::zeros(roi.size(), CV_8UC1);
    cv::Point localCenter(radius, radius);
    cv::circle(mask, localCenter, radius, cv::Scalar(255), -1);
    
    // Apply mask to ROI
    cv::Mat maskedROI;
    roi.copyTo(maskedROI, mask);
    
    return maskedROI;
}

double BubbleDetector::calculateFillPercentage(
    const cv::Mat& image,
    const cv::Point& center,
    int radius) {
    
    // Extract bubble ROI
    cv::Mat bubbleROI = extractBubbleROI(image, center, radius);
    
    // Convert to grayscale if needed
    cv::Mat gray;
    if (bubbleROI.channels() == 3) {
        cv::cvtColor(bubbleROI, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = bubbleROI.clone();
    }
    
    // Apply threshold to separate filled from unfilled
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // Count non-zero (filled) pixels
    int filledPixels = cv::countNonZero(binary);
    
    // Total pixels in circle
    int totalPixels = static_cast<int>(CV_PI * radius * radius);
    
    if (totalPixels == 0) {
        return 0.0;
    }
    
    // Calculate fill percentage
    double fillPercentage = static_cast<double>(filledPixels) / totalPixels;
    
    return fillPercentage;
}

bool BubbleDetector::isMarked(double fillPercentage) const {
    return fillPercentage >= fillThreshold;
}

std::vector<int> BubbleDetector::detectMarkedBubbles(
    const cv::Mat& image,
    const std::vector<cv::Rect>& bubbleRegions) {
    
    std::vector<int> markedIndices;
    
    for (size_t i = 0; i < bubbleRegions.size(); i++) {
        cv::Rect region = bubbleRegions[i];
        
        // Extract region
        cv::Mat roi = image(region);
        
        // Detect bubbles in this region
        int estimatedRadius = std::min(region.width, region.height) / 3;
        std::vector<cv::Vec3f> bubbles = detectBubbles(
            roi,
            estimatedRadius - 5,
            estimatedRadius + 5
        );
        
        // Check if any bubble is marked
        for (const auto& bubble : bubbles) {
            cv::Point center(
                static_cast<int>(bubble[0]),
                static_cast<int>(bubble[1])
            );
            int radius = static_cast<int>(bubble[2]);
            
            double fillPct = calculateFillPercentage(roi, center, radius);
            
            if (isMarked(fillPct)) {
                markedIndices.push_back(static_cast<int>(i));
                break; // Only count once per region
            }
        }
    }
    
    return markedIndices;
}

int BubbleDetector::detectMarkedAnswer(
    const cv::Mat& image,
    const cv::Rect& questionRegion,
    int numOptions) {
    
    // Extract question region
    cv::Mat roi = image(questionRegion);
    
    // Divide region into equal parts for each option
    int optionWidth = questionRegion.width / numOptions;
    
    std::vector<std::pair<int, double>> markedOptions; // (option index, fill percentage)
    
    for (int i = 0; i < numOptions; i++) {
        cv::Rect optionRect(i * optionWidth, 0, optionWidth, questionRegion.height);
        cv::Mat optionROI = roi(optionRect);
        
        // Detect bubbles in this option
        int estimatedRadius = std::min(optionWidth, questionRegion.height) / 3;
        std::vector<cv::Vec3f> bubbles = detectBubbles(
            optionROI,
            estimatedRadius - 5,
            estimatedRadius + 5
        );
        
        // Find the bubble with highest fill percentage
        double maxFill = 0.0;
        for (const auto& bubble : bubbles) {
            cv::Point center(
                static_cast<int>(bubble[0]),
                static_cast<int>(bubble[1])
            );
            int radius = static_cast<int>(bubble[2]);
            
            double fillPct = calculateFillPercentage(optionROI, center, radius);
            maxFill = std::max(maxFill, fillPct);
        }
        
        if (isMarked(maxFill)) {
            markedOptions.push_back({i, maxFill});
        }
    }
    
    // Return -1 if none or multiple answers marked
    if (markedOptions.size() != 1) {
        return -1;
    }
    
    return markedOptions[0].first;
}

void BubbleDetector::setFillThreshold(double threshold) {
    if (threshold >= 0.0 && threshold <= 1.0) {
        fillThreshold = threshold;
    }
}

void BubbleDetector::visualizeBubbles(
    cv::Mat& image,
    const std::vector<cv::Vec3f>& bubbles,
    const std::vector<double>& fillPercentages) {
    
    for (size_t i = 0; i < bubbles.size(); i++) {
        cv::Point center(
            static_cast<int>(bubbles[i][0]),
            static_cast<int>(bubbles[i][1])
        );
        int radius = static_cast<int>(bubbles[i][2]);
        
        // Determine color based on fill percentage
        cv::Scalar color;
        if (!fillPercentages.empty() && i < fillPercentages.size()) {
            if (isMarked(fillPercentages[i])) {
                color = cv::Scalar(0, 255, 0); // Green for marked
            } else {
                color = cv::Scalar(255, 0, 0); // Blue for unmarked
            }
        } else {
            color = cv::Scalar(0, 0, 255); // Red for unknown
        }
        
        // Draw circle
        cv::circle(image, center, radius, color, 2);
        
        // Draw center
        cv::circle(image, center, 3, color, -1);
    }
}
