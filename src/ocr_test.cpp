/**
 * OCR Test - Basit el yazısı tanıma testi
 * Kullanım: ./ocr_test <image.jpg>
 */

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <allheaders.h>
#include <iostream>
#include <string>

// OpenCV Mat'ı Leptonica Pix'e dönüştür
Pix* matToPix(const cv::Mat& mat) {
    cv::Mat tempMat;
    
    // RGB formatına çevir (Leptonica RGB kullanır)
    if (mat.channels() == 1) {
        // Grayscale
        return pixRead("preprocessed_output.jpg");  // Dosyadan oku (geçici çözüm)
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, tempMat, cv::COLOR_BGR2RGB);
    } else {
        tempMat = mat.clone();
    }
    
    // Dosyaya kaydet ve oku (en güvenli yöntem)
    cv::imwrite("temp_for_pix.jpg", mat);
    Pix* pix = pixRead("temp_for_pix.jpg");
    
    return pix;
}

// OCR için ön işleme
cv::Mat preprocessForOCR(const cv::Mat& image) {
    cv::Mat processed;
    
    // Grayscale'e çevir
    if (image.channels() == 3) {
        cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = image.clone();
    }
    
    // Resize - büyüt (OCR için daha iyi)
    cv::resize(processed, processed, cv::Size(), 2.0, 2.0, cv::INTER_CUBIC);
    
    // YENİ: Daha yumuşak kontrast artırma
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(processed, processed);
    
    // Hafif blur
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    
    // YENİ: Adaptive threshold - Otsu yerine
    cv::adaptiveThreshold(processed, processed, 255, 
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                         cv::THRESH_BINARY, 21, 10);
    
    // Padding ekle
    cv::copyMakeBorder(processed, processed, 10, 10, 10, 10, 
                       cv::BORDER_CONSTANT, cv::Scalar(255));
    
    return processed;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Kullanım: " << argv[0] << " <image.jpg>" << std::endl;
        std::cout << "Örnek: " << argv[0] << " el_yazisi.jpg" << std::endl;
        return 1;
    }
    
    std::string imagePath = argv[1];
    
    // Görüntüyü yükle
    cv::Mat image = cv::imread(imagePath);
    
    if (image.empty()) {
        std::cerr << "Görüntü yüklenemedi: " << imagePath << std::endl;
        return 1;
    }
    
    std::cout << "Görüntü yüklendi: " << image.cols << "x" << image.rows << std::endl;
    
    // Ön işleme
    std::cout << "Ön işleme yapılıyor..." << std::endl;
    
    // YENİ: Preprocessing YAPMA - Orijinal görüntüyü kullan
    cv::Mat preprocessed = image.clone();
    
    // Ön işlenmiş görüntüyü kaydet
    cv::imwrite("preprocessed_output.jpg", preprocessed);
    std::cout << "Ön işlenmiş görüntü kaydedildi: preprocessed_output.jpg (ORIJINAL)" << std::endl;
    
    // Tesseract başlat
    std::cout << "Tesseract başlatılıyor..." << std::endl;
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    
    // Türkçe dil desteği ile başlat
    if (api->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY)) {
        std::cerr << "Tesseract başlatılamadı!" << std::endl;
        std::cerr << "Türkçe dil paketi kurulu olduğundan emin olun:" << std::endl;
        std::cerr << "brew install tesseract-lang" << std::endl;
        return 1;
    }
    
    std::cout << "Tesseract başlatıldı (Türkçe)" << std::endl;
    
    // PSM ayarla - tek satır için
    api->SetPageSegMode(tesseract::PSM_AUTO);
    
    // Pix'e dönüştür
    std::cout << "Görüntü dönüştürülüyor..." << std::endl;
    Pix* pix = matToPix(preprocessed);
    
    if (!pix) {
        std::cerr << "Pix dönüşümü başarısız!" << std::endl;
        return 1;
    }
    
    // OCR uygula
    std::cout << "OCR uygulanıyor..." << std::endl;
    api->SetImage(pix);
    
    char* rawText = api->GetUTF8Text();
    int confidence = api->MeanTextConf();
    
    // Sonuçları göster
    std::cout << "\n========================================" << std::endl;
    std::cout << "OCR SONUÇLARI:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Güven: " << confidence << "%" << std::endl;
    std::cout << "Metin: \"" << (rawText ? rawText : "") << "\"" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Görüntüyü göster
    cv::namedWindow("Original", cv::WINDOW_NORMAL);
    cv::namedWindow("Preprocessed", cv::WINDOW_NORMAL);
    cv::imshow("Original", image);
    cv::imshow("Preprocessed", preprocessed);
    
    std::cout << "Pencereyi kapatmak için bir tuşa basın..." << std::endl;
    cv::waitKey(0);
    
    // Temizlik
    delete[] rawText;
    pixDestroy(&pix);
    api->End();
    delete api;
    
    return 0;
}