#ifndef ANSWER_COMPARATOR_H
#define ANSWER_COMPARATOR_H

#include "AnswerKey.h"
#include <string>
#include <algorithm>

class AnswerComparator {
public:
    explicit AnswerComparator(bool caseSensitive = false);
    
    bool compareMultipleChoice(int studentAnswer, int correctAnswer) const;
    bool compareFillInBlank(const std::string& studentAnswer, const std::string& correctAnswer) const;
    bool compareTrueFalse(int studentAnswer, int correctAnswer) const;
    bool compareAnswer(const Answer& studentAns, const Answer& correctAns) const;
    void setCaseSensitive(bool sensitive);
    double calculateTextSimilarity(const std::string& text1, const std::string& text2) const;

private:
    bool caseSensitive;
    
    std::string normalizeText(const std::string& text) const;
    std::string trim(const std::string& text) const;
    std::string toLower(const std::string& text) const;
};

#endif
