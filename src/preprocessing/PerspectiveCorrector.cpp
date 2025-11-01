#include "PerspectiveCorrector.h"
#include <algorithm>
#include <iostream>
#include <cmath>

PerspectiveCorrector::PerspectiveCorrector(double cannyThreshold1, double cannyThreshold2)
    : cannyThreshold1(cannyThreshold1), cannyThreshold2(cannyThreshold2) {
}

cv::Mat PerspectiveCorrector::preprocessForEdgeDetection(const cv::Mat& image) {
    cv::Mat gray, blurred;
    
    // Convert to grayscale
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
    
    return blurred;
}

std::vector<cv::Point2f> PerspectiveCorrector::detectPaperCorners(const cv::Mat& image) {
    // Preprocess image
    cv::Mat preprocessed = preprocessForEdgeDetection(image);
    
    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(preprocessed, edges, cannyThreshold1, cannyThreshold2);
    
    // Dilate to connect broken edges
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(edges, edges, kernel);
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    if (contours.empty()) {
        throw std::runtime_error("Kağıt kenarları tespit edilemedi!");
    }
    
    // Find largest quadrilateral
    std::vector<cv::Point2f> corners = findLargestQuadrilateral(contours);
    
    if (corners.size() != 4) {
        throw std::runtime_error("Kağıdın 4 köşesi bulunamadı!");
    }
    
    // Order points
    return orderPoints(corners);
}

std::vector<cv::Point2f> PerspectiveCorrector::findLargestQuadrilateral(
    const std::vector<std::vector<cv::Point>>& contours) {
    
    double maxArea = 0;
    std::vector<cv::Point2f> largestQuad;
    
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        // Skip small contours
        if (area < 10000) {
            continue;
        }
        
        // Approximate contour to polygon
        double peri = cv::arcLength(contour, true);
        std::vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, 0.02 * peri, true);
        
        // Check if it's a quadrilateral
        if (approx.size() == 4 && area > maxArea) {
            maxArea = area;
            largestQuad.clear();
            for (const auto& point : approx) {
                largestQuad.push_back(cv::Point2f(point.x, point.y));
            }
        }
    }
    
    return largestQuad;
}

std::vector<cv::Point2f> PerspectiveCorrector::orderPoints(const std::vector<cv::Point2f>& points) {
    if (points.size() != 4) {
        return points;
    }
    
    std::vector<cv::Point2f> ordered(4);
    
    // Calculate sum and difference of coordinates
    std::vector<float> sums(4);
    std::vector<float> diffs(4);
    
    for (int i = 0; i < 4; i++) {
        sums[i] = points[i].x + points[i].y;
        diffs[i] = points[i].y - points[i].x;
    }
    
    // Top-left has smallest sum
    auto minSum = std::min_element(sums.begin(), sums.end());
    ordered[0] = points[std::distance(sums.begin(), minSum)];
    
    // Bottom-right has largest sum
    auto maxSum = std::max_element(sums.begin(), sums.end());
    ordered[2] = points[std::distance(sums.begin(), maxSum)];
    
    // Top-right has smallest difference
    auto minDiff = std::min_element(diffs.begin(), diffs.end());
    ordered[1] = points[std::distance(diffs.begin(), minDiff)];
    
    // Bottom-left has largest difference
    auto maxDiff = std::max_element(diffs.begin(), diffs.end());
    ordered[3] = points[std::distance(diffs.begin(), maxDiff)];
    
    return ordered;
}

cv::Mat PerspectiveCorrector::applyPerspectiveTransform(
    const cv::Mat& image,
    const std::vector<cv::Point2f>& corners,
    int outputWidth,
    int outputHeight) {
    
    if (corners.size() != 4) {
        throw std::runtime_error("Perspektif dönüşümü için 4 köşe noktası gerekli!");
    }
    
    // Define destination points for the warped image
    std::vector<cv::Point2f> dst = {
        cv::Point2f(0, 0),
        cv::Point2f(outputWidth - 1, 0),
        cv::Point2f(outputWidth - 1, outputHeight - 1),
        cv::Point2f(0, outputHeight - 1)
    };
    
    // Calculate perspective transformation matrix
    cv::Mat transformMatrix = cv::getPerspectiveTransform(corners, dst);
    
    // Apply warp perspective
    cv::Mat warped;
    cv::warpPerspective(image, warped, transformMatrix, cv::Size(outputWidth, outputHeight));
    
    return warped;
}

cv::Mat PerspectiveCorrector::correctPerspective(
    const cv::Mat& image,
    int outputWidth,
    int outputHeight) {
    
    try {
        // Detect corners
        std::vector<cv::Point2f> corners = detectPaperCorners(image);
        
        // Apply transformation
        cv::Mat corrected = applyPerspectiveTransform(image, corners, outputWidth, outputHeight);
        
        std::cout << "Perspektif düzeltmesi başarılı" << std::endl;
        return corrected;
        
    } catch (const std::exception& e) {
        std::cerr << "Perspektif düzeltme hatası: " << e.what() << std::endl;
        // Return original image if correction fails
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(outputWidth, outputHeight));
        return resized;
    }
}

void PerspectiveCorrector::setCannyThresholds(double threshold1, double threshold2) {
    cannyThreshold1 = threshold1;
    cannyThreshold2 = threshold2;
}
