#ifndef ANSWER_COMPARATOR_H
#define ANSWER_COMPARATOR_H

#include "AnswerKey.h"
#include <string>
#include <algorithm>

/**
 * @class AnswerComparator
 * @brief Compares student answers with correct answers
 */
class AnswerComparator {
public:
    /**
     * @brief Constructor
     * @param caseSensitive Whether text comparison should be case-sensitive
     */
    explicit AnswerComparator(bool caseSensitive = false);
    
    /**
     * @brief Compare a multiple choice answer
     * @param studentAnswer Student's selected option
     * @param correctAnswer Correct option
     * @return true if correct, false otherwise
     */
    bool compareMultipleChoice(int studentAnswer, int correctAnswer) const;
    
    /**
     * @brief Compare a fill-in-the-blank answer
     * @param studentAnswer Student's text answer
     * @param correctAnswer Correct text answer
     * @return true if correct, false otherwise
     */
    bool compareFillInBlank(const std::string& studentAnswer, 
                           const std::string& correctAnswer) const;
    
    /**
     * @brief Compare a True/False answer
     * @param studentAnswer Student's answer (0=True, 1=False)
     * @param correctAnswer Correct answer (0=True, 1=False)
     * @return true if correct, false otherwise
     */
    bool compareTrueFalse(int studentAnswer, int correctAnswer) const;
    
    /**
     * @brief Compare any answer type
     * @param studentAns Student's answer
     * @param correctAns Correct answer
     * @return true if correct, false otherwise
     */
    bool compareAnswer(const Answer& studentAns, const Answer& correctAns) const;
    
    /**
     * @brief Set case sensitivity for text comparison
     * @param sensitive true for case-sensitive, false otherwise
     */
    void setCaseSensitive(bool sensitive);
    
    /**
     * @brief Calculate similarity percentage between two text answers
     * @param text1 First text
     * @param text2 Second text
     * @return Similarity percentage (0.0 to 1.0)
     */
    double calculateTextSimilarity(const std::string& text1, 
                                   const std::string& text2) const;

private:
    bool caseSensitive;
    
    /**
     * @brief Normalize text for comparison (trim, lowercase if needed)
     * @param text Input text
     * @return Normalized text
     */
    std::string normalizeText(const std::string& text) const;
    
    /**
     * @brief Trim whitespace from text
     * @param text Input text
     * @return Trimmed text
     */
    std::string trim(const std::string& text) const;
    
    /**
     * @brief Convert text to lowercase
     * @param text Input text
     * @return Lowercase text
     */
    std::string toLower(const std::string& text) const;
};

#endif // ANSWER_COMPARATOR_H
