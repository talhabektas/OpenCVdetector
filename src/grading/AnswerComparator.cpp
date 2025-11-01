#include "AnswerComparator.h"
#include <cctype>
#include <algorithm>

AnswerComparator::AnswerComparator(bool caseSensitive)
    : caseSensitive(caseSensitive) {
}

std::string AnswerComparator::trim(const std::string& text) const {
    auto start = std::find_if_not(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    
    auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    
    return (start < end) ? std::string(start, end) : std::string();
}

std::string AnswerComparator::toLower(const std::string& text) const {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string AnswerComparator::normalizeText(const std::string& text) const {
    std::string normalized = trim(text);
    
    if (!caseSensitive) {
        normalized = toLower(normalized);
    }
    
    return normalized;
}

bool AnswerComparator::compareMultipleChoice(int studentAnswer, int correctAnswer) const {
    return studentAnswer == correctAnswer;
}

bool AnswerComparator::compareFillInBlank(const std::string& studentAnswer,
                                         const std::string& correctAnswer) const {
    std::string normStudent = normalizeText(studentAnswer);
    std::string normCorrect = normalizeText(correctAnswer);
    
    return normStudent == normCorrect;
}

bool AnswerComparator::compareTrueFalse(int studentAnswer, int correctAnswer) const {
    return studentAnswer == correctAnswer;
}

bool AnswerComparator::compareAnswer(const Answer& studentAns, const Answer& correctAns) const {
    // Check if same question and type
    if (studentAns.questionNumber != correctAns.questionNumber ||
        studentAns.type != correctAns.type) {
        return false;
    }
    
    switch (correctAns.type) {
        case Answer::MULTIPLE_CHOICE:
            return compareMultipleChoice(studentAns.selectedOption, correctAns.selectedOption);
            
        case Answer::FILL_IN_BLANK:
            return compareFillInBlank(studentAns.textAnswer, correctAns.textAnswer);
            
        case Answer::TRUE_FALSE:
            return compareTrueFalse(studentAns.selectedOption, correctAns.selectedOption);
            
        default:
            return false;
    }
}

void AnswerComparator::setCaseSensitive(bool sensitive) {
    caseSensitive = sensitive;
}

double AnswerComparator::calculateTextSimilarity(const std::string& text1,
                                                 const std::string& text2) const {
    std::string norm1 = normalizeText(text1);
    std::string norm2 = normalizeText(text2);
    
    if (norm1.empty() && norm2.empty()) {
        return 1.0;
    }
    
    if (norm1.empty() || norm2.empty()) {
        return 0.0;
    }
    
    // Simple Levenshtein distance-based similarity
    size_t len1 = norm1.length();
    size_t len2 = norm2.length();
    
    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));
    
    for (size_t i = 0; i <= len1; i++) {
        dp[i][0] = i;
    }
    
    for (size_t j = 0; j <= len2; j++) {
        dp[0][j] = j;
    }
    
    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            size_t cost = (norm1[i - 1] == norm2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,      // deletion
                dp[i][j - 1] + 1,      // insertion
                dp[i - 1][j - 1] + cost // substitution
            });
        }
    }
    
    size_t distance = dp[len1][len2];
    size_t maxLen = std::max(len1, len2);
    
    double similarity = 1.0 - (static_cast<double>(distance) / maxLen);
    
    return std::max(0.0, similarity);
}
