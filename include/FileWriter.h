#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "ScoreCalculator.h"
#include <opencv2/opencv.hpp>
#include <string>

class FileWriter {
public:
    FileWriter();
    
    bool saveResultsToText(const std::string& filename, const ExamScore& score, const std::string& studentName = "", const std::string& examName = "") const;
    bool saveResultsToCSV(const std::string& filename, const ExamScore& score, const std::string& studentName = "") const;
    bool saveResultImage(const std::string& filename, const cv::Mat& image) const;
    std::string createTimestampedFilename(const std::string& prefix, const std::string& extension) const;

private:
    std::string getCurrentTimestamp() const;
};

#endif
