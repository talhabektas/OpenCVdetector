#ifndef RESULT_DISPLAYER_H
#define RESULT_DISPLAYER_H

#include "ScoreCalculator.h"
#include "SheetStructureAnalyzer.h"
#include <opencv2/opencv.hpp>
#include <string>

class ResultDisplayer {
public:
    ResultDisplayer();
    
    void displayScoreSummary(const ExamScore& score) const;
    void displayDetailedResults(const ExamScore& score) const;
    cv::Mat createVisualResults(const cv::Mat& examImage, const ExamScore& score, const std::vector<QuestionRegion>& regions) const;
    void displayInWindow(const std::string& windowName, const cv::Mat& resultsImage, int waitTime = 0) const;
    void displayStatistics(const std::map<std::string, double>& stats) const;

private:
    void drawResultMark(cv::Mat& image, const cv::Rect& region, bool isCorrect) const;
    void drawScoreBox(cv::Mat& image, const ExamScore& score) const;
};

#endif
