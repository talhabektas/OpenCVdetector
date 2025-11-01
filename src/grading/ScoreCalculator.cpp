#include "ScoreCalculator.h"
#include <iostream>

ScoreCalculator::ScoreCalculator(const AnswerKey& answerKey, const AnswerComparator& comparator)
    : answerKey(answerKey), comparator(comparator), pointsPerQuestion(1.0),
      partialCreditEnabled(true), partialCreditThreshold(0.7) {
}

void ScoreCalculator::setPointsPerQuestion(double points) {
    if (points > 0.0) {
        pointsPerQuestion = points;
    }
}

void ScoreCalculator::setQuestionPoints(int questionNum, double points) {
    if (points > 0.0) {
        customPoints[questionNum] = points;
    }
}

void ScoreCalculator::setPartialCreditEnabled(bool enable) {
    partialCreditEnabled = enable;
}

void ScoreCalculator::setPartialCreditThreshold(double threshold) {
    if (threshold >= 0.0 && threshold <= 1.0) {
        partialCreditThreshold = threshold;
    }
}

double ScoreCalculator::getQuestionPoints(int questionNum) const {
    auto it = customPoints.find(questionNum);
    if (it != customPoints.end()) {
        return it->second;
    }
    return pointsPerQuestion;
}

double ScoreCalculator::calculatePartialCredit(const Answer& studentAns, 
                                               const Answer& correctAns) const {
    if (!partialCreditEnabled || studentAns.type != Answer::FILL_IN_BLANK) {
        return 0.0;
    }
    
    double similarity = comparator.calculateTextSimilarity(
        studentAns.textAnswer,
        correctAns.textAnswer
    );
    
    if (similarity >= partialCreditThreshold) {
        return similarity;
    }
    
    return 0.0;
}

ExamScore ScoreCalculator::calculateScore(const std::vector<Answer>& studentAnswers) {
    ExamScore score;
    score.totalQuestions = answerKey.getTotalQuestions();
    
    // Create a map of student answers for quick lookup
    std::map<int, Answer> studentAnswerMap;
    for (const auto& ans : studentAnswers) {
        studentAnswerMap[ans.questionNumber] = ans;
    }
    
    // Evaluate each question
    for (int qNum = 1; qNum <= score.totalQuestions; qNum++) {
        if (!answerKey.hasAnswer(qNum)) {
            continue;
        }
        
        Answer correctAns = answerKey.getAnswer(qNum);
        QuestionResult result;
        result.questionNumber = qNum;
        result.correctAnswer = correctAns;
        
        auto it = studentAnswerMap.find(qNum);
        if (it == studentAnswerMap.end()) {
            // Unanswered
            result.isCorrect = false;
            result.partialCredit = 0.0;
            score.unanswered++;
        } else {
            result.studentAnswer = it->second;
            result.isCorrect = comparator.compareAnswer(result.studentAnswer, correctAns);
            
            if (result.isCorrect) {
                result.partialCredit = 1.0;
                score.correctAnswers++;
            } else {
                // Check for partial credit
                result.partialCredit = calculatePartialCredit(result.studentAnswer, correctAns);
                if (result.partialCredit == 0.0) {
                    score.incorrectAnswers++;
                }
            }
        }
        
        // Add to results
        score.questionResults.push_back(result);
        
        // Calculate raw score
        double questionPoints = getQuestionPoints(qNum);
        score.rawScore += questionPoints * result.partialCredit;
    }
    
    // Calculate maximum possible score
    double maxScore = 0.0;
    for (int qNum = 1; qNum <= score.totalQuestions; qNum++) {
        maxScore += getQuestionPoints(qNum);
    }
    
    // Calculate percentage
    if (maxScore > 0.0) {
        score.percentageScore = (score.rawScore / maxScore) * 100.0;
    }
    
    return score;
}

std::map<std::string, double> ScoreCalculator::getStatistics(const ExamScore& score) const {
    std::map<std::string, double> stats;
    
    stats["Total Questions"] = score.totalQuestions;
    stats["Correct Answers"] = score.correctAnswers;
    stats["Incorrect Answers"] = score.incorrectAnswers;
    stats["Unanswered"] = score.unanswered;
    stats["Raw Score"] = score.rawScore;
    stats["Percentage"] = score.percentageScore;
    
    // Calculate accuracy rate
    int answered = score.totalQuestions - score.unanswered;
    if (answered > 0) {
        stats["Accuracy Rate"] = (static_cast<double>(score.correctAnswers) / answered) * 100.0;
    } else {
        stats["Accuracy Rate"] = 0.0;
    }
    
    return stats;
}
