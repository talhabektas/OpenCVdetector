#ifndef HANDWRITING_DETECTOR_H
#define HANDWRITING_DETECTOR_H

#include <opencv2/opencv.hpp>

/**
 * @class HandwritingDetector
 * @brief Detects presence of handwriting in fill-in-the-blank regions
 * 
 * Uses pixel density analysis and edge detection to determine
 * if a region contains handwritten content.
 */
class HandwritingDetector {
public:
    /**
     * @brief Constructor with configurable minimum pixel density
     * @param minDensity Minimum pixel density to consider as handwriting (default: 0.05 = 5%)
     */
    explicit HandwritingDetector(double minDensity = 0.05);
    
    /**
     * @brief Detect if handwriting is present in a region
     * @param image Input image
     * @param region Region of interest to check
     * @return true if handwriting detected, false otherwise
     */
    bool hasHandwriting(const cv::Mat& image, const cv::Rect& region);
    
    /**
     * @brief Extract handwriting ROI for OCR processing
     * @param image Input image
     * @param region Region containing handwriting
     * @return Extracted and preprocessed ROI
     */
    cv::Mat extractHandwritingROI(const cv::Mat& image, const cv::Rect& region);
    
    /**
     * @brief Calculate pixel density in a region
     * @param roi Region of interest
     * @return Pixel density (0.0 to 1.0)
     */
    double calculatePixelDensity(const cv::Mat& roi);
    
    /**
     * @brief Set minimum pixel density threshold
     * @param density New threshold value (0.0 to 1.0)
     */
    void setMinDensity(double density);
    
    /**
     * @brief Detect multiple handwriting regions in an image
     * @param image Input image
     * @param searchRegions Regions to search for handwriting
     * @return Vector of regions containing handwriting
     */
    std::vector<cv::Rect> detectHandwritingRegions(
        const cv::Mat& image,
        const std::vector<cv::Rect>& searchRegions
    );
    
    /**
     * @brief Advanced handwriting detection using edge analysis
     * @param image Input image
     * @param region Region to analyze
     * @return Confidence score (0.0 to 1.0) of handwriting presence
     */
    double detectHandwritingConfidence(const cv::Mat& image, const cv::Rect& region);

private:
    double minimumPixelDensity;
    cv::Mat preprocessingKernel;
    
    /**
     * @brief Preprocess region for handwriting detection
     * @param roi Region of interest
     * @return Preprocessed image
     */
    cv::Mat preprocessForDetection(const cv::Mat& roi);
    
    /**
     * @brief Analyze pixel density in ROI
     * @param roi Binary image
     * @return Density value (0.0 to 1.0)
     */
    double analyzePixelDensity(const cv::Mat& roi);
    
    /**
     * @brief Count edge pixels to detect writing strokes
     * @param roi Region of interest
     * @return Number of edge pixels
     */
    int countEdgePixels(const cv::Mat& roi);
    
    /**
     * @brief Check if region has connected components (writing strokes)
     * @param roi Binary image
     * @return true if significant connected components found
     */
    bool hasConnectedComponents(const cv::Mat& roi);
};

#endif // HANDWRITING_DETECTOR_H
