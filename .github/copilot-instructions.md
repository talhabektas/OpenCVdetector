# OMR System - GitHub Copilot Instructions

## Proje Tanımı
Optik İşaret Tanıma (OMR) sistemi - Çoktan seçmeli sınavları, boşluk doldurma sorularını ve Doğru/Yanlış sorularını otomatik değerlendiren sistem.

## Teknik Gereksinimler
- **Dil**: C++ (C++17 veya üstü)
- **Ana Kütüphane**: OpenCV 4.x
- **OCR Motoru**: Tesseract OCR (El yazısı tanıma için)
- **Paradigma**: Nesne Yönelimli Programlama (OOP)
- **Temiz Kod**: SOLID prensipleri takip et

## 🚨 KRİTİK ÖNCELİKLER

### 1. BOŞLUK DOLDURMA TESPİTİ (EN ÖNEMLİ)
- Kağıt üzerindeki boş alanları tespit et
- El yazısı olup olmadığını kontrol et
- El yazısını Tesseract OCR ile oku
- OCR doğruluğunu artırmak için ön işleme yap

### 2. BUBBLE (BALON) TESPİTİ
- Çoktan seçmeli sorulardaki dolu balonları tespit et
- HoughCircles algoritması kullan
- Dolu yüzde hesabı yap (>%60 = işaretli)

### 3. KAMERA İŞLEME
- Telefon/bilgisayar kamerasından canlı görüntü al
- Bird's-eye view (kuş bakışı) düzeltmesi uygula
- Perspective transform ile kağıdı düzelt

## Kod Yazma Kuralları

### Genel Prensipler
```cpp
// ✅ DOĞRU: Anlamlı isimler kullan
double calculateBubbleFillPercentage(const cv::Mat& bubbleROI);

// ❌ YANLIŞ: Kısa/belirsiz isimler kullanma
double calc(cv::Mat img);

// ✅ DOĞRU: Her class için .h ve .cpp dosyası oluştur
// BubbleDetector.h ve BubbleDetector.cpp

// ✅ DOĞRU: Hata kontrolü yap
try {
    cv::Mat frame = camera.captureFrame();
} catch (const cv::Exception& e) {
    std::cerr << "Kamera hatası: " << e.what() << std::endl;
}

// ✅ DOĞRU: const kullan
void processImage(const cv::Mat& image);

// ✅ DOĞRU: Smart pointer kullan
std::unique_ptr<OCRProcessor> ocrProcessor = 
    std::make_unique<OCRProcessor>();
```

### Class Yapısı
```cpp
// Her class tek bir sorumluluğa sahip olmalı (Single Responsibility)
class HandwritingDetector {
private:
    double minimumPixelDensity;
    cv::Mat preprocessingKernel;
    
public:
    // Constructor
    explicit HandwritingDetector(double minDensity = 0.05);
    
    // Public methods - dokümantasyon ekle
    /**
     * Verilen bölgede el yazısı olup olmadığını tespit eder
     * @param image Giriş görüntüsü
     * @param region Kontrol edilecek bölge
     * @return true eğer el yazısı tespit edildiyse
     */
    bool detectHandwriting(const cv::Mat& image, const cv::Rect& region);
    
private:
    // Private helper methods
    cv::Mat preprocessRegion(const cv::Mat& roi);
    double calculatePixelDensity(const cv::Mat& roi);
};
```

## Proje Yapısı
```
OMR-System/
├── .github/
│   └── copilot-instructions.md
├── src/
│   ├── camera/
│   │   ├── CameraManager.h
│   │   └── CameraManager.cpp
│   ├── preprocessing/
│   │   ├── PerspectiveCorrector.h
│   │   ├── PerspectiveCorrector.cpp
│   │   ├── ImageEnhancer.h
│   │   └── ImageEnhancer.cpp
│   ├── detection/
│   │   ├── BubbleDetector.h          # Balon tespiti
│   │   ├── BubbleDetector.cpp
│   │   ├── HandwritingDetector.h     # 🚨 KRİTİK
│   │   ├── HandwritingDetector.cpp
│   │   ├── SheetStructureAnalyzer.h
│   │   └── SheetStructureAnalyzer.cpp
│   ├── ocr/
│   │   ├── OCRProcessor.h            # 🚨 KRİTİK - El yazısı okuma
│   │   └── OCRProcessor.cpp
│   ├── grading/
│   │   ├── AnswerKey.h
│   │   ├── AnswerComparator.h
│   │   └── ScoreCalculator.h
│   ├── output/
│   │   ├── ResultDisplayer.h
│   │   ├── FileWriter.h
│   │   └── EmailSender.h
│   └── main.cpp
├── include/
├── tests/
├── CMakeLists.txt
└── README.md
```

## Ana Modüller

### 1. CameraManager (Kamera Yönetimi)
```cpp
class CameraManager {
public:
    bool initCamera(int deviceId = 0);
    cv::Mat captureFrame();
    void releaseCamera();
private:
    cv::VideoCapture camera;
};
```

### 2. PerspectiveCorrector (Perspektif Düzeltme) ⚡ YÜK SEK PRİORİTE
```cpp
class PerspectiveCorrector {
public:
    // Kağıdın köşelerini tespit et
    std::vector<cv::Point2f> detectPaperCorners(const cv::Mat& image);
    
    // Kuş bakışı görünüm elde et
    cv::Mat applyPerspectiveTransform(
        const cv::Mat& image, 
        const std::vector<cv::Point2f>& corners
    );
};
```

### 3. BubbleDetector (Balon Tespiti) 🚨 KRİTİK
```cpp
class BubbleDetector {
private:
    double fillThreshold = 0.6;  // %60 dolu = işaretli
    
public:
    std::vector<int> detectMarkedBubbles(
        const cv::Mat& image,
        const std::vector<cv::Rect>& bubbleRegions
    );
    
private:
    double calculateFillPercentage(const cv::Mat& bubbleROI);
    bool isMarked(double fillPercentage);
};
```

### 4. HandwritingDetector (El Yazısı Tespiti) 🚨 EN KRİTİK
```cpp
class HandwritingDetector {
public:
    // Bölgede el yazısı var mı kontrol et
    bool hasHandwriting(const cv::Mat& image, const cv::Rect& region);
    
    // El yazısı ROI'sini çıkar
    cv::Mat extractHandwritingROI(const cv::Mat& image, const cv::Rect& region);
    
private:
    cv::Mat preprocessForDetection(const cv::Mat& roi);
    double analyzePixelDensity(const cv::Mat& roi);
};
```

### 5. OCRProcessor (OCR İşlemci) 🚨 EN KRİTİK
```cpp
class OCRProcessor {
private:
    tesseract::TessBaseAPI* tesseractAPI;
    
public:
    OCRProcessor();
    ~OCRProcessor();
    
    // El yazısını metne çevir
    std::string recognizeText(const cv::Mat& handwritingROI);
    
private:
    cv::Mat preprocessForOCR(const cv::Mat& image);
    std::string postProcessText(const std::string& rawText);
};
```

## Algoritma Detayları

### Boşluk Doldurma Algoritması (EN ÖNEMLİ)
```cpp
/**
 * 1. Boşluk bölgelerini tespit et (SheetStructureAnalyzer ile)
 * 2. Her bölge için:
 *    a. ROI'yi çıkar
 *    b. Pixel yoğunluğu analizi yap
 *    c. Eğer yazı varsa:
 *       - Binarizasyon uygula
 *       - Gürültüyü temizle
 *       - Kontrast artır
 *       - Tesseract OCR uygula (PSM mode 7: tek satır)
 *       - Metni doğrula ve temizle
 */
std::string processFillInBlank(const cv::Mat& image, const cv::Rect& region) {
    // Önce el yazısı var mı kontrol et
    if (!handwritingDetector.hasHandwriting(image, region)) {
        return "";  // Boş bırakılmış
    }
    
    // ROI çıkar
    cv::Mat roi = image(region);
    
    // OCR için ön işle
    cv::Mat preprocessed = ocrProcessor.preprocessForOCR(roi);
    
    // OCR uygula
    std::string text = ocrProcessor.recognizeText(preprocessed);
    
    return text;
}
```

### Balon Tespit Algoritması
```cpp
/**
 * 1. Gaussian blur uygula (5x5 kernel)
 * 2. Grayscale'e çevir
 * 3. HoughCircles ile daireleri tespit et
 * 4. Her daire için:
 *    a. ROI çıkar
 *    b. Non-zero pixel say
 *    c. Dolu yüzde = (non-zero / total) * 100
 *    d. Eğer >%60 ise işaretli
 */
```

### Perspektif Düzeltme
```cpp
/**
 * 1. Canny edge detection (threshold1=50, threshold2=150)
 * 2. Konturları bul, en büyük dikdörtgeni seç
 * 3. 4 köşe noktasını sırala (sol-üst, sağ-üst, sağ-alt, sol-alt)
 * 4. getPerspectiveTransform() ile matrix hesapla
 * 5. warpPerspective() uygula
 */
```

## OpenCV İpuçları
```cpp
// Görüntü ön işleme
cv::GaussianBlur(image, blurred, cv::Size(5, 5), 0);
cv::cvtColor(blurred, gray, cv::COLOR_BGR2GRAY);
cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

// Morfolojik işlemler
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
cv::morphologyEx(binary, cleaned, cv::MORPH_OPEN, kernel);
cv::morphologyEx(cleaned, result, cv::MORPH_CLOSE, kernel);

// Kontur bulma
std::vector<std::vector<cv::Point>> contours;
cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

// Daire tespit
std::vector<cv::Vec3f> circles;
cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, 
                 gray.rows/16, 50, 30, minRadius, maxRadius);
```

## Performans Hedefleri
- Real-time işleme: 30+ FPS
- Balon tespit doğruluğu: >%95
- El yazısı tanıma doğruluğu: >%85
- Perspektif düzeltme: <50ms/frame

## Yapılmaması Gerekenler ❌
- Global değişken kullanma
- Magic number kullanma (const/define kullan)
- Hata kontrolü yapma
- Kamera kaynağını release etmemeyi unutma
- Input validasyon atlamak
- Aşırı nested if-else (early return kullan)

## Yapılması Gerekenler ✅
- RAII kullan (Resource Acquisition Is Initialization)
- const correctness uygula
- Smart pointer kullan (unique_ptr, shared_ptr)
- Exception handling yap
- Log mesajları ekle
- Her modül için unit test yaz

## Test Gereksinimleri
- En az 50 farklı el yazısı örneği ile test et
- Farklı ışık koşullarında test et
- Eğik/döndürülmüş kağıtlarla test et
- FPS'i ölç ve optimize et
- OCR doğruluğunu doğrula (hedef: >%85)

## CMake Örneği
```cmake
cmake_minimum_required(VERSION 3.15)
project(OMR_System)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV 4.0 REQUIRED)
find_package(Tesseract REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${Tesseract_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(OMR_System
    src/main.cpp
    src/camera/CameraManager.cpp
    src/detection/BubbleDetector.cpp
    src/detection/HandwritingDetector.cpp
    src/ocr/OCRProcessor.cpp
    # ... diğer dosyalar
)

target_link_libraries(OMR_System
    ${OpenCV_LIBS}
    tesseract
    leptonica
)
```

## Önemli Notlar

1. **El Yazısı Tanıma için**: Tesseract'ı Türkçe dil desteği ile başlat:
```cpp
tesseractAPI->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY);
```

2. **OCR Doğruluğu için**: PSM (Page Segmentation Mode) ayarla:
```cpp
tesseractAPI->SetPageSegMode(tesseract::PSM_SINGLE_LINE);  // Tek satır için
```

3. **Gerçek Zamanlı İşleme için**: Multi-threading kullan:
```cpp
std::thread processingThread(&processFrame, frame);
```

Bu talimatları takip ederek temiz, performanslı ve bakımı kolay kod üret!