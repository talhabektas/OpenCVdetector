/**
 * El Yazısı Satır Bulucu - MSER Tabanlı
 */

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Kullanım: " << argv[0] << " <image.jpg>" << std::endl;
        return 1;
    }
    
    cv::Mat image = cv::imread(argv[1]);
    if (image.empty()) {
        std::cerr << "Görüntü yüklenemedi!" << std::endl;
        return 1;
    }
    
    std::cout << "Görüntü boyutu: " << image.cols << "x" << image.rows << std::endl;
    
    // Grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    
    // BASİT YÖNTEM: Dikey projeksiyon profili
    std::cout << "\nYatay yazı satırları aranıyor (projeksiyon profili)...\n" << std::endl;
    
    // Binarize
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // Yatay projeksiyon (her satırdaki siyah pixel sayısı)
    std::vector<int> projection(binary.rows, 0);
    for (int y = 0; y < binary.rows; y++) {
        for (int x = 0; x < binary.cols; x++) {
            if (binary.at<uchar>(y, x) > 0) {
                projection[y]++;
            }
        }
    }
    
    // Yazı olan satırları bul (eşik üstü pixel sayısı)
    int threshold = binary.cols * 0.03;  // Satırın %3'ü dolu ise yazı var
    std::vector<std::pair<int, int>> textLines;  // (start_y, end_y)
    
    bool inText = false;
    int startY = 0;
    
    for (int y = 0; y < projection.size(); y++) {
        if (projection[y] > threshold && !inText) {
            // Yazı başladı
            startY = y;
            inText = true;
        } else if (projection[y] <= threshold && inText) {
            // Yazı bitti
            if (y - startY > 20) {  // Minimum yükseklik
                textLines.push_back({startY, y});
            }
            inText = false;
        }
    }
    
    // Son satır
    if (inText && projection.size() - startY > 20) {
        textLines.push_back({startY, (int)projection.size()});
    }
    
    std::cout << "Bulunan satır sayısı: " << textLines.size() << "\n" << std::endl;
    
    // Görselleştir
    cv::Mat visualized = image.clone();
    
    for (size_t i = 0; i < textLines.size(); i++) {
        int y1 = textLines[i].first;
        int y2 = textLines[i].second;
        int height = y2 - y1;
        
        // Padding ekle
        y1 = std::max(0, y1 - 10);
        y2 = std::min(image.rows, y2 + 10);
        height = y2 - y1;
        
        cv::Rect roi(0, y1, image.cols, height);
        
        // Dikdörtgen çiz
        cv::rectangle(visualized, roi, cv::Scalar(0, 255, 0), 3);
        
        // Label
        std::string label = "Satir " + std::to_string(i + 1);
        cv::putText(visualized, label, cv::Point(10, y1 + 20),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        
        std::cout << "Satır " << (i + 1) << ": y=" << y1 << " - " << y2 
                  << " (yükseklik=" << height << ")" << std::endl;
        
        // ROI kaydet
        cv::Mat lineROI = image(roi);
        std::string filename = "line_" + std::to_string(i + 1) + ".jpg";
        cv::imwrite(filename, lineROI);
        std::cout << "  Kaydedildi: " << filename << std::endl;
        
        // OCR uygula
        std::string tempFile = "temp_ocr_" + std::to_string(i) + ".jpg";
        cv::imwrite(tempFile, lineROI);
        
        std::string cmd = "tesseract " + tempFile + " stdout -l tur --psm 3 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[256];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                result += buffer;
            }
            pclose(pipe);
            
            // Trim
            result.erase(0, result.find_first_not_of(" \n\r\t"));
            result.erase(result.find_last_not_of(" \n\r\t") + 1);
            
            if (!result.empty()) {
                std::cout << "  OCR: \"" << result << "\"" << std::endl;
            }
        }
        
        std::remove(tempFile.c_str());
        std::cout << std::endl;
    }
    
    cv::imwrite("lines_detected.jpg", visualized);
    std::cout << "Görselleştirme kaydedildi: lines_detected.jpg" << std::endl;
    
    cv::namedWindow("Satırlar", cv::WINDOW_NORMAL);
    cv::imshow("Satırlar", visualized);
    cv::waitKey(0);
    
    return 0;
}