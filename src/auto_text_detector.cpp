/**
 * Otomatik Yazı Bölgesi Bulucu
 * Normal kağıtta yazılı metinleri otomatik bulur
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

struct TextRegion {
    cv::Rect boundingBox;
    int lineNumber;
};

std::vector<TextRegion> findTextRegions(const cv::Mat& image) {
    std::vector<TextRegion> regions;
    
    // 1. Grayscale'e çevir
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // 2. Binarize - yazıları öne çıkar
    cv::Mat binary;
    cv::adaptiveThreshold(gray, binary, 255, 
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                         cv::THRESH_BINARY_INV, 31, 15);  // Daha büyük block size
    
    // 3. Morfolojik işlemler - satırları birleştir
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(30, 5));  // Daha küçük kernel
    cv::Mat morphed;
    cv::morphologyEx(binary, morphed, cv::MORPH_CLOSE, kernel);
    
    // 4. Konturları bul
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(morphed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // 5. Filtreleme - sadece yatay satırları al
    int lineNum = 0;
    for (const auto& contour : contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        
        // Minimum boyut kontrolü - EL YAZISI için daha küçük eşikler
        if (bbox.width > 80 && bbox.height > 20 && bbox.height < 200) {
            // Aspect ratio kontrolü (genişlik > yükseklik)
            float aspectRatio = (float)bbox.width / bbox.height;
            if (aspectRatio > 2.0) {  // Daha düşük aspect ratio
                TextRegion region;
                region.boundingBox = bbox;
                region.lineNumber = lineNum++;
                regions.push_back(region);
            }
        }
    }
    
    // 6. Y koordinatına göre sırala (yukarıdan aşağıya)
    std::sort(regions.begin(), regions.end(), 
              [](const TextRegion& a, const TextRegion& b) {
                  return a.boundingBox.y < b.boundingBox.y;
              });
    
    // Satır numaralarını güncelle
    for (size_t i = 0; i < regions.size(); i++) {
        regions[i].lineNumber = i + 1;
    }
    
    return regions;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Kullanım: " << argv[0] << " <image.jpg>" << std::endl;
        return 1;
    }
    
    // Görüntüyü yükle
    cv::Mat image = cv::imread(argv[1]);
    if (image.empty()) {
        std::cerr << "Görüntü yüklenemedi!" << std::endl;
        return 1;
    }
    
    std::cout << "Görüntü boyutu: " << image.cols << "x" << image.rows << std::endl;
    std::cout << "Yazı bölgeleri aranıyor...\n" << std::endl;
    
    // Yazı bölgelerini bul
    auto regions = findTextRegions(image);
    
    std::cout << "Bulunan bölge sayısı: " << regions.size() << "\n" << std::endl;
    
    // Görselleştirme
    cv::Mat visualized = image.clone();
    
    for (const auto& region : regions) {
        // Dikdörtgen çiz
        cv::rectangle(visualized, region.boundingBox, cv::Scalar(0, 255, 0), 2);
        
        // Satır numarası yaz
        std::string label = "Satir " + std::to_string(region.lineNumber);
        cv::putText(visualized, label, 
                   cv::Point(region.boundingBox.x, region.boundingBox.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
        
        // Bilgileri yazdır
        std::cout << "Satır " << region.lineNumber << ": "
                  << "x=" << region.boundingBox.x << ", "
                  << "y=" << region.boundingBox.y << ", "
                  << "w=" << region.boundingBox.width << ", "
                  << "h=" << region.boundingBox.height << std::endl;
        
        // ROI'yi kaydet
        cv::Mat roi = image(region.boundingBox);
        std::string filename = "text_line_" + std::to_string(region.lineNumber) + ".jpg";
        cv::imwrite(filename, roi);
        std::cout << "  Kaydedildi: " << filename << std::endl;
    }
    
    // Sonucu göster
    cv::namedWindow("Bulunan Yazı Bölgeleri", cv::WINDOW_NORMAL);
    cv::imshow("Bulunan Yazı Bölgeleri", visualized);
    
    std::cout << "\nGörselleştirme kaydediliyor..." << std::endl;
    cv::imwrite("text_regions_detected.jpg", visualized);
    std::cout << "Kaydedildi: text_regions_detected.jpg" << std::endl;
    
    std::cout << "\nPencereyi kapatmak için bir tuşa basın..." << std::endl;
    cv::waitKey(0);
    
    return 0;
}