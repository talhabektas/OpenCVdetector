#include "ImageEnhancer.h"
#include <iostream>

ImageEnhancer::ImageEnhancer() {
    // Initialize CLAHE
    clahe = cv::createCLAHE();
    clahe->setClipLimit(2.0);
    clahe->setTilesGridSize(cv::Size(8, 8));
}

cv::Mat ImageEnhancer::convertToGray(const cv::Mat& image) {
    if (image.channels() == 1) {
        return image.clone();
    }
    
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

cv::Mat ImageEnhancer::enhanceImage(const cv::Mat& image) {
    cv::Mat enhanced = image.clone();
    
    // Convert to grayscale if needed
    enhanced = convertToGray(enhanced);
    
    // Enhance contrast
    enhanced = enhanceContrast(enhanced);
    
    // Remove noise
    enhanced = removeNoise(enhanced);
    
    return enhanced;
}

cv::Mat ImageEnhancer::adaptiveBinarize(const cv::Mat& image, int blockSize, double C) {
    cv::Mat gray = convertToGray(image);
    cv::Mat binary;
    
    // Ensure block size is odd
    if (blockSize % 2 == 0) {
        blockSize++;
    }
    
    cv::adaptiveThreshold(gray, binary, 255, 
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                         cv::THRESH_BINARY,
                         blockSize, C);
    
    return binary;
}

cv::Mat ImageEnhancer::removeNoise(const cv::Mat& image, int kernelSize) {
    cv::Mat denoised;
    
    // Apply Gaussian blur for noise reduction
    cv::GaussianBlur(image, denoised, cv::Size(kernelSize, kernelSize), 0);
    
    // Apply morphological opening to remove small noise
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(denoised, denoised, cv::MORPH_OPEN, kernel);
    
    return denoised;
}

cv::Mat ImageEnhancer::enhanceContrast(const cv::Mat& image, double clipLimit, int tileSize) {
    cv::Mat gray = convertToGray(image);
    cv::Mat enhanced;
    
    // Update CLAHE parameters
    clahe->setClipLimit(clipLimit);
    clahe->setTilesGridSize(cv::Size(tileSize, tileSize));
    
    // Apply CLAHE
    clahe->apply(gray, enhanced);
    
    return enhanced;
}

cv::Mat ImageEnhancer::sharpenImage(const cv::Mat& image) {
    cv::Mat sharpened;
    
    // Create sharpening kernel
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
        0, -1, 0,
       -1,  5, -1,
        0, -1, 0);
    
    // Apply filter
    cv::filter2D(image, sharpened, -1, kernel);
    
    return sharpened;
}

cv::Mat ImageEnhancer::preprocessForOCR(const cv::Mat& image) {
    cv::Mat processed = image.clone();
    
    // 1. Convert to grayscale
    processed = convertToGray(processed);
    
    // 2. Enhance contrast
    processed = enhanceContrast(processed, 3.0, 8);
    
    // 3. Remove noise with gentle blur
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    
    // 4. Apply Otsu's binarization for better OCR
    cv::Mat binary;
    cv::threshold(processed, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    // 5. Invert if background is dark (OCR expects black text on white)
    int whitePixels = cv::countNonZero(binary);
    int totalPixels = binary.rows * binary.cols;
    if (whitePixels < totalPixels / 2) {
        cv::bitwise_not(binary, binary);
    }
    
    // 6. Remove small noise with morphological operations
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    
    // 7. Slight dilation to make characters more solid
    cv::dilate(binary, binary, kernel, cv::Point(-1, -1), 1);
    
    return binary;
}

cv::Mat ImageEnhancer::preprocessForBubbles(const cv::Mat& image) {
    cv::Mat processed = image.clone();
    
    // 1. Convert to grayscale
    processed = convertToGray(processed);
    
    // 2. Apply Gaussian blur (helps HoughCircles)
    cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0);
    
    // 3. Enhance contrast
    processed = enhanceContrast(processed, 2.0, 8);
    
    // 4. Apply adaptive threshold to clearly separate bubbles
    processed = adaptiveBinarize(processed, 15, 2);
    
    // 5. Morphological closing to fill small gaps in bubble circles
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
    
    return processed;
}
