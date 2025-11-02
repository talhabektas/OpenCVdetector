#ifndef OCR_PROCESSOR_H
#define OCR_PROCESSOR_H

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <allheaders.h>
#include <string>
#include <memory>

class OCRProcessor {
public:
    explicit OCRProcessor(const char* language = "tur", const char* dataPath = nullptr);
    ~OCRProcessor();
    
    std::string recognizeText(const cv::Mat& handwritingROI);
    cv::Mat preprocessForOCR(const cv::Mat& image);
    void setPageSegmentationMode(tesseract::PageSegMode mode);
    float getConfidence() const;
    std::string recognizeTextWithConfidence(const cv::Mat& handwritingROI, float minConfidence = 50.0f);
    bool isInitialized() const;

private:
    tesseract::TessBaseAPI* tesseractAPI;
    bool initialized;
    float lastConfidence;
    
    std::string postProcessText(const std::string& rawText);
    Pix* matToPix(const cv::Mat& image);
    std::string trimText(const std::string& text);
    
    OCRProcessor(const OCRProcessor&) = delete;
    OCRProcessor& operator=(const OCRProcessor&) = delete;
};

#endif
