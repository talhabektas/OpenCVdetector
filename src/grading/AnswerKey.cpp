#include "AnswerKey.h"
#include <fstream>
#include <iostream>

AnswerKey::AnswerKey() {
}

void AnswerKey::addMultipleChoiceAnswer(int questionNum, int correctOption) {
    Answer ans;
    ans.questionNumber = questionNum;
    ans.type = Answer::MULTIPLE_CHOICE;
    ans.selectedOption = correctOption;
    ans.textAnswer = "";
    
    answers[questionNum] = ans;
}

void AnswerKey::addFillInBlankAnswer(int questionNum, const std::string& correctText) {
    Answer ans;
    ans.questionNumber = questionNum;
    ans.type = Answer::FILL_IN_BLANK;
    ans.selectedOption = -1;
    ans.textAnswer = correctText;
    
    answers[questionNum] = ans;
}

void AnswerKey::addTrueFalseAnswer(int questionNum, bool isTrue) {
    Answer ans;
    ans.questionNumber = questionNum;
    ans.type = Answer::TRUE_FALSE;
    ans.selectedOption = isTrue ? 0 : 1; // 0 = True, 1 = False
    ans.textAnswer = "";
    
    answers[questionNum] = ans;
}

Answer AnswerKey::getAnswer(int questionNum) const {
    auto it = answers.find(questionNum);
    if (it != answers.end()) {
        return it->second;
    }
    return Answer(); // Return empty answer if not found
}

bool AnswerKey::hasAnswer(int questionNum) const {
    return answers.find(questionNum) != answers.end();
}

int AnswerKey::getTotalQuestions() const {
    return static_cast<int>(answers.size());
}

bool AnswerKey::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cevap anahtarı dosyası açılamadı: " << filename << std::endl;
        return false;
    }
    
    clear();
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse line: "questionNum,type,answer"
        // Example: "1,MC,2" or "5,FILL,Istanbul"
        size_t firstComma = line.find(',');
        size_t secondComma = line.find(',', firstComma + 1);
        
        if (firstComma == std::string::npos || secondComma == std::string::npos) {
            continue;
        }
        
        int questionNum = std::stoi(line.substr(0, firstComma));
        std::string type = line.substr(firstComma + 1, secondComma - firstComma - 1);
        std::string answer = line.substr(secondComma + 1);
        
        if (type == "MC") {
            addMultipleChoiceAnswer(questionNum, std::stoi(answer));
        } else if (type == "FILL") {
            addFillInBlankAnswer(questionNum, answer);
        } else if (type == "TF") {
            addTrueFalseAnswer(questionNum, answer == "T" || answer == "1");
        }
    }
    
    file.close();
    std::cout << answers.size() << " cevap yüklendi" << std::endl;
    return true;
}

bool AnswerKey::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Dosya oluşturulamadı: " << filename << std::endl;
        return false;
    }
    
    file << "# Cevap Anahtarı\n";
    file << "# Format: questionNum,type,answer\n\n";
    
    for (const auto& pair : answers) {
        const Answer& ans = pair.second;
        file << ans.questionNumber << ",";
        
        switch (ans.type) {
            case Answer::MULTIPLE_CHOICE:
                file << "MC," << ans.selectedOption;
                break;
            case Answer::FILL_IN_BLANK:
                file << "FILL," << ans.textAnswer;
                break;
            case Answer::TRUE_FALSE:
                file << "TF," << (ans.selectedOption == 0 ? "T" : "F");
                break;
        }
        
        file << "\n";
    }
    
    file.close();
    return true;
}

void AnswerKey::clear() {
    answers.clear();
}
