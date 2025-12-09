#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <locale>
#include <codecvt>

using namespace cv;
using namespace std;

// OCR fonksiyonu (final_grader'dan kopyalandı)
string ocrText(const Mat& roi, tesseract::TessBaseAPI* ocr) {
    if (roi.empty()) return "";
    
    try {
        // Önce 4x büyüt
        Mat enlarged;
        resize(roi, enlarged, Size(), 4.0, 4.0, INTER_CUBIC);
        
        // Bilateral filter (gürültü temizleme ama kenarları koruma)
        Mat filtered;
        bilateralFilter(enlarged, filtered, 9, 75, 75);
        
        // CLAHE (Contrast Limited Adaptive Histogram Equalization)
        Mat gray;
        if (filtered.channels() == 3) {
            cvtColor(filtered, gray, COLOR_BGR2GRAY);
        } else {
            gray = filtered.clone();
        }
        
        Ptr<CLAHE> clahe = createCLAHE(5.0, Size(8, 8));
        Mat enhanced;
        clahe->apply(gray, enhanced);
        
        // Adaptive threshold
        Mat binary;
        adaptiveThreshold(enhanced, binary, 255, ADAPTIVE_THRESH_GAUSSIAN_C, 
                         THRESH_BINARY, 21, 10);
        
        // Morfolojik açma (küçük noktaları temizle)
        Mat kernel = getStructuringElement(MORPH_RECT, Size(2, 2));
        morphologyEx(binary, binary, MORPH_OPEN, kernel);
        
        // 30px padding ekle
        Mat padded;
        copyMakeBorder(binary, padded, 30, 30, 30, 30, BORDER_CONSTANT, Scalar(255));
        
        // OCR uygula
        ocr->SetImage(padded.data, padded.cols, padded.rows, 1, padded.step);
        char* outText = ocr->GetUTF8Text();
        string result(outText);
        delete[] outText;
        
        // Temizle
        result.erase(remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(remove(result.begin(), result.end(), '\r'), result.end());
        
        // Boşlukları temizle
        while (!result.empty() && isspace(result.back())) result.pop_back();
        while (!result.empty() && isspace(result.front())) result.erase(0, 1);
        
        return result;
        
    } catch (const exception& e) {
        cerr << "OCR hatası: " << e.what() << endl;
        return "";
    }
}

// Checkbox/bubble işaretli mi kontrol et
bool isMarked(const Mat& roi) {
    if (roi.empty()) return false;
    
    Mat gray;
    if (roi.channels() == 3) {
        cvtColor(roi, gray, COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }
    
    // Binary threshold
    Mat binary;
    threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    
    // Siyah pixel sayısı
    int nonZero = countNonZero(binary);
    double fillRatio = (double)nonZero / (binary.rows * binary.cols);
    
    return fillRatio > 0.05;
}

// Cevap karşılaştırma
bool compareAnswer(const string& student, const string& correct) {
    // Türkçe karakter normalize
    auto normalize = [](string s) {
        transform(s.begin(), s.end(), s.begin(), ::tolower);
        
        // Türkçe karakterleri düzelt
        size_t pos = 0;
        while ((pos = s.find("İ", pos)) != string::npos) {
            s.replace(pos, 2, "i");
        }
        pos = 0;
        while ((pos = s.find("ı", pos)) != string::npos) {
            s.replace(pos, 2, "i");
        }
        
        return s;
    };
    
    string normStudent = normalize(student);
    string normCorrect = normalize(correct);
    
    return normStudent == normCorrect;
}

int main() {
    // Tesseract başlat
    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY)) {
        cerr << "Tesseract başlatılamadı!" << endl;
        return -1;
    }
    ocr->SetPageSegMode(tesseract::PSM_RAW_LINE);
    
    // Kameradan düzeltilmiş görüntüyü oku
    Mat page1 = imread("corrected_page1.jpg");
    if (page1.empty()) {
        cerr << "corrected_page1.jpg bulunamadı!" << endl;
        return -1;
    }
    
    cout << "\n🎓 KAMERADAN DÜZELTİLMİŞ GÖRÜNTÜ TESTİ\n" << endl;
    cout << "📐 Görüntü boyutu: " << page1.cols << "x" << page1.rows << endl;
    
    // El yazısı ROI'leri (final_grader'dan)
    vector<Rect> handwritingROIs = {
        Rect(234, 328, 312, 77),   // Soru 1
        Rect(240, 494, 308, 94),   // Soru 2
        Rect(257, 641, 467, 106),  // Soru 3
        Rect(284, 796, 360, 112),  // Soru 4
        Rect(294, 945, 354, 92)    // Soru 5
    };
    
    vector<string> correctAnswers = {
        "ankara", "atatürk", "asya avrupa", "arjantin", "sakarya"
    };
    
    cout << "\n📝 El Yazısı Testleri:\n" << endl;
    
    int totalScore = 0;
    for (size_t i = 0; i < handwritingROIs.size(); i++) {
        Rect roi = handwritingROIs[i];
        
        // ROI sınırları kontrol
        if (roi.x < 0 || roi.y < 0 || 
            roi.x + roi.width > page1.cols || 
            roi.y + roi.height > page1.rows) {
            cout << "  Soru " << (i+1) << ": ROI SINIRLARIN DIŞINDA!" << endl;
            continue;
        }
        
        Mat textROI = page1(roi);
        string studentAnswer = ocrText(textROI, ocr);
        
        // Debug: ROI'yi kaydet
        string debugPath = "debug_camera_q" + to_string(i+1) + ".jpg";
        imwrite(debugPath, textROI);
        
        bool correct = compareAnswer(studentAnswer, correctAnswers[i]);
        int points = correct ? 12 : 0;
        totalScore += points;
        
        cout << "  Soru " << (i+1) << ": \"" << studentAnswer << "\" ";
        cout << "(Doğru: \"" << correctAnswers[i] << "\") → ";
        cout << (correct ? "✅" : "❌") << " " << points << "p" << endl;
    }
    
    // True/False testleri
    cout << "\n✓/✗ True/False Testleri:\n" << endl;
    
    // Soru 1
    Rect tf1_true(145, 1223, 45, 45);
    Rect tf1_false(315, 1223, 45, 45);
    
    bool s1_true = isMarked(page1(tf1_true));
    bool s1_false = isMarked(page1(tf1_false));
    
    string s1_answer = s1_true ? "TRUE" : (s1_false ? "FALSE" : "BOŞ");
    bool s1_correct = (s1_answer == "FALSE");
    int s1_points = s1_correct ? 10 : 0;
    totalScore += s1_points;
    
    cout << "  Soru 1: " << s1_answer << " (Doğru: FALSE) → ";
    cout << (s1_correct ? "✅" : "❌") << " " << s1_points << "p" << endl;
    
    // Soru 2
    Rect tf2_true(140, 1326, 184, 103);
    Rect tf2_false(424, 1319, 201, 104);
    
    bool s2_true = isMarked(page1(tf2_true));
    bool s2_false = isMarked(page1(tf2_false));
    
    string s2_answer = s2_true ? "TRUE" : (s2_false ? "FALSE" : "BOŞ");
    bool s2_correct = (s2_answer == "TRUE");
    int s2_points = s2_correct ? 10 : 0;
    totalScore += s2_points;
    
    cout << "  Soru 2: " << s2_answer << " (Doğru: TRUE) → ";
    cout << (s2_correct ? "✅" : "❌") << " " << s2_points << "p" << endl;
    
    cout << "\n==================================================\n";
    cout << "🎯 KAMERADAN DÜZELTİLMİŞ GÖRÜNTÜ PUANI: " << totalScore << " / 80" << endl;
    cout << "==================================================\n" << endl;
    
    cout << "💡 Debug görüntüleri: debug_camera_q1.jpg ~ debug_camera_q5.jpg" << endl;
    
    ocr->End();
    delete ocr;
    
    return 0;
}
