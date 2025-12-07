/**
 * CANLI EL YAZISI OKUYUCU
 * Kameradan canlı görüntü al, yazıları otomatik bul ve oku
 * BIRD'S EYE VIEW desteği ile perspektif düzeltme
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

// Global değişkenler - perspektif düzeltme için
std::vector<cv::Point2f> selectedCorners;
bool perspectiveMode = false;
cv::Mat perspectiveCorrectedImage;

// Mouse callback - köşe seçimi için
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN && perspectiveMode) {
        if (selectedCorners.size() < 4) {
            selectedCorners.push_back(cv::Point2f(x, y));
            std::cout << "📍 Köşe " << selectedCorners.size() << " seçildi: (" 
                      << x << ", " << y << ")" << std::endl;
            
            if (selectedCorners.size() == 4) {
                std::cout << "✅ 4 köşe tamamlandı! Perspektif düzeltiliyor..." << std::endl;
            }
        }
    }
}

// Perspektif düzeltme fonksiyonu
cv::Mat applyPerspectiveCorrection(const cv::Mat& image, const std::vector<cv::Point2f>& corners) {
    if (corners.size() != 4) {
        return image.clone();
    }
    
    // Hedef noktalar (A4 oranı: 210mm x 297mm ≈ 1:1.414)
    float width = 800;
    float height = 1131; // A4 oranı
    
    std::vector<cv::Point2f> dst = {
        cv::Point2f(0, 0),
        cv::Point2f(width - 1, 0),
        cv::Point2f(width - 1, height - 1),
        cv::Point2f(0, height - 1)
    };
    
    // Perspektif dönüşüm matrisi
    cv::Mat transformMatrix = cv::getPerspectiveTransform(corners, dst);
    
    // Dönüşümü uygula
    cv::Mat warped;
    cv::warpPerspective(image, warped, transformMatrix, cv::Size(width, height));
    
    return warped;
}

struct TextBlock {
    cv::Rect region;
    std::string text;
    int confidence;
    bool isCorrect;
    std::string expectedAnswer;
};

std::vector<cv::Rect> findTextRegions(const cv::Mat& image) {
    std::vector<cv::Rect> regions;
    
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // Adaptive threshold - yazıları bul
    cv::Mat binary;
    cv::adaptiveThreshold(gray, binary, 255,
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                         cv::THRESH_BINARY_INV, 31, 10);
    
    // Gürültü temizle
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    
    // Kelimeleri birleştir
    kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(40, 10));
    cv::dilate(binary, binary, kernel, cv::Point(-1, -1), 1);
    
    // Konturları bul
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Filtrele
    for (const auto& contour : contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        
        // Minimum boyut ve aspect ratio
        if (bbox.width > 80 && bbox.height > 25 && bbox.height < 300) {
            float aspect = (float)bbox.width / bbox.height;
            if (aspect > 1.5 && bbox.width < image.cols * 0.8) {
                // Padding
                int pad = 10;
                bbox.x = std::max(0, bbox.x - pad);
                bbox.y = std::max(0, bbox.y - pad);
                bbox.width = std::min(image.cols - bbox.x, bbox.width + 2 * pad);
                bbox.height = std::min(image.rows - bbox.y, bbox.height + 2 * pad);
                
                regions.push_back(bbox);
            }
        }
    }
    
    // Y koordinatına göre sırala
    std::sort(regions.begin(), regions.end(),
              [](const cv::Rect& a, const cv::Rect& b) {
                  return a.y < b.y;
              });
    
    return regions;
}

std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    // Türkçe İ -> i
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
    
    // Boşlukları temizle
    s1.erase(std::remove_if(s1.begin(), s1.end(), ::isspace), s1.end());
    s2.erase(std::remove_if(s2.begin(), s2.end(), ::isspace), s2.end());
    
    return s1 == s2;
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   CANLI EL YAZISI OKUYUCU + BIRD'S EYE VIEW ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝\n" << std::endl;
    
    // Kamerayı aç
    cv::VideoCapture camera(0);
    
    if (!camera.isOpened()) {
        std::cerr << "❌ Kamera açılamadı!" << std::endl;
        return 1;
    }
    
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    
    std::cout << "✅ Kamera hazır!" << std::endl;
    std::cout << "\n📋 CEVAP ANAHTARI:" << std::endl;
    for (const auto& pair : answerKey) {
        std::cout << "   Soru " << pair.first << ": " << pair.second << std::endl;
    }
    
    std::cout << "\n📷 KULLANIM:" << std::endl;
    std::cout << "   P     = Perspektif düzeltme modu (4 köşeye tıkla)" << std::endl;
    std::cout << "   R     = Perspektif sıfırla (normal moda dön)" << std::endl;
    std::cout << "   SPACE = Görüntü yakala ve oku" << std::endl;
    std::cout << "   ESC   = Çıkış" << std::endl;
    std::cout << "\n──────────────────────────────────────────────\n" << std::endl;
    
    cv::Mat frame;
    bool processing = false;
    bool usePerspective = false;
    
    cv::namedWindow("El Yazisi Okuyucu");
    cv::setMouseCallback("El Yazisi Okuyucu", onMouse, nullptr);
    
    while (true) {
        camera >> frame;
        
        if (frame.empty()) {
            break;
        }
        
        cv::Mat display;
        cv::Mat workingFrame;
        
        // Perspektif düzeltme aktifse düzeltilmiş görüntüyü kullan
        if (usePerspective && !perspectiveCorrectedImage.empty()) {
            workingFrame = perspectiveCorrectedImage.clone();
        } else {
            workingFrame = frame.clone();
        }
        
        display = workingFrame.clone();
        
        if (!processing) {
            // Perspektif modu aktifse köşeleri göster
            if (perspectiveMode) {
                // Seçilen köşeleri çiz
                for (size_t i = 0; i < selectedCorners.size(); i++) {
                    cv::circle(display, selectedCorners[i], 8, cv::Scalar(0, 0, 255), -1);
                    cv::putText(display, std::to_string(i + 1),
                               cv::Point(selectedCorners[i].x + 10, selectedCorners[i].y - 10),
                               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
                }
                
                // Çizgileri çiz
                if (selectedCorners.size() > 1) {
                    for (size_t i = 0; i < selectedCorners.size() - 1; i++) {
                        cv::line(display, selectedCorners[i], selectedCorners[i + 1],
                                cv::Scalar(0, 255, 255), 2);
                    }
                    if (selectedCorners.size() == 4) {
                        cv::line(display, selectedCorners[3], selectedCorners[0],
                                cv::Scalar(0, 255, 255), 2);
                    }
                }
                
                // Bilgi
                std::string info = "PERSPEKTIF MODU: Koseler " + 
                                  std::to_string(selectedCorners.size()) + "/4";
                cv::putText(display, info, cv::Point(10, 30),
                           cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                cv::putText(display, "Siralama: Sol-Ust, Sag-Ust, Sag-Alt, Sol-Alt",
                           cv::Point(10, 60),
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
            }
            // Normal mod - yazı bölgelerini göster
            else {
                auto regions = findTextRegions(workingFrame);
                
                for (size_t i = 0; i < regions.size(); i++) {
                    cv::rectangle(display, regions[i], cv::Scalar(0, 255, 0), 2);
                    std::string label = "Bolge " + std::to_string(i + 1);
                    cv::putText(display, label,
                               cv::Point(regions[i].x, regions[i].y - 5),
                               cv::FONT_HERSHEY_SIMPLEX, 0.5,
                               cv::Scalar(0, 255, 0), 1);
                }
                
                // Bilgi
                std::string mode = usePerspective ? "[DUZELTILMIS]" : "[NORMAL]";
                cv::putText(display, mode + " P:Perspektif | R:Reset | SPACE:Oku | ESC:Cikis",
                           cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                           0.6, cv::Scalar(0, 255, 0), 2);
            }
        }
        
        cv::imshow("El Yazisi Okuyucu", display);
        
        int key = cv::waitKey(1);
        
        if (key == 27) { // ESC
            break;
        }
        else if (key == 'p' || key == 'P') { // Perspektif modu
            if (!perspectiveMode) {
                perspectiveMode = true;
                selectedCorners.clear();
                std::cout << "\n🔵 PERSPEKTİF MODU AKTİF!" << std::endl;
                std::cout << "📍 4 köşeye tıklayın (sıralama: sol-üst, sağ-üst, sağ-alt, sol-alt)" << std::endl;
            }
        }
        else if (key == 'r' || key == 'R') { // Reset
            perspectiveMode = false;
            usePerspective = false;
            selectedCorners.clear();
            perspectiveCorrectedImage.release();
            std::cout << "\n♻️  Perspektif sıfırlandı, normal moda dönüldü" << std::endl;
        }
        else if (key == 32 && !processing) { // SPACE
            // Eğer perspektif modundaysa ve 4 köşe seçildiyse düzelt
            if (perspectiveMode && selectedCorners.size() == 4) {
                perspectiveCorrectedImage = applyPerspectiveCorrection(frame, selectedCorners);
                usePerspective = true;
                perspectiveMode = false;
                std::cout << "✅ Perspektif düzeltmesi tamamlandı!" << std::endl;
                std::cout << "   SPACE ile okuma yapabilir, R ile sıfırlayabilirsiniz\n" << std::endl;
                continue;
            }
            
            processing = true;
            
            std::cout << "\n📸 Görüntü yakalandı!" << std::endl;
            std::cout << "🔍 Yazı bölgeleri aranıyor..." << std::endl;
            
            auto regions = findTextRegions(workingFrame);
            std::cout << "   Bulunan bölge: " << regions.size() << "\n" << std::endl;
            
            if (regions.empty()) {
                std::cout << "⚠️  Hiç yazı bulunamadı!\n" << std::endl;
                processing = false;
                continue;
            }
            
            // Her bölgeyi işle
            cv::Mat result = workingFrame.clone();
            int questionNum = 1;
            int correct = 0;
            int wrong = 0;
            
            for (const auto& region : regions) {
                // ROI kaydet
                cv::Mat roi = workingFrame(region);
                std::string filename = "temp_region.jpg";
                cv::imwrite(filename, roi);
                
                // Tesseract ile oku
                std::string cmd = "tesseract " + filename + " stdout -l tur --psm 3 2>/dev/null";
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
                
                // Label
                std::string label = "Q" + std::to_string(questionNum);
                cv::putText(result, label,
                           cv::Point(region.x, region.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
                
                // İkon
                if (!ocrText.empty()) {
                    std::string icon = isCorrect ? "✓" : "✗";
                    cv::putText(result, icon,
                               cv::Point(region.x + region.width + 10, region.y + 30),
                               cv::FONT_HERSHEY_SIMPLEX, 1.0, color, 2);
                    
                    if (isCorrect) correct++;
                    else wrong++;
                }
                
                // Konsol çıktısı
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
            std::cout << "Toplam Bölge: " << regions.size() << std::endl;
            std::cout << "✅ Doğru: " << correct << std::endl;
            std::cout << "❌ Yanlış: " << wrong << std::endl;
            
            if (correct + wrong > 0) {
                int percentage = (correct * 100) / (correct + wrong);
                std::cout << "📊 Başarı: %" << percentage << std::endl;
            }
            std::cout << "══════════════════════════════════════════════\n" << std::endl;
            
            // Sonuç görüntüsünü göster
            cv::imshow("SONUC", result);
            cv::imwrite("last_result.jpg", result);
            std::cout << "💾 Sonuç kaydedildi: last_result.jpg" << std::endl;
            std::cout << "\nYeni okuma için SPACE, çıkmak için ESC" << std::endl;
            
            processing = false;
        }
    }
    
    camera.release();
    cv::destroyAllWindows();
    
    return 0;
}
