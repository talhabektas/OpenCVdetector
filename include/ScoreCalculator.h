#ifndef SCORE_CALCULATOR_H
#define SCORE_CALCULATOR_H

#include "AnswerKey.h"
#include "AnswerComparator.h"
#include <vector>
#include <map>

struct QuestionResult {
    int questionNumber;
    bool isCorrect;
    Answer studentAnswer;
    Answer correctAnswer;
    double partialCredit;
    
    QuestionResult() : questionNumber(0), isCorrect(false), partialCredit(0.0) {}
};

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

class ScoreCalculator {
public:
    ScoreCalculator(const AnswerKey& answerKey, const AnswerComparator& comparator);
    
    ExamScore calculateScore(const std::vector<Answer>& studentAnswers);
    void setPointsPerQuestion(double points);
    void setQuestionPoints(int questionNum, double points);
    void setPartialCreditEnabled(bool enable);
    void setPartialCreditThreshold(double threshold);
    std::map<std::string, double> getStatistics(const ExamScore& score) const;

private:
    const AnswerKey& answerKey;
    const AnswerComparator& comparator;
    double pointsPerQuestion;
    std::map<int, double> customPoints;
    bool partialCreditEnabled;
    double partialCreditThreshold;
    
    double getQuestionPoints(int questionNum) const;
    double calculatePartialCredit(const Answer& studentAns, const Answer& correctAns) const;
};

#endif
