#include "OCRProcessor.h"
#include <iostream>
#include <algorithm>
#include <cctype>

OCRProcessor::OCRProcessor(const char* language, const char* dataPath)
    : tesseractAPI(nullptr), initialized(false), lastConfidence(0.0f) {
    
    try {
        // Create Tesseract API instance
        tesseractAPI = new tesseract::TessBaseAPI();
        
        // Initialize with language
        if (tesseractAPI->Init(dataPath, language, tesseract::OEM_LSTM_ONLY) != 0) {
            std::cerr << "Tesseract başlatılamadı! Dil: " << language << std::endl;
            delete tesseractAPI;
            tesseractAPI = nullptr;
            return;
        }
        
        // Set page segmentation mode for single line text
        tesseractAPI->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
        
        // Configure for better handwriting recognition
        tesseractAPI->SetVariable("tessedit_char_whitelist", 
            "ABCÇDEFGĞHIİJKLMNOÖPRSŞTUÜVYZabcçdefgğhıijklmnoöprsştuüvyz0123456789 .,;:!?-");
        
        initialized = true;
        std::cout << "OCR başarıyla başlatıldı (Dil: " << language << ")" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "OCR başlatma hatası: " << e.what() << std::endl;
        if (tesseractAPI) {
            delete tesseractAPI;
            tesseractAPI = nullptr;
        }
    }
}

OCRProcessor::~OCRProcessor() {
    if (tesseractAPI) {
        tesseractAPI->End();
        delete tesseractAPI;
        tesseractAPI = nullptr;
    }
}

// OpenCV Mat'ı Leptonica Pix'e dönüştür
Pix* OCRProcessor::matToPix(const cv::Mat& mat) {
    // En güvenilir yöntem: Dosyaya kaydet, Leptonica ile oku
    static int tempCounter = 0;
    std::string tempFile = "temp_pix_" + std::to_string(tempCounter++) + ".jpg";
    
    cv::imwrite(tempFile, mat);
    Pix* pix = pixRead(tempFile.c_str());
    
    // Temp dosyayı sil
    std::remove(tempFile.c_str());
    
    return pix;
}

cv::Mat OCRProcessor::preprocessForOCR(const cv::Mat& image) {
    cv::Mat processed;
    
    // Convert to grayscale
    if (image.channels() == 3) {
        cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = image.clone();
    }
    
    // Resize if too small (OCR works better with larger text)
    if (processed.rows < 40) {
        double scale = 40.0 / processed.rows;
        cv::resize(processed, processed, cv::Size(), scale, scale, cv::INTER_CUBIC);
    }
    
    // Enhance contrast using CLAHE
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    clahe->apply(processed, processed);
    
    // Apply light Gaussian blur to reduce noise
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    
    // Binarization using Otsu's method
    cv::Mat binary;
    cv::threshold(processed, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    // Check if we need to invert (OCR expects black text on white background)
    int whitePixels = cv::countNonZero(binary);
    int totalPixels = binary.rows * binary.cols;
    if (whitePixels < totalPixels / 2) {
        cv::bitwise_not(binary, binary);
    }
    
    // Remove small noise
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    
    // Slight dilation to connect broken characters
    cv::dilate(binary, binary, kernel, cv::Point(-1, -1), 1);
    
    // Add border (Tesseract works better with margins)
    int borderSize = 10;
    cv::copyMakeBorder(binary, binary, borderSize, borderSize, borderSize, borderSize,
                      cv::BORDER_CONSTANT, cv::Scalar(255));
    
    return binary;
}

std::string OCRProcessor::trimText(const std::string& text) {
    // Find first non-whitespace character
    auto start = std::find_if_not(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    
    // Find last non-whitespace character
    auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    
    // Return trimmed string
    return (start < end) ? std::string(start, end) : std::string();
}

std::string OCRProcessor::postProcessText(const std::string& rawText) {
    if (rawText.empty()) {
        return "";
    }
    
    // Trim whitespace
    std::string cleaned = trimText(rawText);
    
    // Remove multiple spaces
    std::string result;
    bool lastWasSpace = false;
    
    for (char c : cleaned) {
        if (std::isspace(c)) {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }
    
    // Final trim
    result = trimText(result);
    
    return result;
}

std::string OCRProcessor::recognizeText(const cv::Mat& handwritingROI) {
    if (!initialized || !tesseractAPI) {
        std::cerr << "OCR başlatılmamış!" << std::endl;
        return "";
    }
    
    if (handwritingROI.empty()) {
        std::cerr << "Boş görüntü!" << std::endl;
        return "";
    }
    
    std::cout << "[DEBUG OCR] Giriş görüntüsü: " << handwritingROI.cols << "x" 
              << handwritingROI.rows << " channels=" << handwritingROI.channels() << std::endl;
    
    try {
        // YENİ: Minimal preprocessing - orijinal görüntüyü kullan
        // Ağır preprocessing el yazısını bozuyor!
        cv::Mat preprocessed = handwritingROI.clone();
        
        std::cout << "[DEBUG OCR] Ön işleme sonrası: " << preprocessed.cols << "x" 
                  << preprocessed.rows << std::endl;
        
        // Save preprocessed image for debugging
        static int debugCounter = 0;
        std::string debugFilename = "debug_ocr_preprocessed_" + std::to_string(debugCounter++) + ".jpg";
        cv::imwrite(debugFilename, preprocessed);
        std::cout << "[DEBUG OCR] Ön işlenmiş görüntü kaydedildi: " << debugFilename << std::endl;
        
        // Convert to Pix format
        Pix* pix = matToPix(preprocessed);
        
        if (!pix) {
            std::cerr << "Pix dönüşümü başarısız!" << std::endl;
            return "";
        }
        
        std::cout << "[DEBUG OCR] Pix dönüşümü başarılı" << std::endl;
        
        // Set image for OCR
        tesseractAPI->SetImage(pix);
        
        // Perform OCR
        char* rawText = tesseractAPI->GetUTF8Text();
        
        // Get confidence
        lastConfidence = tesseractAPI->MeanTextConf();
        
        std::cout << "[DEBUG OCR] Ham metin: \"" << (rawText ? rawText : "NULL") 
                  << "\" Güven: " << lastConfidence << "%" << std::endl;
        
        // Convert to string
        std::string text = rawText ? rawText : "";
        
        // Clean up
        delete[] rawText;
        pixDestroy(&pix);
        
        // Post-process text
        text = postProcessText(text);
        
        if (!text.empty()) {
            std::cout << "OCR Sonuç: \"" << text << "\" (Güven: " 
                     << lastConfidence << "%)" << std::endl;
        } else {
            std::cout << "[DEBUG OCR] Post-processing sonrası metin boş!" << std::endl;
        }
        
        return text;
        
    } catch (const std::exception& e) {
        std::cerr << "OCR hatası: " << e.what() << std::endl;
        return "";
    }
}

std::string OCRProcessor::recognizeTextWithConfidence(
    const cv::Mat& handwritingROI,
    float minConfidence) {
    
    std::string text = recognizeText(handwritingROI);
    
    if (lastConfidence < minConfidence) {
        std::cout << "Düşük güven skoru (" << lastConfidence 
                 << "%), metin reddedildi." << std::endl;
        return "";
    }
    
    return text;
}

void OCRProcessor::setPageSegmentationMode(tesseract::PageSegMode mode) {
    if (initialized && tesseractAPI) {
        tesseractAPI->SetPageSegMode(mode);
    }
}

float OCRProcessor::getConfidence() const {
    return lastConfidence;
}

bool OCRProcessor::isInitialized() const {
    return initialized && tesseractAPI != nullptr;
}
