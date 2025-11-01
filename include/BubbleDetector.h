#ifndef BUBBLE_DETECTOR_H
#define BUBBLE_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @class BubbleDetector
 * @brief Detects and analyzes filled bubbles in multiple-choice questions
 * 
 * Uses HoughCircles algorithm and fill percentage analysis.
 * A bubble is considered marked if fill percentage > 60%.
 */
class BubbleDetector {
public:
    /**
     * @brief Constructor with configurable fill threshold
     * @param fillThreshold Minimum fill percentage to consider bubble as marked (default: 0.6 = 60%)
     */
    explicit BubbleDetector(double fillThreshold = 0.6);
    
    /**
     * @brief Detect all circles (bubbles) in the image
     * @param image Input image (should be preprocessed)
     * @param minRadius Minimum bubble radius in pixels
     * @param maxRadius Maximum bubble radius in pixels
     * @return Vector of detected circles (x, y, radius)
     */
    std::vector<cv::Vec3f> detectBubbles(
        const cv::Mat& image,
        int minRadius = 10,
        int maxRadius = 30
    );
    
    /**
     * @brief Calculate fill percentage of a bubble
     * @param image Input image
     * @param center Center point of the bubble
     * @param radius Radius of the bubble
     * @return Fill percentage (0.0 to 1.0)
     */
    double calculateFillPercentage(
        const cv::Mat& image,
        const cv::Point& center,
        int radius
    );
    
    /**
     * @brief Check if a bubble is marked based on fill percentage
     * @param fillPercentage Fill percentage (0.0 to 1.0)
     * @return true if bubble is marked, false otherwise
     */
    bool isMarked(double fillPercentage) const;
    
    /**
     * @brief Detect marked bubbles in predefined regions
     * @param image Input image
     * @param bubbleRegions Vector of rectangular regions containing bubbles
     * @return Vector of indices of marked bubbles
     */
    std::vector<int> detectMarkedBubbles(
        const cv::Mat& image,
        const std::vector<cv::Rect>& bubbleRegions
    );
    
    /**
     * @brief Detect marked answer for a single question (row of bubbles)
     * @param image Input image
     * @param questionRegion Region containing all answer bubbles for one question
     * @param numOptions Number of answer options (e.g., 4 for A-D, 5 for A-E)
     * @return Index of marked answer (0-based), or -1 if none or multiple marked
     */
    int detectMarkedAnswer(
        const cv::Mat& image,
        const cv::Rect& questionRegion,
        int numOptions = 5
    );
    
    /**
     * @brief Set fill threshold
     * @param threshold New threshold value (0.0 to 1.0)
     */
    void setFillThreshold(double threshold);
    
    /**
     * @brief Draw detected bubbles on image for visualization
     * @param image Image to draw on
     * @param bubbles Vector of detected bubbles
     * @param fillPercentages Fill percentages for each bubble (optional)
     */
    void visualizeBubbles(
        cv::Mat& image,
        const std::vector<cv::Vec3f>& bubbles,
        const std::vector<double>& fillPercentages = {}
    );

private:
    double fillThreshold;
    
    /**
     * @brief Extract circular ROI for a bubble
     * @param image Input image
     * @param center Center of circle
     * @param radius Radius of circle
     * @return Circular ROI mask and bounded region
     */
    cv::Mat extractBubbleROI(
        const cv::Mat& image,
        const cv::Point& center,
        int radius
    );
};

#endif // BUBBLE_DETECTOR_H
