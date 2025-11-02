/**
 * Silent Crop - Pencere açmadan kırp
 */
#include <opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Kullanım: " << argv[0] << " <image> <x> <y> <w> <h> [output]" << std::endl;
        return 1;
    }
    
    std::string input = argv[1];
    int x = std::stoi(argv[2]);
    int y = std::stoi(argv[3]);
    int w = std::stoi(argv[4]);
    int h = std::stoi(argv[5]);
    std::string output = argc > 6 ? argv[6] : "cropped.jpg";
    
    cv::Mat img = cv::imread(input);
    if (img.empty()) {
        std::cerr << "Görüntü yüklenemedi!" << std::endl;
        return 1;
    }
    
    if (x < 0 || y < 0 || x + w > img.cols || y + h > img.rows) {
        std::cerr << "Koordinatlar sınırların dışında!" << std::endl;
        return 1;
    }
    
    cv::Rect roi(x, y, w, h);
    cv::Mat cropped = img(roi);
    cv::imwrite(output, cropped);
    
    return 0;
}