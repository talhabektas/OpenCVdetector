#ifndef IMAGE_ENHANCER_H
#define IMAGE_ENHANCER_H

#include <opencv2/opencv.hpp>

class ImageEnhancer {
public:
    ImageEnhancer();
    
    cv::Mat enhanceImage(const cv::Mat& image);
    cv::Mat adaptiveBinarize(const cv::Mat& image, int blockSize = 11, double C = 2);
    cv::Mat removeNoise(const cv::Mat& image, int kernelSize = 3);
    cv::Mat enhanceContrast(const cv::Mat& image, double clipLimit = 2.0, int tileSize = 8);
    cv::Mat sharpenImage(const cv::Mat& image);
    cv::Mat preprocessForOCR(const cv::Mat& image);
    cv::Mat preprocessForBubbles(const cv::Mat& image);

private:
    cv::Ptr<cv::CLAHE> clahe;
    cv::Mat convertToGray(const cv::Mat& image);
};

#endif
