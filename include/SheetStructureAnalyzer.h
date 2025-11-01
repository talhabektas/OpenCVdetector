#ifndef SHEET_STRUCTURE_ANALYZER_H
#define SHEET_STRUCTURE_ANALYZER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * @struct QuestionRegion
 * @brief Represents a question region on the exam sheet
 */
struct QuestionRegion {
    int questionNumber;
    cv::Rect region;
    enum Type { MULTIPLE_CHOICE, FILL_IN_BLANK, TRUE_FALSE } type;
    int numOptions; // For multiple choice
    
    QuestionRegion(int num, const cv::Rect& rect, Type t, int options = 0)
        : questionNumber(num), region(rect), type(t), numOptions(options) {}
};

/**
 * @class SheetStructureAnalyzer
 * @brief Analyzes exam sheet structure and identifies question regions
 * 
 * Detects and categorizes different question types and their locations.
 */
class SheetStructureAnalyzer {
public:
    /**
     * @brief Constructor
     */
    SheetStructureAnalyzer();
    
    /**
     * @brief Analyze sheet structure and detect all question regions
     * @param image Input image of the exam sheet
     * @return Vector of detected question regions
     */
    std::vector<QuestionRegion> analyzeSheet(const cv::Mat& image);
    
    /**
     * @brief Detect multiple choice question regions
     * @param image Input image
     * @param numQuestions Number of multiple choice questions
     * @param optionsPerQuestion Number of options per question (default: 5)
     * @return Vector of multiple choice regions
     */
    std::vector<QuestionRegion> detectMultipleChoiceRegions(
        const cv::Mat& image,
        int numQuestions,
        int optionsPerQuestion = 5
    );
    
    /**
     * @brief Detect fill-in-the-blank regions
     * @param image Input image
     * @param numQuestions Number of fill-in questions
     * @return Vector of fill-in-the-blank regions
     */
    std::vector<QuestionRegion> detectFillInBlankRegions(
        const cv::Mat& image,
        int numQuestions
    );
    
    /**
     * @brief Detect True/False question regions
     * @param image Input image
     * @param numQuestions Number of True/False questions
     * @return Vector of True/False regions
     */
    std::vector<QuestionRegion> detectTrueFalseRegions(
        const cv::Mat& image,
        int numQuestions
    );
    
    /**
     * @brief Create custom layout configuration
     * @param sheetHeight Total height of sheet
     * @param sheetWidth Total width of sheet
     */
    void setSheetDimensions(int sheetHeight, int sheetWidth);
    
    /**
     * @brief Visualize detected regions on image
     * @param image Image to draw on
     * @param regions Regions to visualize
     * @return Image with drawn regions
     */
    cv::Mat visualizeRegions(
        const cv::Mat& image,
        const std::vector<QuestionRegion>& regions
    );

private:
    int sheetHeight;
    int sheetWidth;
    
    // Default layout parameters
    static constexpr int DEFAULT_MC_START_Y = 100;
    static constexpr int DEFAULT_MC_HEIGHT = 40;
    static constexpr int DEFAULT_MC_SPACING = 10;
    static constexpr int DEFAULT_FILL_HEIGHT = 60;
    
    /**
     * @brief Calculate region for a multiple choice question
     * @param questionNumber Question number (0-based)
     * @param optionsPerQuestion Number of options
     * @return Region rectangle
     */
    cv::Rect calculateMultipleChoiceRegion(int questionNumber, int optionsPerQuestion);
    
    /**
     * @brief Calculate region for a fill-in-the-blank question
     * @param questionNumber Question number (0-based)
     * @return Region rectangle
     */
    cv::Rect calculateFillInBlankRegion(int questionNumber);
    
    /**
     * @brief Detect horizontal lines that might separate questions
     * @param image Input image
     * @return Y-coordinates of detected lines
     */
    std::vector<int> detectSeparatorLines(const cv::Mat& image);
};

#endif // SHEET_STRUCTURE_ANALYZER_H
