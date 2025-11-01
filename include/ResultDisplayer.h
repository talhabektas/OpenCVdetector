#ifndef RESULT_DISPLAYER_H
#define RESULT_DISPLAYER_H

#include "ScoreCalculator.h"
#include "SheetStructureAnalyzer.h"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @class ResultDisplayer
 * @brief Displays exam results visually on screen and images
 */
class ResultDisplayer {
public:
    /**
     * @brief Constructor
     */
    ResultDisplayer();
    
    /**
     * @brief Display score summary in console
     * @param score Exam score to display
     */
    void displayScoreSummary(const ExamScore& score) const;
    
    /**
     * @brief Display detailed results for each question
     * @param score Exam score with question results
     */
    void displayDetailedResults(const ExamScore& score) const;
    
    /**
     * @brief Create visual representation of results on exam image
     * @param examImage Original exam image
     * @param score Exam score
     * @param regions Question regions
     * @return Image with results overlaid
     */
    cv::Mat createVisualResults(
        const cv::Mat& examImage,
        const ExamScore& score,
        const std::vector<QuestionRegion>& regions
    ) const;
    
    /**
     * @brief Display results in a window
     * @param windowName Window name
     * @param resultsImage Image to display
     * @param waitTime Wait time in ms (0 = wait for key, -1 = no wait)
     */
    void displayInWindow(
        const std::string& windowName,
        const cv::Mat& resultsImage,
        int waitTime = 0
    ) const;
    
    /**
     * @brief Print statistics
     * @param stats Statistics map
     */
    void displayStatistics(const std::map<std::string, double>& stats) const;

private:
    /**
     * @brief Draw checkmark or X mark on question region
     * @param image Image to draw on
     * @param region Question region
     * @param isCorrect Whether answer is correct
     */
    void drawResultMark(
        cv::Mat& image,
        const cv::Rect& region,
        bool isCorrect
    ) const;
    
    /**
     * @brief Draw score box on image
     * @param image Image to draw on
     * @param score Exam score
     */
    void drawScoreBox(cv::Mat& image, const ExamScore& score) const;
};

#endif // RESULT_DISPLAYER_H
