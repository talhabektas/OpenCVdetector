#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "ScoreCalculator.h"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @class FileWriter
 * @brief Writes exam results to files
 */
class FileWriter {
public:
    /**
     * @brief Constructor
     */
    FileWriter();
    
    /**
     * @brief Save results to text file
     * @param filename Output filename
     * @param score Exam score
     * @param studentName Student name (optional)
     * @param examName Exam name (optional)
     * @return true if successful, false otherwise
     */
    bool saveResultsToText(
        const std::string& filename,
        const ExamScore& score,
        const std::string& studentName = "",
        const std::string& examName = ""
    ) const;
    
    /**
     * @brief Save results to CSV file
     * @param filename Output filename
     * @param score Exam score
     * @param studentName Student name (optional)
     * @return true if successful, false otherwise
     */
    bool saveResultsToCSV(
        const std::string& filename,
        const ExamScore& score,
        const std::string& studentName = ""
    ) const;
    
    /**
     * @brief Save image with results overlay
     * @param filename Output filename
     * @param image Image to save
     * @return true if successful, false otherwise
     */
    bool saveResultImage(
        const std::string& filename,
        const cv::Mat& image
    ) const;
    
    /**
     * @brief Create timestamped filename
     * @param prefix Filename prefix
     * @param extension File extension (e.g., ".txt", ".jpg")
     * @return Timestamped filename
     */
    std::string createTimestampedFilename(
        const std::string& prefix,
        const std::string& extension
    ) const;

private:
    /**
     * @brief Get current timestamp string
     * @return Timestamp in format YYYYMMDD_HHMMSS
     */
    std::string getCurrentTimestamp() const;
};

#endif // FILE_WRITER_H
