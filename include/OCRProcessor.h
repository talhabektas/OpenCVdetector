#ifndef OCR_PROCESSOR_H
#define OCR_PROCESSOR_H

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <allheaders.h>
#include <string>
#include <memory>

/**
 * @class OCRProcessor
 * @brief Processes handwritten text using Tesseract OCR
 * 
 * Configured for Turkish language support and optimized for handwriting recognition.
 */
class OCRProcessor {
public:
    /**
     * @brief Constructor - initializes Tesseract with Turkish language
     * @param language Language code (default: "tur" for Turkish)
     * @param dataPath Path to tessdata directory (default: nullptr for system default)
     */
    explicit OCRProcessor(const char* language = "tur", const char* dataPath = nullptr);
    
    /**
     * @brief Destructor - cleans up Tesseract resources
     */
    ~OCRProcessor();
    
    /**
     * @brief Recognize text from preprocessed image
     * @param handwritingROI Preprocessed image containing handwriting
     * @return Recognized text string
     */
    std::string recognizeText(const cv::Mat& handwritingROI);
    
    /**
     * @brief Preprocess image for optimal OCR accuracy
     * @param image Input image
     * @return Preprocessed image ready for OCR
     */
    cv::Mat preprocessForOCR(const cv::Mat& image);
    
    /**
     * @brief Set page segmentation mode
     * @param mode Tesseract PSM mode (e.g., PSM_SINGLE_LINE = 7)
     */
    void setPageSegmentationMode(tesseract::PageSegMode mode);
    
    /**
     * @brief Get OCR confidence score for last recognition
     * @return Confidence score (0-100)
     */
    float getConfidence() const;
    
    /**
     * @brief Recognize text with confidence threshold
     * @param handwritingROI Input image
     * @param minConfidence Minimum confidence threshold (0-100)
     * @return Recognized text if confidence >= threshold, empty string otherwise
     */
    std::string recognizeTextWithConfidence(
        const cv::Mat& handwritingROI,
        float minConfidence = 50.0f
    );
    
    /**
     * @brief Check if OCR is initialized and ready
     * @return true if ready, false otherwise
     */
    bool isInitialized() const;

private:
    tesseract::TessBaseAPI* tesseractAPI;
    bool initialized;
    float lastConfidence;
    
    /**
     * @brief Post-process recognized text
     * @param rawText Raw text from Tesseract
     * @return Cleaned and corrected text
     */
    std::string postProcessText(const std::string& rawText);
    
    /**
     * @brief Convert OpenCV Mat to Tesseract Pix format
     * @param image OpenCV image
     * @return Leptonica Pix image
     */
    Pix* matToPix(const cv::Mat& image);
    
    /**
     * @brief Remove extra whitespace and newlines
     * @param text Input text
     * @return Trimmed text
     */
    std::string trimText(const std::string& text);
    
    // Disable copy constructor and assignment operator
    OCRProcessor(const OCRProcessor&) = delete;
    OCRProcessor& operator=(const OCRProcessor&) = delete;
};

#endif // OCR_PROCESSOR_H
