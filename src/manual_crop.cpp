/**
 * Manuel Crop Tool - Belirtilen koordinatları kırp
 * Kullanım: ./manual_crop <image.jpg> <x> <y> <width> <height>
 */

#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cout << "Kullanım: " << argv[0] << " <image.jpg> <x> <y> <width> <height>" << std::endl;
        std::cout << "Örnek: " << argv[0] << " IMG_8616.jpg 500 1000 1000 200" << std::endl;
        return 1;
    }
    
    std::string imagePath = argv[1];
    int x = std::stoi(argv[2]);
    int y = std::stoi(argv[3]);
    int w = std::stoi(argv[4]);
    int h = std::stoi(argv[5]);
    
    // Load image
    cv::Mat img = cv::imread(imagePath);
    
    if (img.empty()) {
        std::cerr << "Görüntü yüklenemedi: " << imagePath << std::endl;
        return 1;
    }
    
    std::cout << "Orijinal boyut: " << img.cols << "x" << img.rows << std::endl;
    
    // Validate coordinates
    if (x < 0 || y < 0 || x + w > img.cols || y + h > img.rows) {
        std::cerr << "Hata: Koordinatlar görüntü sınırlarının dışında!" << std::endl;
        std::cerr << "Max: " << img.cols << "x" << img.rows << std::endl;
        return 1;
    }
    
    // Crop
    cv::Rect roi(x, y, w, h);
    cv::Mat cropped = img(roi);
    
    // Save
    cv::imwrite("cropped_text.jpg", cropped);
    std::cout << "Kaydedildi: cropped_text.jpg (" << w << "x" << h << ")" << std::endl;
    
    // Show
    cv::namedWindow("Cropped", cv::WINDOW_NORMAL);
    cv::imshow("Cropped", cropped);
    cv::waitKey(0);
    
    return 0;
}