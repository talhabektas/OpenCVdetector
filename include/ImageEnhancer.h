#ifndef IMAGE_ENHANCER_H
#define IMAGE_ENHANCER_H

#include <opencv2/opencv.hpp>

/**
 * @class ImageEnhancer
 * @brief Enhances image quality for better detection and OCR accuracy
 * 
 * Provides methods for noise reduction, contrast enhancement, and binarization.
 */
class ImageEnhancer {
public:
    /**
     * @brief Constructor
     */
    ImageEnhancer();
    
    /**
     * @brief Enhance image quality for general processing
     * @param image Input image
     * @return Enhanced image
     */
    cv::Mat enhanceImage(const cv::Mat& image);
    
    /**
     * @brief Apply adaptive binarization for better text/mark detection
     * @param image Input grayscale image
     * @param blockSize Size of pixel neighborhood (default: 11)
     * @param C Constant subtracted from mean (default: 2)
     * @return Binary image
     */
    cv::Mat adaptiveBinarize(const cv::Mat& image, int blockSize = 11, double C = 2);
    
    /**
     * @brief Remove noise from image using morphological operations
     * @param image Input image
     * @param kernelSize Size of morphological kernel (default: 3)
     * @return Denoised image
     */
    cv::Mat removeNoise(const cv::Mat& image, int kernelSize = 3);
    
    /**
     * @brief Enhance contrast using CLAHE (Contrast Limited Adaptive Histogram Equalization)
     * @param image Input image
     * @param clipLimit Threshold for contrast limiting (default: 2.0)
     * @param tileSize Size of grid for histogram equalization (default: 8)
     * @return Contrast-enhanced image
     */
    cv::Mat enhanceContrast(const cv::Mat& image, double clipLimit = 2.0, int tileSize = 8);
    
    /**
     * @brief Sharpen image to improve edge clarity
     * @param image Input image
     * @return Sharpened image
     */
    cv::Mat sharpenImage(const cv::Mat& image);
    
    /**
     * @brief Complete preprocessing pipeline for OCR
     * @param image Input image
     * @return Preprocessed image optimized for OCR
     */
    cv::Mat preprocessForOCR(const cv::Mat& image);
    
    /**
     * @brief Complete preprocessing pipeline for bubble detection
     * @param image Input image
     * @return Preprocessed image optimized for bubble detection
     */
    cv::Mat preprocessForBubbles(const cv::Mat& image);

private:
    cv::Ptr<cv::CLAHE> clahe;
    
    /**
     * @brief Convert image to grayscale if needed
     * @param image Input image
     * @return Grayscale image
     */
    cv::Mat convertToGray(const cv::Mat& image);
};

#endif // IMAGE_ENHANCER_H
