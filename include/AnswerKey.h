#ifndef ANSWER_KEY_H
#define ANSWER_KEY_H

#include <vector>
#include <string>
#include <map>

/**
 * @struct Answer
 * @brief Represents a single answer
 */
struct Answer {
    int questionNumber;
    enum Type { MULTIPLE_CHOICE, FILL_IN_BLANK, TRUE_FALSE } type;
    
    // For multiple choice and True/False
    int selectedOption; // -1 if not applicable
    
    // For fill-in-the-blank
    std::string textAnswer;
    
    Answer() : questionNumber(0), type(MULTIPLE_CHOICE), selectedOption(-1), textAnswer("") {}
};

/**
 * @class AnswerKey
 * @brief Stores correct answers for an exam
 */
class AnswerKey {
public:
    /**
     * @brief Constructor
     */
    AnswerKey();
    
    /**
     * @brief Add a multiple choice answer
     * @param questionNum Question number
     * @param correctOption Correct option (0-based index)
     */
    void addMultipleChoiceAnswer(int questionNum, int correctOption);
    
    /**
     * @brief Add a fill-in-the-blank answer
     * @param questionNum Question number
     * @param correctText Correct text answer
     */
    void addFillInBlankAnswer(int questionNum, const std::string& correctText);
    
    /**
     * @brief Add a True/False answer
     * @param questionNum Question number
     * @param isTrue True if answer is True, False if answer is False
     */
    void addTrueFalseAnswer(int questionNum, bool isTrue);
    
    /**
     * @brief Get correct answer for a question
     * @param questionNum Question number
     * @return Answer structure
     */
    Answer getAnswer(int questionNum) const;
    
    /**
     * @brief Check if answer key contains a question
     * @param questionNum Question number
     * @return true if exists, false otherwise
     */
    bool hasAnswer(int questionNum) const;
    
    /**
     * @brief Get total number of questions
     * @return Total questions
     */
    int getTotalQuestions() const;
    
    /**
     * @brief Load answer key from file
     * @param filename Path to answer key file
     * @return true if successful, false otherwise
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Save answer key to file
     * @param filename Path to save file
     * @return true if successful, false otherwise
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Clear all answers
     */
    void clear();

private:
    std::map<int, Answer> answers;
};

#endif // ANSWER_KEY_H
