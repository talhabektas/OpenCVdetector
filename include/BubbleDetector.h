#ifndef BUBBLE_DETECTOR_H
#define BUBBLE_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>

class BubbleDetector {
public:
    explicit BubbleDetector(double fillThreshold = 0.6);
    
    std::vector<cv::Vec3f> detectBubbles(const cv::Mat& image, int minRadius = 10, int maxRadius = 30);
    double calculateFillPercentage(const cv::Mat& image, const cv::Point& center, int radius);
    bool isMarked(double fillPercentage) const;
    std::vector<int> detectMarkedBubbles(const cv::Mat& image, const std::vector<cv::Rect>& bubbleRegions);
    int detectMarkedAnswer(const cv::Mat& image, const cv::Rect& questionRegion, int numOptions = 5);
    void setFillThreshold(double threshold);
    void visualizeBubbles(cv::Mat& image, const std::vector<cv::Vec3f>& bubbles, const std::vector<double>& fillPercentages = {});

private:
    double fillThreshold;
    cv::Mat extractBubbleROI(const cv::Mat& image, const cv::Point& center, int radius);
};

#endif
