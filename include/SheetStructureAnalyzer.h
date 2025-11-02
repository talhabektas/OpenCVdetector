#ifndef SHEET_STRUCTURE_ANALYZER_H
#define SHEET_STRUCTURE_ANALYZER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct QuestionRegion {
    int questionNumber;
    cv::Rect region;
    enum Type { MULTIPLE_CHOICE, FILL_IN_BLANK, TRUE_FALSE } type;
    int numOptions;
    
    QuestionRegion(int num, const cv::Rect& rect, Type t, int options = 0)
        : questionNumber(num), region(rect), type(t), numOptions(options) {}
};

class SheetStructureAnalyzer {
public:
    SheetStructureAnalyzer();
    
    std::vector<QuestionRegion> analyzeSheet(const cv::Mat& image);
    std::vector<QuestionRegion> detectMultipleChoiceRegions(const cv::Mat& image, int numQuestions, int optionsPerQuestion = 5);
    std::vector<QuestionRegion> detectFillInBlankRegions(const cv::Mat& image, int numQuestions);
    std::vector<QuestionRegion> detectTrueFalseRegions(const cv::Mat& image, int numQuestions);
    void setSheetDimensions(int sheetHeight, int sheetWidth);
    cv::Mat visualizeRegions(const cv::Mat& image, const std::vector<QuestionRegion>& regions);

private:
    int sheetHeight;
    int sheetWidth;
    
    static constexpr int DEFAULT_MC_START_Y = 100;
    static constexpr int DEFAULT_MC_HEIGHT = 40;
    static constexpr int DEFAULT_MC_SPACING = 10;
    static constexpr int DEFAULT_FILL_HEIGHT = 60;
    
    cv::Rect calculateMultipleChoiceRegion(int questionNumber, int optionsPerQuestion);
    cv::Rect calculateFillInBlankRegion(int questionNumber);
    std::vector<int> detectSeparatorLines(const cv::Mat& image);
};

#endif
