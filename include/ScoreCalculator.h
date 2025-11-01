#ifndef SCORE_CALCULATOR_H
#define SCORE_CALCULATOR_H

#include "AnswerKey.h"
#include "AnswerComparator.h"
#include <vector>
#include <map>

/**
 * @struct QuestionResult
 * @brief Result for a single question
 */
struct QuestionResult {
    int questionNumber;
    bool isCorrect;
    Answer studentAnswer;
    Answer correctAnswer;
    double partialCredit; // 0.0 to 1.0 (for partial credit on fill-in-the-blank)
    
    QuestionResult() : questionNumber(0), isCorrect(false), partialCredit(0.0) {}
};

/**
 * @struct ExamScore
 * @brief Overall exam scoring results
 */
struct ExamScore {
    int totalQuestions;
    int correctAnswers;
    int incorrectAnswers;
    int unanswered;
    double rawScore;
    double percentageScore;
    std::vector<QuestionResult> questionResults;
    
    ExamScore() : totalQuestions(0), correctAnswers(0), incorrectAnswers(0),
                  unanswered(0), rawScore(0.0), percentageScore(0.0) {}
};

/**
 * @class ScoreCalculator
 * @brief Calculates exam scores and statistics
 */
class ScoreCalculator {
public:
    /**
     * @brief Constructor
     * @param answerKey Reference to answer key
     * @param comparator Reference to answer comparator
     */
    ScoreCalculator(const AnswerKey& answerKey, const AnswerComparator& comparator);
    
    /**
     * @brief Calculate score for student answers
     * @param studentAnswers Vector of student answers
     * @return ExamScore structure with detailed results
     */
    ExamScore calculateScore(const std::vector<Answer>& studentAnswers);
    
    /**
     * @brief Set point value for each question
     * @param points Points per question (default: 1.0)
     */
    void setPointsPerQuestion(double points);
    
    /**
     * @brief Set custom point values for specific questions
     * @param questionNum Question number
     * @param points Point value
     */
    void setQuestionPoints(int questionNum, double points);
    
    /**
     * @brief Enable/disable partial credit for fill-in-the-blank
     * @param enable true to enable, false to disable
     */
    void setPartialCreditEnabled(bool enable);
    
    /**
     * @brief Set minimum similarity for partial credit
     * @param threshold Minimum similarity (0.0 to 1.0)
     */
    void setPartialCreditThreshold(double threshold);
    
    /**
     * @brief Get detailed statistics
     * @param score Exam score to analyze
     * @return Map of statistic names to values
     */
    std::map<std::string, double> getStatistics(const ExamScore& score) const;

private:
    const AnswerKey& answerKey;
    const AnswerComparator& comparator;
    double pointsPerQuestion;
    std::map<int, double> customPoints;
    bool partialCreditEnabled;
    double partialCreditThreshold;
    
    /**
     * @brief Get point value for a question
     * @param questionNum Question number
     * @return Point value
     */
    double getQuestionPoints(int questionNum) const;
    
    /**
     * @brief Calculate partial credit for a fill-in-the-blank answer
     * @param studentAns Student answer
     * @param correctAns Correct answer
     * @return Partial credit (0.0 to 1.0)
     */
    double calculatePartialCredit(const Answer& studentAns, const Answer& correctAns) const;
};

#endif // SCORE_CALCULATOR_H
