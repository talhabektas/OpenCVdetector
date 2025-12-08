/**
 * FOTOĞRAF OKUYUCU
 * Telefondan çekilen fotoğrafları oku
 * Manuel koordinat seçimi + Bird's Eye View + OCR
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>

// Cevap anahtarı
std::map<int, std::string> answerKey = {
    {1, "Istanbul"},
    {2, "1923"},
    {3, "Ankara"},
    {4, "Mustafa Kemal"},
    {5, "Cumhuriyet"}
};

// Global değişkenler
cv::Mat originalImage;
cv::Mat displayImage;
std::vector<cv::Point2f> perspectiveCorners;
std::vector<cv::Rect> textRegions;
bool selectingPerspective = false;
bool selectingText = false;
cv::Point dragStart;
cv::Rect currentRect;
bool dragging = false;

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    size_t pos = 0;
    while ((pos = result.find("İ", pos)) != std::string::npos) {
        result.replace(pos, 2, "i");
        pos += 1;
    }
    while ((pos = result.find("I", pos)) != std::string::npos) {
        result.replace(pos, 1, "ı");
        pos += 1;
    }
    return result;
}

bool compareAnswers(const std::string& student, const std::string& correct) {
    std::string s1 = toLowerCase(student);
    std::string s2 = toLowerCase(correct);
    s1.erase(std::remove_if(s1.begin(), s1.end(), ::isspace), s1.end());
    s2.erase(std::remove_if(s2.begin(), s2.end(), ::isspace), s2.end());
    return s1 == s2;
}

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (selectingPerspective) {
        if (event == cv::EVENT_LBUTTONDOWN && perspectiveCorners.size() < 4) {
            perspectiveCorners.push_back(cv::Point2f(x, y));
            std::cout << "📍 Köşe " << perspectiveCorners.size() << ": (" 
                      << x << ", " << y << ")" << std::endl;
        }
    }
    else if (selectingText) {
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
            if (currentRect.width > 50 && currentRect.height > 20) {
                textRegions.push_back(currentRect);
                std::cout << "📦 Bölge " << textRegions.size() << " eklendi: "
                          << currentRect << std::endl;
            }
            currentRect = cv::Rect();
        }
    }
}

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

void updateDisplay() {
    displayImage = originalImage.clone();
    
    // Perspektif köşelerini çiz
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
    
    // Metin bölgelerini çiz
    for (size_t i = 0; i < textRegions.size(); i++) {
        cv::rectangle(displayImage, textRegions[i], cv::Scalar(0, 255, 0), 2);
        std::string label = "S" + std::to_string(i + 1);
        cv::putText(displayImage, label,
                   cv::Point(textRegions[i].x, textRegions[i].y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    }
    
    // Şu anki sürükleme
    if (dragging && currentRect.width > 0) {
        cv::rectangle(displayImage, currentRect, cv::Scalar(255, 0, 0), 2);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Kullanım: " << argv[0] << " <fotoğraf.jpg>" << std::endl;
        return 1;
    }
    
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   FOTOĞRAF OKUYUCU (BIRD'S EYE VIEW + OCR)  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝\n" << std::endl;
    
    // Görüntü yükle
    originalImage = cv::imread(argv[1]);
    
    if (originalImage.empty()) {
        std::cerr << "❌ Fotoğraf yüklenemedi: " << argv[1] << std::endl;
        std::cerr << "💡 HEIC ise önce JPG'ye çevirin: magick foto.heic foto.jpg" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Fotoğraf yüklendi: " << argv[1] << std::endl;
    std::cout << "📐 Boyut: " << originalImage.cols << "x" << originalImage.rows << "\n" << std::endl;
    
    // Görüntüyü küçült (ekrana sığsın)
    float scale = 1.0;
    if (originalImage.cols > 1200) {
        scale = 1200.0 / originalImage.cols;
        cv::resize(originalImage, originalImage, cv::Size(), scale, scale);
    }
    
    displayImage = originalImage.clone();
    
    std::cout << "🎮 KONTROLLER:" << std::endl;
    std::cout << "   P = Perspektif düzeltme (4 köşeye tıkla)" << std::endl;
    std::cout << "   T = Metin bölgesi seç (sürükle)" << std::endl;
    std::cout << "   R = Sıfırla" << std::endl;
    std::cout << "   ENTER = OCR yap ve oku" << std::endl;
    std::cout << "   ESC = Çıkış" << std::endl;
    std::cout << "\n──────────────────────────────────────────────\n" << std::endl;
    
    cv::namedWindow("Fotograf Okuyucu", cv::WINDOW_NORMAL);
    cv::setMouseCallback("Fotograf Okuyucu", onMouse, nullptr);
    
    while (true) {
        updateDisplay();
        
        // Bilgi metni
        std::string info;
        if (selectingPerspective) {
            info = "PERSPEKTIF MODU: " + std::to_string(perspectiveCorners.size()) + "/4 kose";
        } else if (selectingText) {
            info = "METIN SECME: Surukle ve birak (" + std::to_string(textRegions.size()) + " bolge)";
        } else {
            info = "P:Perspektif | T:Metin Sec | ENTER:Oku | ESC:Cikis";
        }
        
        cv::putText(displayImage, info, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0), 2);
        
        cv::imshow("Fotograf Okuyucu", displayImage);
        
        int key = cv::waitKey(10);
        
        if (key == 27) { // ESC
            break;
        }
        else if (key == 'p' || key == 'P') {
            selectingPerspective = true;
            selectingText = false;
            perspectiveCorners.clear();
            std::cout << "\n🔵 PERSPEKTİF MODU - 4 köşeye tıklayın:" << std::endl;
            std::cout << "   1. Sol-üst" << std::endl;
            std::cout << "   2. Sağ-üst" << std::endl;
            std::cout << "   3. Sağ-alt" << std::endl;
            std::cout << "   4. Sol-alt\n" << std::endl;
        }
        else if (key == 't' || key == 'T') {
            selectingPerspective = false;
            selectingText = true;
            std::cout << "\n📝 METİN SEÇME MODU - Sürükleyerek bölge seçin\n" << std::endl;
        }
        else if (key == 'r' || key == 'R') {
            selectingPerspective = false;
            selectingText = false;
            perspectiveCorners.clear();
            textRegions.clear();
            std::cout << "\n♻  Sıfırlandı\n" << std::endl;
        }
        else if (key == 13 || key == 10) { // ENTER
            // Perspektif düzeltme varsa uygula
            cv::Mat workingImage = originalImage.clone();
            
            if (perspectiveCorners.size() == 4) {
                std::cout << "\n🔄 Perspektif düzeltiliyor..." << std::endl;
                workingImage = applyPerspectiveCorrection(originalImage, perspectiveCorners);
                std::cout << "✅ Düzeltme tamamlandı\n" << std::endl;
                
                // Perspektif düzeltilince bölgeleri sıfırla
                textRegions.clear();
                selectingText = false;
                selectingPerspective = false;
                
                // Düzeltilmiş görüntüyü göster
                originalImage = workingImage.clone();
                cv::imshow("Fotograf Okuyucu", workingImage);
                cv::waitKey(1000);
                
                std::cout << "📝 Şimdi 'T' tuşu ile metin bölgelerini seçin\n" << std::endl;
                continue;
            }
            
            if (textRegions.empty()) {
                std::cout << "\n⚠  Önce metin bölgelerini seçin (T tuşu)\n" << std::endl;
                continue;
            }
            
            // OCR işlemi
            std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
            std::cout << "║   OCR İŞLEMİ BAŞLIYOR                       ║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════╝\n" << std::endl;
            
            cv::Mat result = workingImage.clone();
            int questionNum = 1;
            int correct = 0;
            int wrong = 0;
            
            for (const auto& region : textRegions) {
                cv::Mat roi = workingImage(region);
                std::string filename = "temp_region.jpg";
                cv::imwrite(filename, roi);
                
                // Tesseract OCR
                std::string cmd = "tesseract " + filename + " stdout -l tur --psm 7 2>/dev/null";
                FILE* pipe = popen(cmd.c_str(), "r");
                
                std::string ocrText = "";
                if (pipe) {
                    char buffer[256];
                    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                        ocrText += buffer;
                    }
                    pclose(pipe);
                }
                
                // Temizle
                ocrText.erase(0, ocrText.find_first_not_of(" \n\r\t"));
                ocrText.erase(ocrText.find_last_not_of(" \n\r\t") + 1);
                
                // Karşılaştır
                bool isCorrect = false;
                std::string expected = "";
                
                if (answerKey.count(questionNum)) {
                    expected = answerKey[questionNum];
                    isCorrect = !ocrText.empty() && compareAnswers(ocrText, expected);
                }
                
                // Görselleştir
                cv::Scalar color = ocrText.empty() ? cv::Scalar(128, 128, 128) :
                                  (isCorrect ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255));
                
                cv::rectangle(result, region, color, 3);
                
                std::string label = "S" + std::to_string(questionNum);
                cv::putText(result, label,
                           cv::Point(region.x, region.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
                
                if (!ocrText.empty()) {
                    std::string icon = isCorrect ? "✓" : "✗";
                    cv::putText(result, icon,
                               cv::Point(region.x + region.width + 10, region.y + 40),
                               cv::FONT_HERSHEY_SIMPLEX, 1.2, color, 2);
                    
                    if (isCorrect) correct++;
                    else wrong++;
                }
                
                // Konsol
                std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
                std::cout << "Soru " << questionNum << ":" << std::endl;
                std::cout << "  Okunan: ";
                if (ocrText.empty()) {
                    std::cout << "(boş)" << std::endl;
                } else {
                    std::cout << "\"" << ocrText << "\"" << std::endl;
                }
                
                if (!expected.empty()) {
                    std::cout << "  Beklenen: \"" << expected << "\"" << std::endl;
                    std::cout << "  Sonuç: " << (isCorrect ? "✅ DOĞRU" : "❌ YANLIŞ") << std::endl;
                }
                std::cout << std::endl;
                
                questionNum++;
            }
            
            // Özet
            std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
            std::cout << "║   SONUÇLAR                                   ║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
            std::cout << "Toplam Soru: " << textRegions.size() << std::endl;
            std::cout << "✅ Doğru: " << correct << std::endl;
            std::cout << "❌ Yanlış: " << wrong << std::endl;
            
            if (correct + wrong > 0) {
                int percentage = (correct * 100) / (correct + wrong);
                std::cout << "📊 Başarı: %" << percentage << std::endl;
            }
            std::cout << "══════════════════════════════════════════════\n" << std::endl;
            
            cv::imshow("SONUC", result);
            cv::imwrite("photo_result.jpg", result);
            std::cout << "💾 Sonuç kaydedildi: photo_result.jpg\n" << std::endl;
        }
        
        // Perspektif tamamlandıysa otomatik bitir
        if (selectingPerspective && perspectiveCorners.size() == 4) {
            selectingPerspective = false;
            std::cout << "✅ 4 köşe seçildi! ENTER tuşuna basarak düzeltebilirsiniz\n" << std::endl;
        }
    }
    
    cv::destroyAllWindows();
    return 0;
}