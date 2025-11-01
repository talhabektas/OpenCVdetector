#include "FileWriter.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

FileWriter::FileWriter() {
}

std::string FileWriter::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string FileWriter::createTimestampedFilename(
    const std::string& prefix,
    const std::string& extension) const {
    
    return prefix + "_" + getCurrentTimestamp() + extension;
}

bool FileWriter::saveResultsToText(
    const std::string& filename,
    const ExamScore& score,
    const std::string& studentName,
    const std::string& examName) const {
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Dosya oluşturulamadı: " << filename << std::endl;
        return false;
    }
    
    file << "====================================\n";
    file << "        SINAV SONUÇ RAPORU\n";
    file << "====================================\n\n";
    
    if (!examName.empty()) {
        file << "Sınav: " << examName << "\n";
    }
    
    if (!studentName.empty()) {
        file << "Öğrenci: " << studentName << "\n";
    }
    
    file << "Tarih: " << getCurrentTimestamp() << "\n\n";
    
    file << "------------------------------------\n";
    file << "ÖZET\n";
    file << "------------------------------------\n";
    file << "Toplam Soru:     " << score.totalQuestions << "\n";
    file << "Doğru:           " << score.correctAnswers << "\n";
    file << "Yanlış:          " << score.incorrectAnswers << "\n";
    file << "Boş:             " << score.unanswered << "\n";
    file << "Ham Puan:        " << std::fixed << std::setprecision(2) 
         << score.rawScore << "\n";
    file << "Yüzde:           %" << std::fixed << std::setprecision(2) 
         << score.percentageScore << "\n\n";
    
    file << "------------------------------------\n";
    file << "DETAYLI SONUÇLAR\n";
    file << "------------------------------------\n";
    
    for (const auto& result : score.questionResults) {
        file << "Soru " << std::setw(2) << result.questionNumber << ": ";
        
        if (result.isCorrect) {
            file << "DOĞRU";
        } else if (result.partialCredit > 0.0) {
            file << "KISMEN DOĞRU (" 
                 << std::fixed << std::setprecision(0) 
                 << (result.partialCredit * 100) << "%)";
        } else {
            file << "YANLIŞ";
        }
        
        if (result.correctAnswer.type == Answer::FILL_IN_BLANK) {
            file << "\n   Öğrenci Cevabı: \"" << result.studentAnswer.textAnswer << "\"";
            file << "\n   Doğru Cevap:    \"" << result.correctAnswer.textAnswer << "\"";
        }
        
        file << "\n";
    }
    
    file << "====================================\n";
    
    file.close();
    std::cout << "Sonuçlar kaydedildi: " << filename << std::endl;
    return true;
}

bool FileWriter::saveResultsToCSV(
    const std::string& filename,
    const ExamScore& score,
    const std::string& studentName) const {
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "CSV dosyası oluşturulamadı: " << filename << std::endl;
        return false;
    }
    
    // Header
    file << "Öğrenci,Soru,Durum,Kısmi Puan,Öğrenci Cevabı,Doğru Cevap\n";
    
    // Data rows
    for (const auto& result : score.questionResults) {
        file << (studentName.empty() ? "Bilinmiyor" : studentName) << ",";
        file << result.questionNumber << ",";
        file << (result.isCorrect ? "DOGRU" : "YANLIS") << ",";
        file << std::fixed << std::setprecision(2) << result.partialCredit << ",";
        
        // Student answer
        if (result.studentAnswer.type == Answer::FILL_IN_BLANK) {
            file << "\"" << result.studentAnswer.textAnswer << "\",";
        } else {
            file << result.studentAnswer.selectedOption << ",";
        }
        
        // Correct answer
        if (result.correctAnswer.type == Answer::FILL_IN_BLANK) {
            file << "\"" << result.correctAnswer.textAnswer << "\"";
        } else {
            file << result.correctAnswer.selectedOption;
        }
        
        file << "\n";
    }
    
    file.close();
    std::cout << "CSV kaydedildi: " << filename << std::endl;
    return true;
}

bool FileWriter::saveResultImage(
    const std::string& filename,
    const cv::Mat& image) const {
    
    if (image.empty()) {
        std::cerr << "Boş görüntü kaydedilemez!" << std::endl;
        return false;
    }
    
    try {
        cv::imwrite(filename, image);
        std::cout << "Sonuç görüntüsü kaydedildi: " << filename << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "Görüntü kaydetme hatası: " << e.what() << std::endl;
        return false;
    }
}
