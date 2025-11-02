/**
 * Image Preview Tool - Görüntüyü grid ile göster
 */

#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Kullanım: " << argv[0] << " <image.jpg>" << std::endl;
        return 1;
    }
    
    std::string imagePath = argv[1];
    cv::Mat img = cv::imread(imagePath);
    
    if (img.empty()) {
        std::cerr << "Görüntü yüklenemedi!" << std::endl;
        return 1;
    }
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "GÖRÜNTÜ BİLGİLERİ" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Boyut: " << img.cols << " x " << img.rows << " piksel" << std::endl;
    std::cout << "Genişlik (X): 0 - " << img.cols << std::endl;
    std::cout << "Yükseklik (Y): 0 - " << img.rows << std::endl;
    std::cout << "==================================\n" << std::endl;
    
    // Küçük bir kopyası
    cv::Mat display;
    double scale = std::min(1200.0 / img.cols, 900.0 / img.rows);
    if (scale < 1.0) {
        cv::resize(img, display, cv::Size(), scale, scale);
    } else {
        display = img.clone();
    }
    
    // Grid çiz
    cv::Mat gridImg = display.clone();
    
    // Dikey çizgiler (her 200 piksel)
    for (int x = 0; x < display.cols; x += int(200 * scale)) {
        cv::line(gridImg, cv::Point(x, 0), cv::Point(x, display.rows), 
                 cv::Scalar(0, 255, 0), 1);
        // Koordinat yaz
        int realX = int(x / scale);
        cv::putText(gridImg, std::to_string(realX), cv::Point(x + 5, 20),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    }
    
    // Yatay çizgiler (her 200 piksel)
    for (int y = 0; y < display.rows; y += int(200 * scale)) {
        cv::line(gridImg, cv::Point(0, y), cv::Point(display.cols, y), 
                 cv::Scalar(0, 255, 0), 1);
        // Koordinat yaz
        int realY = int(y / scale);
        cv::putText(gridImg, std::to_string(realY), cv::Point(5, y + 15),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    }
    
    // Mouse callback için değişkenler
    struct MouseData {
        cv::Mat* img;
        double scale;
    } mouseData;
    
    mouseData.img = &img;
    mouseData.scale = scale;
    
    // Mouse callback
    auto mouseCallback = [](int event, int x, int y, int flags, void* userdata) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            MouseData* data = (MouseData*)userdata;
            int realX = int(x / data->scale);
            int realY = int(y / data->scale);
            std::cout << "\n📍 Tıklanan nokta: X=" << realX << ", Y=" << realY << std::endl;
            std::cout << "   Örnek crop: ./manual_crop <image> " 
                     << realX << " " << realY << " 1000 200" << std::endl;
        }
    };
    
    cv::namedWindow("Grid View - Tıklayın", cv::WINDOW_NORMAL);
    cv::setMouseCallback("Grid View - Tıklayın", mouseCallback, &mouseData);
    
    std::cout << "📋 NASIL KULLANILIR:" << std::endl;
    std::cout << "1. Yeşil grid çizgilerindeki sayılar = koordinatlar" << std::endl;
    std::cout << "2. El yazınızın üstüne TIKLAYIN" << std::endl;
    std::cout << "3. Koordinatları not edin" << std::endl;
    std::cout << "4. ESC ile çıkın\n" << std::endl;
    
    cv::imshow("Grid View - Tıklayın", gridImg);
    cv::waitKey(0);
    
    return 0;
}
