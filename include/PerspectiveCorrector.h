#ifndef PERSPECTIVE_CORRECTOR_H
#define PERSPECTIVE_CORRECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @class PerspectiveCorrector
 * @brief Detects paper corners and applies bird's-eye view transformation
 * 
 * This class handles perspective correction for exam sheets captured at an angle.
 * Uses Canny edge detection and contour analysis to find the paper boundaries.
 */
class PerspectiveCorrector {
public:
    /**
     * @brief Constructor with configurable parameters
     * @param cannyThreshold1 First threshold for Canny edge detector (default: 50)
     * @param cannyThreshold2 Second threshold for Canny edge detector (default: 150)
     */
    explicit PerspectiveCorrector(double cannyThreshold1 = 50.0, double cannyThreshold2 = 150.0);
    
    /**
     * @brief Detect the four corners of the paper in the image
     * @param image Input image
     * @return Vector of 4 corner points (ordered: top-left, top-right, bottom-right, bottom-left)
     * @throws std::runtime_error if paper corners cannot be detected
     */
    std::vector<cv::Point2f> detectPaperCorners(const cv::Mat& image);
    
    /**
     * @brief Apply perspective transformation to get bird's-eye view
     * @param image Input image
     * @param corners Four corner points of the paper
     * @param outputWidth Desired output width (default: 850)
     * @param outputHeight Desired output height (default: 1100)
     * @return Warped image with corrected perspective
     */
    cv::Mat applyPerspectiveTransform(
        const cv::Mat& image,
        const std::vector<cv::Point2f>& corners,
        int outputWidth = 850,
        int outputHeight = 1100
    );
    
    /**
     * @brief Complete perspective correction pipeline
     * @param image Input image
     * @param outputWidth Desired output width (default: 850)
     * @param outputHeight Desired output height (default: 1100)
     * @return Corrected bird's-eye view image
     */
    cv::Mat correctPerspective(
        const cv::Mat& image,
        int outputWidth = 850,
        int outputHeight = 1100
    );
    
    /**
     * @brief Set Canny edge detection thresholds
     * @param threshold1 First threshold
     * @param threshold2 Second threshold
     */
    void setCannyThresholds(double threshold1, double threshold2);

private:
    double cannyThreshold1;
    double cannyThreshold2;
    
    /**
     * @brief Order points in clockwise order starting from top-left
     * @param points Unordered corner points
     * @return Ordered points (top-left, top-right, bottom-right, bottom-left)
     */
    std::vector<cv::Point2f> orderPoints(const std::vector<cv::Point2f>& points);
    
    /**
     * @brief Find the largest contour that approximates to a quadrilateral
     * @param contours Vector of all contours
     * @return Quadrilateral contour points
     */
    std::vector<cv::Point2f> findLargestQuadrilateral(
        const std::vector<std::vector<cv::Point>>& contours
    );
    
    /**
     * @brief Preprocess image for better edge detection
     * @param image Input image
     * @return Preprocessed image
     */
    cv::Mat preprocessForEdgeDetection(const cv::Mat& image);
};

#endif // PERSPECTIVE_CORRECTOR_H
