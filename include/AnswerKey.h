#ifndef ANSWER_KEY_H
#define ANSWER_KEY_H

#include <vector>
#include <string>
#include <map>

struct Answer {
    int questionNumber;
    enum Type { MULTIPLE_CHOICE, FILL_IN_BLANK, TRUE_FALSE } type;
    int selectedOption;
    std::string textAnswer;
    
    Answer() : questionNumber(0), type(MULTIPLE_CHOICE), selectedOption(-1), textAnswer("") {}
};

class AnswerKey {
public:
    AnswerKey();
    
    void addMultipleChoiceAnswer(int questionNum, int correctOption);
    void addFillInBlankAnswer(int questionNum, const std::string& correctText);
    void addTrueFalseAnswer(int questionNum, bool isTrue);
    Answer getAnswer(int questionNum) const;
    bool hasAnswer(int questionNum) const;
    int getTotalQuestions() const;
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    void clear();

private:
    std::map<int, Answer> answers;
};

#endif
