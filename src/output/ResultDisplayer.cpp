#include "ResultDisplayer.h"
#include "SheetStructureAnalyzer.h"
#include <iostream>
#include <iomanip>

ResultDisplayer::ResultDisplayer() {
}

void ResultDisplayer::displayScoreSummary(const ExamScore& score) const {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "           SINAV SONUÇLARI" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Toplam Soru Sayısı:  " << score.totalQuestions << std::endl;
    std::cout << "Doğru Cevaplar:      " << score.correctAnswers << std::endl;
    std::cout << "Yanlış Cevaplar:     " << score.incorrectAnswers << std::endl;
    std::cout << "Boş Bırakılan:       " << score.unanswered << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "Ham Puan:            " << score.rawScore << std::endl;
    std::cout << "Yüzde Puan:          %" << score.percentageScore << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

void ResultDisplayer::displayDetailedResults(const ExamScore& score) const {
    std::cout << "\nDetaylı Sonuçlar:" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (const auto& result : score.questionResults) {
        std::cout << "Soru " << std::setw(2) << result.questionNumber << ": ";
        
        if (result.isCorrect) {
            std::cout << "✓ DOĞRU";
        } else if (result.partialCredit > 0.0) {
            std::cout << "~ KISMEN DOĞRU (" 
                     << std::fixed << std::setprecision(0) 
                     << (result.partialCredit * 100) << "%)";
        } else {
            std::cout << "✗ YANLIŞ";
        }
        
        // Show answers for fill-in-the-blank
        if (result.correctAnswer.type == Answer::FILL_IN_BLANK) {
            std::cout << "\n   Öğrenci: \"" << result.studentAnswer.textAnswer << "\"";
            std::cout << "\n   Doğru:   \"" << result.correctAnswer.textAnswer << "\"";
        }
        
        std::cout << std::endl;
    }
    
    std::cout << std::string(70, '-') << std::endl;
}

void ResultDisplayer::drawResultMark(
    cv::Mat& image,
    const cv::Rect& region,
    bool isCorrect) const {
    
    cv::Point center(region.x + region.width - 30, region.y + region.height / 2);
    int radius = 15;
    
    if (isCorrect) {
        // Green checkmark
        cv::circle(image, center, radius, cv::Scalar(0, 255, 0), 2);
        cv::line(image, 
                cv::Point(center.x - 7, center.y),
                cv::Point(center.x - 3, center.y + 7),
                cv::Scalar(0, 255, 0), 3);
        cv::line(image,
                cv::Point(center.x - 3, center.y + 7),
                cv::Point(center.x + 7, center.y - 7),
                cv::Scalar(0, 255, 0), 3);
    } else {
        // Red X mark
        cv::circle(image, center, radius, cv::Scalar(0, 0, 255), 2);
        cv::line(image,
                cv::Point(center.x - 7, center.y - 7),
                cv::Point(center.x + 7, center.y + 7),
                cv::Scalar(0, 0, 255), 3);
        cv::line(image,
                cv::Point(center.x + 7, center.y - 7),
                cv::Point(center.x - 7, center.y + 7),
                cv::Scalar(0, 0, 255), 3);
    }
}

void ResultDisplayer::drawScoreBox(cv::Mat& image, const ExamScore& score) const {
    int boxWidth = 300;
    int boxHeight = 150;
    int boxX = image.cols - boxWidth - 20;
    int boxY = 20;
    
    // Draw background rectangle
    cv::rectangle(image,
                 cv::Point(boxX, boxY),
                 cv::Point(boxX + boxWidth, boxY + boxHeight),
                 cv::Scalar(255, 255, 255),
                 -1);
    
    // Draw border
    cv::rectangle(image,
                 cv::Point(boxX, boxY),
                 cv::Point(boxX + boxWidth, boxY + boxHeight),
                 cv::Scalar(0, 0, 0),
                 2);
    
    // Draw text
    int textY = boxY + 30;
    cv::putText(image, "SONUC",
               cv::Point(boxX + 100, textY),
               cv::FONT_HERSHEY_SIMPLEX, 0.8,
               cv::Scalar(0, 0, 0), 2);
    
    textY += 35;
    cv::putText(image,
               "Dogru: " + std::to_string(score.correctAnswers),
               cv::Point(boxX + 20, textY),
               cv::FONT_HERSHEY_SIMPLEX, 0.6,
               cv::Scalar(0, 150, 0), 2);
    
    textY += 30;
    cv::putText(image,
               "Yanlis: " + std::to_string(score.incorrectAnswers),
               cv::Point(boxX + 20, textY),
               cv::FONT_HERSHEY_SIMPLEX, 0.6,
               cv::Scalar(0, 0, 200), 2);
    
    textY += 30;
    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Puan: %.1f%%", score.percentageScore);
    cv::putText(image, scoreText,
               cv::Point(boxX + 20, textY),
               cv::FONT_HERSHEY_SIMPLEX, 0.7,
               cv::Scalar(0, 0, 0), 2);
}

cv::Mat ResultDisplayer::createVisualResults(
    const cv::Mat& examImage,
    const ExamScore& score,
    const std::vector<QuestionRegion>& regions) const {
    
    cv::Mat resultImage = examImage.clone();
    
    // Ensure color image
    if (resultImage.channels() == 1) {
        cv::cvtColor(resultImage, resultImage, cv::COLOR_GRAY2BGR);
    }
    
    // Draw marks for each question
    for (const auto& result : score.questionResults) {
        // Find corresponding region
        for (const auto& region : regions) {
            if (region.questionNumber == result.questionNumber) {
                drawResultMark(resultImage, region.region, result.isCorrect);
                break;
            }
        }
    }
    
    // Draw score box
    drawScoreBox(resultImage, score);
    
    return resultImage;
}

void ResultDisplayer::displayInWindow(
    const std::string& windowName,
    const cv::Mat& resultsImage,
    int waitTime) const {
    
    if (resultsImage.empty()) {
        std::cerr << "Boş görüntü gösterilemez!" << std::endl;
        return;
    }
    
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::imshow(windowName, resultsImage);
    
    if (waitTime >= 0) {
        cv::waitKey(waitTime);
    }
}

void ResultDisplayer::displayStatistics(const std::map<std::string, double>& stats) const {
    std::cout << "\nİstatistikler:" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    for (const auto& pair : stats) {
        std::cout << std::left << std::setw(25) << pair.first << ": "
                 << std::fixed << std::setprecision(2) << pair.second << std::endl;
    }
    
    std::cout << std::string(50, '-') << std::endl;
}
