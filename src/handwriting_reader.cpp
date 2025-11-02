/**
 * Tam Otomatik El Yazısı Okuyucu
 * Kağıtta nerede yazı varsa buluyor ve okuyor
 */

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <allheaders.h>
#include <iostream>
#include <fstream>
#include <vector>

struct HandwritingBlock {
    cv::Rect region;
    std::string text;
    int confidence;
    int blockNumber;
};

std::vector<cv::Rect> findHandwritingBlocks(const cv::Mat& image) {
    std::vector<cv::Rect> blocks;
    
    // Grayscale
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // Binarize - el yazısını öne çıkar
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // Gürültüyü temizle
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    
    // Harfleri birleştir - kelimeleri/satırları grupla
    kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 5));
    cv::Mat dilated;
    cv::dilate(binary, dilated, kernel, cv::Point(-1, -1), 3);
    
    // Konturları bul
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(dilated, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Filtreleme
    for (const auto& contour : contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        
        // Minimum boyut kontrolü - DAHA DÜŞÜK eşikler
        if (bbox.width > 50 && bbox.height > 20 && bbox.height < 500) {
            // Çok geniş bölgeleri atla (muhtemelen gürültü)
            if (bbox.width < image.cols * 0.95) {
                // Padding ekle
                int padding = 15;
                bbox.x = std::max(0, bbox.x - padding);
                bbox.y = std::max(0, bbox.y - padding);
                bbox.width = std::min(image.cols - bbox.x, bbox.width + 2 * padding);
                bbox.height = std::min(image.rows - bbox.y, bbox.height + 2 * padding);
                
                blocks.push_back(bbox);
            }
        }
    }
    
    // Y koordinatına göre sırala (yukarıdan aşağıya)
    std::sort(blocks.begin(), blocks.end(), 
              [](const cv::Rect& a, const cv::Rect& b) {
                  return a.y < b.y;
              });
    
    return blocks;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Kullanım: " << argv[0] << " <image.jpg>" << std::endl;
        std::cout << "Örnek: " << argv[0] << " notebook.jpg" << std::endl;
        return 1;
    }
    
    std::string imagePath = argv[1];
    
    // Görüntüyü yükle
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Görüntü yüklenemedi: " << imagePath << std::endl;
        return 1;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "OTOMATIK EL YAZISI OKUYUCU" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Görüntü: " << imagePath << std::endl;
    std::cout << "Boyut: " << image.cols << "x" << image.rows << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // El yazısı bloklarını bul
    std::cout << "1. El yazısı blokları aranıyor..." << std::endl;
    auto blocks = findHandwritingBlocks(image);
    std::cout << "   Bulunan blok sayısı: " << blocks.size() << "\n" << std::endl;
    
    if (blocks.empty()) {
        std::cout << "⚠️  Hiç el yazısı bloğu bulunamadı!" << std::endl;
        return 1;
    }
    
    // Tesseract başlat
    std::cout << "2. OCR motoru başlatılıyor..." << std::endl;
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    
    if (api->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY)) {
        std::cerr << "   ❌ Tesseract başlatılamadı!" << std::endl;
        return 1;
    }
    
    api->SetPageSegMode(tesseract::PSM_AUTO);
    std::cout << "   ✅ OCR hazır (Türkçe)\n" << std::endl;
    
    // Her bloğu oku
    std::cout << "3. El yazıları okunuyor..." << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::vector<HandwritingBlock> results;
    cv::Mat visualized = image.clone();
    
    for (size_t i = 0; i < blocks.size(); i++) {
        cv::Rect roi = blocks[i];
        cv::Mat blockImg = image(roi);
        
        // OCR için temp dosya
        std::string tempFile = "temp_block_" + std::to_string(i) + ".jpg";
        cv::imwrite(tempFile, blockImg);
        
        Pix* pix = pixRead(tempFile.c_str());
        if (!pix) {
            std::cout << "Blok " << (i + 1) << ": ❌ Görüntü okunamadı" << std::endl;
            std::remove(tempFile.c_str());
            continue;
        }
        
        api->SetImage(pix);
        char* rawText = api->GetUTF8Text();
        int confidence = api->MeanTextConf();
        
        std::string text = rawText ? rawText : "";
        delete[] rawText;
        pixDestroy(&pix);
        std::remove(tempFile.c_str());
        
        // Temizle
        text.erase(0, text.find_first_not_of(" \n\r\t"));
        text.erase(text.find_last_not_of(" \n\r\t") + 1);
        
        // Sonuç
        HandwritingBlock result;
        result.region = roi;
        result.text = text;
        result.confidence = confidence;
        result.blockNumber = i + 1;
        results.push_back(result);
        
        // Görselleştir
        cv::Scalar color = text.empty() ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
        cv::rectangle(visualized, roi, color, 3);
        
        std::string label = "Blok " + std::to_string(i + 1);
        cv::putText(visualized, label, cv::Point(roi.x, roi.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        
        // Konsola yazdır
        std::cout << "┌─────────────────────────────────────" << std::endl;
        std::cout << "│ Blok " << (i + 1) << " (x=" << roi.x << ", y=" << roi.y 
                  << ", " << roi.width << "x" << roi.height << ")" << std::endl;
        std::cout << "│ Güven: " << confidence << "%" << std::endl;
        
        if (text.empty()) {
            std::cout << "│ Metin: (boş)" << std::endl;
        } else {
            std::cout << "│ Metin: \"" << text << "\"" << std::endl;
        }
        std::cout << "└─────────────────────────────────────\n" << std::endl;
    }
    
    // Özet
    std::cout << "\n========================================" << std::endl;
    std::cout << "ÖZET" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Toplam blok: " << results.size() << std::endl;
    
    int successful = 0;
    for (const auto& r : results) {
        if (!r.text.empty()) successful++;
    }
    
    std::cout << "Okunan: " << successful << std::endl;
    std::cout << "Boş: " << (results.size() - successful) << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Görselleştirmeyi kaydet
    cv::imwrite("handwriting_detected.jpg", visualized);
    std::cout << "📸 Görselleştirme kaydedildi: handwriting_detected.jpg" << std::endl;
    
    // Sonuçları dosyaya yaz
    std::ofstream outFile("handwriting_results.txt");
    for (const auto& r : results) {
        outFile << "Blok " << r.blockNumber << ": ";
        if (r.text.empty()) {
            outFile << "(boş)";
        } else {
            outFile << r.text;
        }
        outFile << " (güven: " << r.confidence << "%)" << std::endl;
    }
    outFile.close();
    std::cout << "💾 Sonuçlar kaydedildi: handwriting_results.txt" << std::endl;
    
    // Göster
    cv::namedWindow("El Yazısı Algılama", cv::WINDOW_NORMAL);
    cv::imshow("El Yazısı Algılama", visualized);
    
    std::cout << "\nPencereyi kapatmak için bir tuşa basın..." << std::endl;
    cv::waitKey(0);
    
    // Temizlik
    api->End();
    delete api;
    
    return 0;
}
