/**
 * SINAV DEĞERLENDİRME SİSTEMİ
 * 
 * Sınav Formatı:
 * 1. El Yazısı Soruları (5 soru x 12 puan = 60 puan)
 * 2. True/False Soruları (2 soru x 10 puan = 20 puan)  
 * 3. Çoktan Seçmeli (2 soru x 10 puan = 20 puan)
 * TOPLAM: 100 puan
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

// ==================== CEVAP ANAHTARI ====================

struct Answer {
    std::string text;
    std::vector<std::string> alternatives; // Alternatif cevaplar
    int points;
};

// El Yazısı Soruları (5 soru x 12 puan)
std::map<int, Answer> handwritingAnswers = {
    {1, {"ankara", {}, 12}},
    {2, {"mustafa kemal atatürk", {"mustafa kemal ataturk", "m.kemal atatürk", "atatürk"}, 12}},
    {3, {"avrupa,asya", {"avrupa ve asya", "asya,avrupa", "asya ve avrupa", "avrupa asya"}, 12}},
    {4, {"arjantin", {"argentina"}, 12}},
    {5, {"sakarya", {}, 12}}
};

// True/False Soruları (2 soru x 10 puan)
std::map<int, Answer> trueFalseAnswers = {
    {1, {"false", {"f", "yanlış", "yanlis"}, 10}},
    {2, {"true", {"t", "doğru", "dogru"}, 10}}
};

// Çoktan Seçmeli Sorular (2 soru x 10 puan)
std::map<int, Answer> multipleChoiceAnswers = {
    {1, {"c", {"c)", "6", "c-)6", "c-6"}, 10}},
    {2, {"d", {"d)", "ai tool", "d-)ai tool", "d-ai tool"}, 10}}
};

// ==================== YARDIMCI FONKSİYONLAR ====================

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Türkçe karakterler
    size_t pos = 0;
    while ((pos = result.find("İ", pos)) != std::string::npos) {
        result.replace(pos, 2, "i");
        pos += 1;
    }
    while ((pos = result.find("I", pos)) != std::string::npos) {
        result.replace(pos, 1, "ı");
        pos += 1;
    }
    while ((pos = result.find("Ş", pos)) != std::string::npos) {
        result.replace(pos, 2, "ş");
        pos += 1;
    }
    while ((pos = result.find("Ğ", pos)) != std::string::npos) {
        result.replace(pos, 2, "ğ");
        pos += 1;
    }
    while ((pos = result.find("Ü", pos)) != std::string::npos) {
        result.replace(pos, 2, "ü");
        pos += 1;
    }
    while ((pos = result.find("Ö", pos)) != std::string::npos) {
        result.replace(pos, 2, "ö");
        pos += 1;
    }
    while ((pos = result.find("Ç", pos)) != std::string::npos) {
        result.replace(pos, 2, "ç");
        pos += 1;
    }
    
    return result;
}

std::string cleanText(const std::string& text) {
    std::string cleaned = toLowerCase(text);
    
    // Boşlukları ve noktalama işaretlerini temizle
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(),
                  [](char c) { return std::isspace(c) || c == '.' || c == ',' || c == '-' || c == ')'; }),
                  cleaned.end());
    
    return cleaned;
}

bool compareAnswer(const std::string& studentAnswer, const Answer& correctAnswer) {
    std::string cleaned = cleanText(studentAnswer);
    
    // Ana cevapla karşılaştır
    if (cleaned.find(cleanText(correctAnswer.text)) != std::string::npos) {
        return true;
    }
    
    // Alternatif cevaplarla karşılaştır
    for (const auto& alt : correctAnswer.alternatives) {
        if (cleaned.find(cleanText(alt)) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// True/False için X veya ✓ tespiti
bool detectCheckMark(const cv::Mat& roi) {
    cv::Mat gray;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }
    
    // Threshold
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // Non-zero pixel sayısı
    int nonZero = cv::countNonZero(binary);
    int total = binary.rows * binary.cols;
    
    double fillRatio = (double)nonZero / total;
    
    // %15'ten fazla dolu ise işaretli
    return fillRatio > 0.15;
}

// Çoktan seçmeli için dolu balon tespiti
bool isBubbleFilled(const cv::Mat& roi) {
    cv::Mat gray;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }
    
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    int nonZero = cv::countNonZero(binary);
    int total = binary.rows * binary.cols;
    
    double fillRatio = (double)nonZero / total;
    
    return fillRatio > 0.50; // %50'den fazla dolu
}

std::string performOCR(const cv::Mat& roi) {
    std::string filename = "temp_ocr_region.jpg";
    cv::imwrite(filename, roi);
    
    std::string cmd = "tesseract " + filename + " stdout -l tur --psm 7 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    
    std::string result = "";
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        pclose(pipe);
    }
    
    // Temizle
    result.erase(0, result.find_first_not_of(" \n\r\t"));
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    
    std::remove(filename.c_str());
    return result;
}

// ==================== GLOBAL DEĞİŞKENLER ====================

cv::Mat originalImage;
cv::Mat displayImage;
std::vector<cv::Point2f> perspectiveCorners;
std::vector<cv::Rect> handwritingRegions;
std::vector<cv::Rect> trueFalseRegions;
std::vector<cv::Rect> multipleChoiceRegions;

bool selectingPerspective = false;
bool selectingHandwriting = false;
bool selectingTrueFalse = false;
bool selectingMultipleChoice = false;

cv::Point dragStart;
cv::Rect currentRect;
bool dragging = false;

// ==================== MOUSE CALLBACK ====================

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (selectingPerspective) {
        if (event == cv::EVENT_LBUTTONDOWN && perspectiveCorners.size() < 4) {
            perspectiveCorners.push_back(cv::Point2f(x, y));
            std::cout << "📍 Köşe " << perspectiveCorners.size() << ": (" 
                      << x << ", " << y << ")" << std::endl;
        }
    }
    else if (selectingHandwriting || selectingTrueFalse || selectingMultipleChoice) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            dragStart = cv::Point(x, y);
            dragging = true;
        }
        else if (event == cv::EVENT_MOUSEMOVE && dragging) {
            currentRect = cv::Rect(
                std::min(dragStart.x, x),
                std::min(dragStart.y, y),
                std::abs(x - dragStart.x),
                std::abs(y - dragStart.y)
            );
        }
        else if (event == cv::EVENT_LBUTTONUP && dragging) {
            dragging = false;
            if (currentRect.width > 30 && currentRect.height > 10) {
                if (selectingHandwriting) {
                    handwritingRegions.push_back(currentRect);
                    std::cout << "✍️  El yazısı bölgesi " << handwritingRegions.size() 
                              << " eklendi" << std::endl;
                }
                else if (selectingTrueFalse) {
                    trueFalseRegions.push_back(currentRect);
                    std::cout << "☑️  True/False bölgesi " << trueFalseRegions.size() 
                              << " eklendi" << std::endl;
                }
                else if (selectingMultipleChoice) {
                    multipleChoiceRegions.push_back(currentRect);
                    std::cout << "⭕ Çoktan seçmeli bölgesi " << multipleChoiceRegions.size() 
                              << " eklendi" << std::endl;
                }
            }
            currentRect = cv::Rect();
        }
    }
}

// ==================== PERSPEKTİF DÜZELTİVE ====================

cv::Mat applyPerspectiveCorrection(const cv::Mat& image, const std::vector<cv::Point2f>& corners) {
    if (corners.size() != 4) {
        return image.clone();
    }
    
    float width = 800;
    float height = 1131;
    
    std::vector<cv::Point2f> dst = {
        cv::Point2f(0, 0),
        cv::Point2f(width - 1, 0),
        cv::Point2f(width - 1, height - 1),
        cv::Point2f(0, height - 1)
    };
    
    cv::Mat transformMatrix = cv::getPerspectiveTransform(corners, dst);
    cv::Mat warped;
    cv::warpPerspective(image, warped, transformMatrix, cv::Size(width, height));
    
    return warped;
}

// ==================== GÖRÜNTÜ GÜNCELLEME ====================

void updateDisplay() {
    displayImage = originalImage.clone();
    
    // Perspektif köşeleri
    for (size_t i = 0; i < perspectiveCorners.size(); i++) {
        cv::circle(displayImage, perspectiveCorners[i], 8, cv::Scalar(0, 0, 255), -1);
        cv::putText(displayImage, std::to_string(i + 1),
                   cv::Point(perspectiveCorners[i].x + 10, perspectiveCorners[i].y - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
    }
    
    if (perspectiveCorners.size() > 1) {
        for (size_t i = 0; i < perspectiveCorners.size() - 1; i++) {
            cv::line(displayImage, perspectiveCorners[i], perspectiveCorners[i + 1],
                    cv::Scalar(0, 255, 255), 2);
        }
        if (perspectiveCorners.size() == 4) {
            cv::line(displayImage, perspectiveCorners[3], perspectiveCorners[0],
                    cv::Scalar(0, 255, 255), 2);
        }
    }
    
    // El yazısı bölgeleri (Yeşil)
    for (size_t i = 0; i < handwritingRegions.size(); i++) {
        cv::rectangle(displayImage, handwritingRegions[i], cv::Scalar(0, 255, 0), 2);
        std::string label = "H" + std::to_string(i + 1);
        cv::putText(displayImage, label,
                   cv::Point(handwritingRegions[i].x, handwritingRegions[i].y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    }
    
    // True/False bölgeleri (Mavi)
    for (size_t i = 0; i < trueFalseRegions.size(); i++) {
        cv::rectangle(displayImage, trueFalseRegions[i], cv::Scalar(255, 0, 0), 2);
        std::string label = "TF" + std::to_string(i + 1);
        cv::putText(displayImage, label,
                   cv::Point(trueFalseRegions[i].x, trueFalseRegions[i].y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 0, 0), 2);
    }
    
    // Çoktan seçmeli bölgeleri (Kırmızı)
    for (size_t i = 0; i < multipleChoiceRegions.size(); i++) {
        cv::rectangle(displayImage, multipleChoiceRegions[i], cv::Scalar(0, 0, 255), 2);
        std::string label = "MC" + std::to_string(i + 1);
        cv::putText(displayImage, label,
                   cv::Point(multipleChoiceRegions[i].x, multipleChoiceRegions[i].y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
    }
    
    // Şu anki sürükleme
    if (dragging && currentRect.width > 0) {
        cv::Scalar color = selectingHandwriting ? cv::Scalar(0, 255, 0) :
                          selectingTrueFalse ? cv::Scalar(255, 0, 0) :
                          cv::Scalar(0, 0, 255);
        cv::rectangle(displayImage, currentRect, color, 2);
    }
}

// ==================== MAIN ====================

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Kullanım: " << argv[0] << " <sınav_kağıdı.jpg>" << std::endl;
        return 1;
    }
    
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   SINAV DEĞERLENDİRME SİSTEMİ               ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝\n" << std::endl;
    
    originalImage = cv::imread(argv[1]);
    
    if (originalImage.empty()) {
        std::cerr << "❌ Sınav kağıdı yüklenemedi!" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Sınav kağıdı yüklendi" << std::endl;
    std::cout << "📐 Boyut: " << originalImage.cols << "x" << originalImage.rows << "\n" << std::endl;
    
    // Görüntüyü küçült
    float scale = 1.0;
    if (originalImage.cols > 1200) {
        scale = 1200.0 / originalImage.cols;
        cv::resize(originalImage, originalImage, cv::Size(), scale, scale);
    }
    
    displayImage = originalImage.clone();
    
    std::cout << "🎮 KONTROLLER:" << std::endl;
    std::cout << "   P = Perspektif düzeltme (4 köşe)" << std::endl;
    std::cout << "   H = El yazısı bölgeleri seç (5 soru)" << std::endl;
    std::cout << "   T = True/False bölgeleri seç (2 soru)" << std::endl;
    std::cout << "   M = Çoktan seçmeli bölgeleri seç (2 soru)" << std::endl;
    std::cout << "   R = Sıfırla" << std::endl;
    std::cout << "   ENTER = Sınavı değerlendir" << std::endl;
    std::cout << "   ESC = Çıkış" << std::endl;
    std::cout << "\n──────────────────────────────────────────────\n" << std::endl;
    
    cv::namedWindow("Sinav Degerlendirme", cv::WINDOW_NORMAL);
    cv::setMouseCallback("Sinav Degerlendirme", onMouse, nullptr);
    
    while (true) {
        updateDisplay();
        
        std::string info;
        if (selectingPerspective) {
            info = "PERSPEKTIF: " + std::to_string(perspectiveCorners.size()) + "/4";
        } else if (selectingHandwriting) {
            info = "EL YAZISI: " + std::to_string(handwritingRegions.size()) + "/5 soru";
        } else if (selectingTrueFalse) {
            info = "TRUE/FALSE: " + std::to_string(trueFalseRegions.size()) + "/2 soru";
        } else if (selectingMultipleChoice) {
            info = "COKTAN SECMELI: " + std::to_string(multipleChoiceRegions.size()) + "/2 soru";
        } else {
            info = "P:Perspektif | H:ElYazisi | T:TrueFalse | M:Secmeli | ENTER:Degerlendir";
        }
        
        cv::putText(displayImage, info, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0), 2);
        
        cv::imshow("Sinav Degerlendirme", displayImage);
        
        int key = cv::waitKey(10);
        
        if (key == 27) { // ESC
            break;
        }
        else if (key == 'p' || key == 'P') {
            selectingPerspective = true;
            selectingHandwriting = selectingTrueFalse = selectingMultipleChoice = false;
            perspectiveCorners.clear();
            std::cout << "\n🔵 PERSPEKTİF MODU - 4 köşeye tıklayın\n" << std::endl;
        }
        else if (key == 'h' || key == 'H') {
            selectingHandwriting = true;
            selectingPerspective = selectingTrueFalse = selectingMultipleChoice = false;
            std::cout << "\n✍️  EL YAZISI MODU - 5 cevap bölgesini seçin\n" << std::endl;
        }
        else if (key == 't' || key == 'T') {
            selectingTrueFalse = true;
            selectingPerspective = selectingHandwriting = selectingMultipleChoice = false;
            std::cout << "\n☑️  TRUE/FALSE MODU - 2 soru için TRUE ve FALSE kutularını seçin\n" << std::endl;
        }
        else if (key == 'm' || key == 'M') {
            selectingMultipleChoice = true;
            selectingPerspective = selectingHandwriting = selectingTrueFalse = false;
            std::cout << "\n⭕ ÇOKTAN SEÇMELİ MODU - Her soru için A,B,C,D şıklarını seçin\n" << std::endl;
        }
        else if (key == 'r' || key == 'R') {
            selectingPerspective = selectingHandwriting = selectingTrueFalse = selectingMultipleChoice = false;
            perspectiveCorners.clear();
            handwritingRegions.clear();
            trueFalseRegions.clear();
            multipleChoiceRegions.clear();
            std::cout << "\n♻️  Sıfırlandı\n" << std::endl;
        }
        else if (key == 13 || key == 10) { // ENTER
            // Perspektif düzeltme
            cv::Mat workingImage = originalImage.clone();
            
            if (perspectiveCorners.size() == 4) {
                std::cout << "\n🔄 Perspektif düzeltiliyor..." << std::endl;
                workingImage = applyPerspectiveCorrection(originalImage, perspectiveCorners);
                std::cout << "✅ Düzeltme tamamlandı\n" << std::endl;
                originalImage = workingImage.clone();
                perspectiveCorners.clear();
                selectingPerspective = false;
                cv::imshow("Sinav Degerlendirme", workingImage);
                cv::waitKey(1000);
                continue;
            }
            
            if (handwritingRegions.empty() && trueFalseRegions.empty() && multipleChoiceRegions.empty()) {
                std::cout << "\n⚠️  Önce bölgeleri seçin!\n" << std::endl;
                continue;
            }
            
            // DEĞERLENDİRME BAŞLIYOR
            std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
            std::cout << "║   SINAV DEĞERLENDİRİLİYOR...                ║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════╝\n" << std::endl;
            
            int totalScore = 0;
            cv::Mat resultImage = workingImage.clone();
            
            // 1. EL YAZISI SORULARI (5 x 12 puan = 60 puan)
            std::cout << "═══════════════════════════════════════════════" << std::endl;
            std::cout << "EL YAZISI SORULARI (12 puan/soru)" << std::endl;
            std::cout << "═══════════════════════════════════════════════\n" << std::endl;
            
            for (size_t i = 0; i < std::min(handwritingRegions.size(), (size_t)5); i++) {
                cv::Mat roi = workingImage(handwritingRegions[i]);
                std::string ocrText = performOCR(roi);
                
                int questionNum = i + 1;
                bool isCorrect = false;
                int points = 0;
                
                if (handwritingAnswers.count(questionNum)) {
                    isCorrect = compareAnswer(ocrText, handwritingAnswers[questionNum]);
                    if (isCorrect) {
                        points = handwritingAnswers[questionNum].points;
                        totalScore += points;
                    }
                }
                
                cv::Scalar color = isCorrect ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                cv::rectangle(resultImage, handwritingRegions[i], color, 3);
                
                std::cout << "Soru " << questionNum << ":" << std::endl;
                std::cout << "  Okunan: \"" << ocrText << "\"" << std::endl;
                std::cout << "  Beklenen: \"" << handwritingAnswers[questionNum].text << "\"" << std::endl;
                std::cout << "  Sonuç: " << (isCorrect ? "✅ DOĞRU" : "❌ YANLIŞ") 
                          << " (" << points << " puan)" << std::endl << std::endl;
            }
            
            // 2. TRUE/FALSE SORULARI (2 x 10 puan = 20 puan)
            std::cout << "═══════════════════════════════════════════════" << std::endl;
            std::cout << "TRUE/FALSE SORULARI (10 puan/soru)" << std::endl;
            std::cout << "═══════════════════════════════════════════════\n" << std::endl;
            
            // Her soru için 2 kutu bekliyoruz: TRUE ve FALSE
            for (size_t i = 0; i < trueFalseRegions.size() / 2; i++) {
                int trueBoxIdx = i * 2;
                int falseBoxIdx = i * 2 + 1;
                
                if (falseBoxIdx >= trueFalseRegions.size()) break;
                
                bool trueChecked = detectCheckMark(workingImage(trueFalseRegions[trueBoxIdx]));
                bool falseChecked = detectCheckMark(workingImage(trueFalseRegions[falseBoxIdx]));
                
                int questionNum = i + 1;
                std::string studentAnswer = trueChecked ? "true" : "false";
                std::string correctAnswer = trueFalseAnswers[questionNum].text;
                
                bool isCorrect = (studentAnswer == correctAnswer);
                int points = isCorrect ? trueFalseAnswers[questionNum].points : 0;
                totalScore += points;
                
                cv::Scalar color = isCorrect ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                cv::rectangle(resultImage, trueFalseRegions[trueBoxIdx], color, 2);
                cv::rectangle(resultImage, trueFalseRegions[falseBoxIdx], color, 2);
                
                std::cout << "Soru " << questionNum << ":" << std::endl;
                std::cout << "  TRUE: " << (trueChecked ? "✓" : "☐") << std::endl;
                std::cout << "  FALSE: " << (falseChecked ? "✓" : "☐") << std::endl;
                std::cout << "  Cevap: " << studentAnswer << std::endl;
                std::cout << "  Beklenen: " << correctAnswer << std::endl;
                std::cout << "  Sonuç: " << (isCorrect ? "✅ DOĞRU" : "❌ YANLIŞ") 
                          << " (" << points << " puan)" << std::endl << std::endl;
            }
            
            // 3. ÇOKTAN SEÇMELİ SORULAR (2 x 10 puan = 20 puan)
            std::cout << "═══════════════════════════════════════════════" << std::endl;
            std::cout << "ÇOKTAN SEÇMELİ SORULAR (10 puan/soru)" << std::endl;
            std::cout << "═══════════════════════════════════════════════\n" << std::endl;
            
            // Her soru için 4 şık bekliyoruz: A, B, C, D
            for (size_t i = 0; i < multipleChoiceRegions.size() / 4; i++) {
                std::string choices = "ABCD";
                std::string studentAnswer = "";
                
                for (int j = 0; j < 4; j++) {
                    int boxIdx = i * 4 + j;
                    if (boxIdx >= multipleChoiceRegions.size()) break;
                    
                    bool isFilled = isBubbleFilled(workingImage(multipleChoiceRegions[boxIdx]));
                    if (isFilled) {
                        studentAnswer += choices[j];
                    }
                }
                
                int questionNum = i + 1;
                std::string correctAnswer = multipleChoiceAnswers[questionNum].text;
                
                bool isCorrect = (toLowerCase(studentAnswer) == correctAnswer);
                int points = isCorrect ? multipleChoiceAnswers[questionNum].points : 0;
                totalScore += points;
                
                cv::Scalar color = isCorrect ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                for (int j = 0; j < 4; j++) {
                    int boxIdx = i * 4 + j;
                    if (boxIdx < multipleChoiceRegions.size()) {
                        cv::rectangle(resultImage, multipleChoiceRegions[boxIdx], color, 2);
                    }
                }
                
                std::cout << "Soru " << questionNum << ":" << std::endl;
                std::cout << "  İşaretlenen: " << (studentAnswer.empty() ? "(boş)" : studentAnswer) << std::endl;
                std::cout << "  Beklenen: " << correctAnswer << std::endl;
                std::cout << "  Sonuç: " << (isCorrect ? "✅ DOĞRU" : "❌ YANLIŞ") 
                          << " (" << points << " puan)" << std::endl << std::endl;
            }
            
            // SONUÇ
            std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
            std::cout << "║   FİNAL SONUCU                               ║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
            std::cout << "🏆 TOPLAM PUAN: " << totalScore << " / 100" << std::endl;
            
            std::string grade;
            if (totalScore >= 90) grade = "AA (Mükemmel)";
            else if (totalScore >= 85) grade = "BA (Çok İyi)";
            else if (totalScore >= 80) grade = "BB (İyi)";
            else if (totalScore >= 75) grade = "CB (Orta)";
            else if (totalScore >= 70) grade = "CC (Geçer)";
            else if (totalScore >= 65) grade = "DC (Şartlı Geçer)";
            else if (totalScore >= 60) grade = "DD (Şartlı Geçer)";
            else if (totalScore >= 50) grade = "FD (Başarısız)";
            else grade = "FF (Başarısız)";
            
            std::cout << "📊 HARF NOTU: " << grade << std::endl;
            std::cout << "══════════════════════════════════════════════\n" << std::endl;
            
            // Sonucu kaydet
            cv::putText(resultImage, "PUAN: " + std::to_string(totalScore) + "/100",
                       cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1.5,
                       cv::Scalar(0, 0, 255), 3);
            
            cv::imshow("SONUC", resultImage);
            cv::imwrite("exam_result.jpg", resultImage);
            std::cout << "💾 Sonuç kaydedildi: exam_result.jpg\n" << std::endl;
        }
        
        if (selectingPerspective && perspectiveCorners.size() == 4) {
            selectingPerspective = false;
            std::cout << "✅ 4 köşe seçildi! ENTER ile düzeltin\n" << std::endl;
        }
    }
    
    cv::destroyAllWindows();
    return 0;
}