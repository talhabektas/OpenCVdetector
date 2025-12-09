#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <allheaders.h>
#include <iostream>

using namespace cv;
using namespace std;

// OCR fonksiyonu (camera_exam_grader'dan kopyalandı)
Pix* matToPix(const Mat& mat) {
    Mat gray;
    if (mat.channels() == 3) cvtColor(mat, gray, COLOR_BGR2GRAY);
    else gray = mat.clone();
    
    Pix* pix = pixCreate(gray.cols, gray.rows, 8);
    for (int y = 0; y < gray.rows; y++) {
        for (int x = 0; x < gray.cols; x++) {
            pixSetPixel(pix, x, y, gray.at<uchar>(y, x));
        }
    }
    return pix;
}

string ocrText(tesseract::TessBaseAPI* ocr, const Mat& roi) {
    if (roi.empty() || roi.cols < 10 || roi.rows < 10) {
        return "";
    }
    
    Mat preprocessed;
    
    // Grayscale
    if (roi.channels() == 3) {
        cvtColor(roi, preprocessed, COLOR_BGR2GRAY);
    } else {
        preprocessed = roi.clone();
    }
    
    // OPTİMİZE EDİLMİŞ PREPROCESSING
    
    // 1. Resize 4x - büyük daha iyi
    resize(preprocessed, preprocessed, Size(), 4.0, 4.0, INTER_CUBIC);
    
    // 2. Bilateral filter - kenarları koru, gürültüyü azalt
    bilateralFilter(preprocessed.clone(), preprocessed, 9, 75, 75);
    
    // 3. CLAHE - kontrast artır
    Ptr<CLAHE> clahe = createCLAHE(3.0, Size(8, 8));
    clahe->apply(preprocessed, preprocessed);
    
    // 4. Otsu threshold
    threshold(preprocessed, preprocessed, 0, 255, THRESH_BINARY | THRESH_OTSU);
    
    // 5. İnce gürültüleri temizle
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(2, 2));
    morphologyEx(preprocessed, preprocessed, MORPH_OPEN, kernel);
    
    // 6. Border
    copyMakeBorder(preprocessed, preprocessed, 50, 50, 50, 50, 
                  BORDER_CONSTANT, Scalar(255));
    
    // DEBUG: Preprocessed görseli kaydet (test için)
    static int debugCounter = 0;
    debugCounter++;
    imwrite("test_preprocessed_q" + to_string(debugCounter) + ".jpg", preprocessed);
    
    // Tesseract
    Pix* pix = matToPix(preprocessed);
    if (!pix) return "";
    
    ocr->SetImage(pix);
    ocr->SetPageSegMode(tesseract::PSM_RAW_LINE); // RAW_LINE daha toleranslı
    
    // Whitelist KALD - bazen kısıtlıyor
    // ocr->SetVariable("tessedit_char_whitelist", ...");
    
    // Daha iyi sonuçlar için ek ayarlar
    ocr->SetVariable("tessedit_do_invert", "0");
    ocr->SetVariable("classify_bln_numeric_mode", "0");
    
    char* raw = ocr->GetUTF8Text();
    string text = raw ? raw : "";
    delete[] raw;
    pixDestroy(&pix);
    
    // Temizleme
    transform(text.begin(), text.end(), text.begin(), ::tolower);
    text.erase(remove_if(text.begin(), text.end(),
                        [](char c) { return c == '\n' || c == '\r'; }),
              text.end());
    
    return text;
}

int main() {
    // OCR init
    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY)) {
        cerr << "❌ OCR hatası!" << endl;
        return -1;
    }
    
    // Test ROI'lerini oku
    cout << "\n🧪 OCR HIZLI TEST\n" << endl;
    
    vector<string> correct = {"ankara", "atatürk", "asya avrupa", "arjantin", "sakarya"};
    
    for (int i = 1; i <= 5; i++) {
        string filename = "debug_q" + to_string(i) + ".jpg";
        Mat roi = imread(filename);
        
        if (roi.empty()) {
            cout << "❌ " << filename << " bulunamadı!" << endl;
            continue;
        }
        
        string result = ocrText(ocr, roi);
        bool isCorrect = (result == correct[i-1]);
        
        cout << "Soru " << i << ": \"" << result << "\" "
             << "(Doğru: \"" << correct[i-1] << "\") → " 
             << (isCorrect ? "✅" : "❌") << endl;
    }
    
    ocr->End();
    delete ocr;
    
    return 0;
}

