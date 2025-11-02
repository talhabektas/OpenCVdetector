#ifndef HANDWRITING_DETECTOR_H
#define HANDWRITING_DETECTOR_H

#include <opencv2/opencv.hpp>

class HandwritingDetector {
public:
    explicit HandwritingDetector(double minDensity = 0.05);
    
    bool hasHandwriting(const cv::Mat& image, const cv::Rect& region);
    cv::Mat extractHandwritingROI(const cv::Mat& image, const cv::Rect& region);
    double calculatePixelDensity(const cv::Mat& roi);
    void setMinDensity(double density);
    std::vector<cv::Rect> detectHandwritingRegions(const cv::Mat& image, const std::vector<cv::Rect>& searchRegions);
    double detectHandwritingConfidence(const cv::Mat& image, const cv::Rect& region);

private:
    double minimumPixelDensity;
    cv::Mat preprocessingKernel;
    
    cv::Mat preprocessForDetection(const cv::Mat& roi);
    double analyzePixelDensity(const cv::Mat& roi);
    int countEdgePixels(const cv::Mat& roi);
    bool hasConnectedComponents(const cv::Mat& roi);
};

#endif
