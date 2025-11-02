#ifndef PERSPECTIVE_CORRECTOR_H
#define PERSPECTIVE_CORRECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>

class PerspectiveCorrector {
public:
    explicit PerspectiveCorrector(double cannyThreshold1 = 50.0, double cannyThreshold2 = 150.0);
    
    std::vector<cv::Point2f> detectPaperCorners(const cv::Mat& image);
    cv::Mat applyPerspectiveTransform(const cv::Mat& image, const std::vector<cv::Point2f>& corners, int outputWidth = 850, int outputHeight = 1100);
    cv::Mat correctPerspective(const cv::Mat& image, int outputWidth = 850, int outputHeight = 1100);
    void setCannyThresholds(double threshold1, double threshold2);

private:
    double cannyThreshold1;
    double cannyThreshold2;
    
    std::vector<cv::Point2f> orderPoints(const std::vector<cv::Point2f>& points);
    std::vector<cv::Point2f> findLargestQuadrilateral(const std::vector<std::vector<cv::Point>>& contours);
    cv::Mat preprocessForEdgeDetection(const cv::Mat& image);
};

#endif
