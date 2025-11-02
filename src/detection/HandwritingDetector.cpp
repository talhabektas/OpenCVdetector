#include "HandwritingDetector.h"
#include <iostream>

HandwritingDetector::HandwritingDetector(double minDensity)
    : minimumPixelDensity(minDensity) {
    
    if (minDensity < 0.0 || minDensity > 1.0) {
        std::cerr << "Uyarı: Minimum density 0.0-1.0 arasında olmalı. 0.05 kullanılıyor." << std::endl;
        this->minimumPixelDensity = 0.05;
    }
    
    // Initialize preprocessing kernel
    preprocessingKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
}

cv::Mat HandwritingDetector::preprocessForDetection(const cv::Mat& roi) {
    cv::Mat processed;
    
    // Convert to grayscale if needed
    if (roi.channels() == 3) {
        cv::cvtColor(roi, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = roi.clone();
    }
    
    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    
    // Apply adaptive threshold
    cv::adaptiveThreshold(
        processed, processed, 255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        11, 2
    );
    
    // Remove small noise with morphological opening
    cv::morphologyEx(processed, processed, cv::MORPH_OPEN, preprocessingKernel);
    
    return processed;
}

double HandwritingDetector::analyzePixelDensity(const cv::Mat& roi) {
    if (roi.empty() || roi.total() == 0) {
        return 0.0;
    }
    
    // Count non-zero pixels (ink pixels)
    int nonZeroPixels = cv::countNonZero(roi);
    
    // Calculate density
    double density = static_cast<double>(nonZeroPixels) / roi.total();
    
    return density;
}

int HandwritingDetector::countEdgePixels(const cv::Mat& roi) {
    cv::Mat edges;
    
    // Convert to grayscale if needed
    cv::Mat gray;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }
    
    // Apply Canny edge detection
    cv::Canny(gray, edges, 50, 150);
    
    // Count edge pixels
    return cv::countNonZero(edges);
}

bool HandwritingDetector::hasConnectedComponents(const cv::Mat& roi) {
    if (roi.empty()) {
        return false;
    }
    
    // Find connected components
    cv::Mat labels, stats, centroids;
    int numComponents = cv::connectedComponentsWithStats(
        roi, labels, stats, centroids, 8, CV_32S
    );
    
    // Subtract 1 for background
    numComponents--;
    
    // Check if there are significant components
    // At least 2 components with minimum area
    const int minArea = 20; // Minimum pixels for a component
    int significantComponents = 0;
    
    for (int i = 1; i < numComponents + 1; i++) {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area >= minArea) {
            significantComponents++;
        }
    }
    
    return significantComponents >= 2;
}

double HandwritingDetector::calculatePixelDensity(const cv::Mat& roi) {
    cv::Mat processed = preprocessForDetection(roi);
    return analyzePixelDensity(processed);
}

bool HandwritingDetector::hasHandwriting(const cv::Mat& image, const cv::Rect& region) {
    // Validate region
    if (region.x < 0 || region.y < 0 ||
        region.x + region.width > image.cols ||
        region.y + region.height > image.rows) {
        std::cerr << "Uyarı: Geçersiz bölge koordinatları" << std::endl;
        return false;
    }
    
    // Extract ROI
    cv::Mat roi = image(region);
    
    if (roi.empty()) {
        std::cout << "[DEBUG] ROI boş!" << std::endl;
        return false;
    }
    
    // Preprocess ROI
    cv::Mat processed = preprocessForDetection(roi);
    
    // Calculate pixel density
    double density = analyzePixelDensity(processed);
    
    std::cout << "[DEBUG] Pixel yoğunluğu: " << (density * 100) << "% (min: " 
              << (minimumPixelDensity * 100) << "%)" << std::endl;
    
    // Quick check: if density is too low, no handwriting
    if (density < minimumPixelDensity) {
        std::cout << "[DEBUG] Pixel yoğunluğu çok düşük!" << std::endl;
        return false;
    }
    
    // Additional check: verify connected components
    bool hasComponents = hasConnectedComponents(processed);
    
    std::cout << "[DEBUG] Connected components: " << (hasComponents ? "VAR" : "YOK") << std::endl;
    
    return hasComponents;
}

cv::Mat HandwritingDetector::extractHandwritingROI(const cv::Mat& image, const cv::Rect& region) {
    // Validate region
    if (region.x < 0 || region.y < 0 ||
        region.x + region.width > image.cols ||
        region.y + region.height > image.rows) {
        throw std::runtime_error("Geçersiz bölge koordinatları!");
    }
    
    // Extract ROI
    cv::Mat roi = image(region).clone();
    
    // Convert to grayscale if needed
    if (roi.channels() == 3) {
        cv::cvtColor(roi, roi, cv::COLOR_BGR2GRAY);
    }
    
    return roi;
}

void HandwritingDetector::setMinDensity(double density) {
    if (density >= 0.0 && density <= 1.0) {
        minimumPixelDensity = density;
    }
}

std::vector<cv::Rect> HandwritingDetector::detectHandwritingRegions(
    const cv::Mat& image,
    const std::vector<cv::Rect>& searchRegions) {
    
    std::vector<cv::Rect> handwritingRegions;
    
    for (const auto& region : searchRegions) {
        if (hasHandwriting(image, region)) {
            handwritingRegions.push_back(region);
        }
    }
    
    return handwritingRegions;
}

double HandwritingDetector::detectHandwritingConfidence(
    const cv::Mat& image,
    const cv::Rect& region) {
    
    // Validate region
    if (region.x < 0 || region.y < 0 ||
        region.x + region.width > image.cols ||
        region.y + region.height > image.rows) {
        return 0.0;
    }
    
    // Extract ROI
    cv::Mat roi = image(region);
    
    if (roi.empty()) {
        return 0.0;
    }
    
    // Preprocess
    cv::Mat processed = preprocessForDetection(roi);
    
    // Calculate pixel density (weight: 0.4)
    double density = analyzePixelDensity(processed);
    double densityScore = std::min(density / 0.2, 1.0) * 0.4;
    
    // Calculate edge density (weight: 0.3)
    int edgePixels = countEdgePixels(roi);
    double edgeDensity = static_cast<double>(edgePixels) / roi.total();
    double edgeScore = std::min(edgeDensity / 0.15, 1.0) * 0.3;
    
    // Check connected components (weight: 0.3)
    bool hasComponents = hasConnectedComponents(processed);
    double componentScore = hasComponents ? 0.3 : 0.0;
    
    // Combined confidence score
    double confidence = densityScore + edgeScore + componentScore;
    
    return std::min(confidence, 1.0);
}
