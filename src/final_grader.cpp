/**
 * EXAM GRADER - GERÇEK KAĞITLARLA ÇALIŞAN VERSİYON
 * 
 * Sayfa 1: 1238x1800 - El yazısı + True/False
 * Sayfa 2: 1232x1782 - Çoktan seçmeli
 */

#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <allheaders.h>
#include <iostream>
#include <vector>
#include <map>

using namespace cv;
using namespace std;

// CEVAP ANAHTARI
map<int, string> answerKey = {
    {1, "ankara"},
    {2, "atatürk"},
    {3, "asya avrupa"},
    {4, "arjantin"},
    {5, "sakarya"}
};

bool tfAnswers[2] = {false, true};
char mcAnswers[2] = {'C', 'D'}; // İlk soru C, ikinci D

int totalScore = 0;

// OCR
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
    Mat preprocessed;
    cvtColor(roi, preprocessed, COLOR_BGR2GRAY);
    
    // OPTİMİZE EDİLMİŞ PREPROCESSING - EL YAZISI İÇİN
    // 1. Optimal resize (4x) - daha az gürültü
    resize(preprocessed, preprocessed, Size(), 4.0, 4.0, INTER_CUBIC);
    
    // 2. Bilateral filter - gürültü azalt ama kenarları koru
    bilateralFilter(preprocessed.clone(), preprocessed, 9, 75, 75);
    
    // 3. Güçlü CLAHE - kontrast artır
    Ptr<CLAHE> clahe = createCLAHE(5.0, Size(8, 8));
    clahe->apply(preprocessed, preprocessed);
    
    // 4. Adaptive threshold 
    adaptiveThreshold(preprocessed, preprocessed, 255, 
                     ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 21, 10);
    
    // 5. Morphological opening - ince gürültüleri temizle
    Mat kernel = getStructuringElement(MORPH_RECT, Size(2, 2));
    morphologyEx(preprocessed, preprocessed, MORPH_OPEN, kernel);
    
    // 6. Border ekle
    copyMakeBorder(preprocessed, preprocessed, 30, 30, 30, 30, 
                  BORDER_CONSTANT, Scalar(255));
    
    Pix* pix = matToPix(preprocessed);
    ocr->SetImage(pix);
    char* raw = ocr->GetUTF8Text();
    string text = raw ? raw : "";
    delete[] raw;
    pixDestroy(&pix);
    
    // Temizle
    transform(text.begin(), text.end(), text.begin(), ::tolower);
    text.erase(remove_if(text.begin(), text.end(),
                        [](char c) { return c == '\n' || c == '\r'; }),
              text.end());
    
    // Baştaki/sondaki boşlukları temizle
    text.erase(0, text.find_first_not_of(" \t"));
    text.erase(text.find_last_not_of(" \t") + 1);
    
    return text;
}

bool isMarked(const Mat& img, Rect roi) {
    if (roi.x < 0 || roi.y < 0 || 
        roi.x + roi.width > img.cols || 
        roi.y + roi.height > img.rows) {
        return false;
    }
    
    Mat r = img(roi);
    Mat gray, thresh;
    cvtColor(r, gray, COLOR_BGR2GRAY);
    threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    
    int filled = countNonZero(thresh);
    int total = thresh.rows * thresh.cols;
    double fillRatio = (double)filled / total;
    
    // Threshold düşürüldü: %5'ten fazla siyah pixel varsa işaretli say
    // (X işaretleri için daha hassas)
    return fillRatio > 0.05;
}

int compareAnswer(string student, string correct) {
    if (student.empty()) return 0;
    
    // Türkçe karakter normalizasyonu
    auto normalize = [](string s) {
        transform(s.begin(), s.end(), s.begin(), ::tolower);
        // Büyük Türkçe karakterleri küçült
        size_t pos;
        while ((pos = s.find("İ")) != string::npos) s.replace(pos, 2, "i");
        while ((pos = s.find("Ü")) != string::npos) s.replace(pos, 2, "ü");
        while ((pos = s.find("Ö")) != string::npos) s.replace(pos, 2, "ö");
        while ((pos = s.find("Ş")) != string::npos) s.replace(pos, 2, "ş");
        while ((pos = s.find("Ç")) != string::npos) s.replace(pos, 2, "ç");
        while ((pos = s.find("Ğ")) != string::npos) s.replace(pos, 2, "ğ");
        return s;
    };
    
    string normStudent = normalize(student);
    string normCorrect = normalize(correct);
    
    // Tam eşleşme
    if (normStudent == normCorrect) return 12;
    if (normStudent.find(normCorrect) != string::npos || 
        normCorrect.find(normStudent) != string::npos) return 12;
    
    // Levenshtein distance benzeri basit kontrol
    int matchCount = 0;
    for (size_t i = 0; i < min(normStudent.length(), normCorrect.length()); i++) {
        if (normStudent[i] == normCorrect[i]) matchCount++;
    }
    
    double matchRatio = (double)matchCount / normCorrect.length();
    if (matchRatio > 0.8) return 12;  // %80+ karakter eşleşmesi
    if (matchRatio > 0.6) return 8;   // %60+ karakter eşleşmesi
    
    // Karakter benzerliği
    int common = 0;
    for (char c : normStudent) {
        if (normCorrect.find(c) != string::npos) common++;
    }
    if (common > normCorrect.length() * 0.5) return 6;
    
    return 0;
}

int main() {
    cout << "\n🎓 EXAM GRADER - GERÇEK KAĞIT VERSİYONU\n" << endl;
    
    // OCR başlat - TÜRKÇE LSTM + ÇEŞİTLİ PSM MODLARI DENEYECEĞİZ
    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY)) {
        cerr << "❌ OCR hatası!" << endl;
        return -1;
    }
    // PSM_SINGLE_WORD yerine PSM_SINGLE_LINE daha iyi olabilir el yazısı için
    ocr->SetPageSegMode(tesseract::PSM_RAW_LINE);
    
    // Sayfa 1 oku
    Mat page1 = imread("page1.png");
    if (page1.empty()) {
        cerr << "❌ page1.png bulunamadı!" << endl;
        return -1;
    }
    
    cout << "📄 SAYFA 1 (1238x1800)\n" << endl;
    cout << "📝 El Yazısı:\n" << endl;
    
    // EL YAZISI ROI'leri - KULLANICI TARAFINDAN 4 KÖŞE İLE İŞARETLENDİ
    vector<Rect> handwriting = {
        Rect(251, 338, 278, 81),   // Soru 1: ANKARA
        Rect(260, 509, 306, 82),   // Soru 2: ATATÜRK
        Rect(259, 662, 470, 91),   // Soru 3: ASYA AVRUPA
        Rect(272, 821, 384, 95),   // Soru 4: ARJANTİN
        Rect(275, 976, 406, 85)    // Soru 5: SAKARYA
    };
    
    for (int i = 0; i < 5; i++) {
        string answer = ocrText(ocr, page1(handwriting[i]));
        int score = compareAnswer(answer, answerKey[i+1]);
        totalScore += score;
        
        cout << "  Soru " << (i+1) << ": \"" << answer << "\"";
        cout << " → " << (score >= 10 ? "✅" : score > 0 ? "⚠" : "❌");
        cout << " " << score << "p" << endl;
        
        // Debug: ROI'yi kaydet
        imwrite("debug_q" + to_string(i+1) + ".jpg", page1(handwriting[i]));
    }
    
    // TRUE/FALSE - KULLANICI TARAFINDAN 4 KÖŞE İLE İŞARETLENDİ
    cout << "\n✓/✗ True/False:\n" << endl;
    
    vector<pair<Rect, Rect>> tfBoxes = {
        {Rect(130, 1161, 493, 94), Rect(130, 1161, 493, 94)},  // Soru 1: Tüm satır (TRUE/FALSE ikisi de)
        {Rect(141, 1309, 459, 128), Rect(141, 1309, 459, 128)} // Soru 2: Tüm satır
    };
    
    // Manuel olarak TRUE ve FALSE kutucuklarını ayır - KULLANICI İŞARETLEDİ
    vector<pair<Rect, Rect>> tfCheckboxes = {
        {Rect(145, 1223, 45, 45), Rect(315, 1223, 45, 45)}, // Soru 1: TRUE, FALSE
        {Rect(140, 1326, 184, 103), Rect(424, 1319, 201, 104)}  // Soru 2: TRUE, FALSE (kullanıcı işaretledi)
    };
    
    for (int i = 0; i < 2; i++) {
        // DEBUG: Fill ratio'ları göster
        Mat trueROI = page1(tfCheckboxes[i].first);
        Mat falseROI = page1(tfCheckboxes[i].second);
        
        Mat grayT, grayF, threshT, threshF;
        cvtColor(trueROI, grayT, COLOR_BGR2GRAY);
        cvtColor(falseROI, grayF, COLOR_BGR2GRAY);
        threshold(grayT, threshT, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        threshold(grayF, threshF, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        
        double trueFill = (double)countNonZero(threshT) / (threshT.rows * threshT.cols);
        double falseFill = (double)countNonZero(threshF) / (threshF.rows * threshF.cols);
        
        cout << "  [DEBUG] Soru " << (i+1) << ": TRUE=" << (trueFill*100) << "% | FALSE=" << (falseFill*100) << "%" << endl;
        
        bool trueChecked = isMarked(page1, tfCheckboxes[i].first);
        bool falseChecked = isMarked(page1, tfCheckboxes[i].second);
        
        bool studentAnswer = trueChecked;
        bool correct = (studentAnswer == tfAnswers[i]);
        int score = correct ? 10 : 0;
        totalScore += score;
        
        cout << "  Soru " << (i+1) << ": " << (studentAnswer ? "TRUE" : "FALSE");
        cout << " → " << (correct ? "✅" : "❌") << " " << score << "p" << endl;
    }
    
    // Sayfa 2
    Mat page2 = imread("page2.png");
    if (page2.empty()) {
        cerr << "❌ page2.png bulunamadı!" << endl;
        return -1;
    }
    
    cout << "\n📄 SAYFA 2 (1232x1782)\n" << endl;
    cout << "🔘 Çoktan Seçmeli:\n" << endl;
    
    // Her soru için A, B, C, D şıklarını kontrol et - HASSAS AYARLANMIŞ
    vector<vector<Rect>> mcOptions = {
        { // Soru 1 - Grid'den doğrulandı
            Rect(90, 310, 80, 30),    // A-) 2 (Y=304)
            Rect(90, 337, 80, 30),    // B-) 3 (Y=334)
            Rect(90, 356, 80, 35),    // C-) 6  ← İŞARETLİ (Y=359) ✅ DOĞRU
            Rect(90, 387, 80, 30)     // D-) 9 (Y=387)
        },
        { // Soru 2 - Grid'den DÜZELTİLDİ
            Rect(100, 503, 90, 25),   // A-) Car (Y=503)
            Rect(100, 530, 140, 25),  // B-) Football ball (Y=530)
            Rect(100, 557, 90, 25),   // C-) Food (Y=557)
            Rect(100, 581, 140, 35)   // D-) AI tool  ← İŞARETLİ (Y=584) - Daire burada!
        }
    };
    
    char options[] = {'A', 'B', 'C', 'D'};
    
    for (int q = 0; q < 2; q++) {
        char marked = '?';
        double maxFill = 0.0;
        
        // DEBUG: Her şıkkın fill ratio'sunu göster
        cout << "  [DEBUG] Soru " << (q+1) << " şıkları:" << endl;
        
        for (int opt = 0; opt < 4; opt++) {
            Rect roi = mcOptions[q][opt];
            Mat r = page2(roi);
            Mat gray, thresh;
            cvtColor(r, gray, COLOR_BGR2GRAY);
            threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
            
            int filled = countNonZero(thresh);
            int total = thresh.rows * thresh.cols;
            double fillRatio = (double)filled / total;
            
            cout << "    " << options[opt] << ": " << (fillRatio * 100) << "% ";
            if (fillRatio > 0.10) cout << "✓ İŞARETLİ";
            cout << endl;
            
            // En yüksek fill ratio'ya sahip şık = işaretli
            if (fillRatio > maxFill) {
                maxFill = fillRatio;
                marked = options[opt];
            }
        }
        
        bool correct = (marked == mcAnswers[q]);
        int score = correct ? 10 : 0;
        totalScore += score;
        
        cout << "  Soru " << (q+1) << ": " << marked;
        cout << " (Doğru: " << mcAnswers[q] << ")";
        cout << " → " << (correct ? "✅" : "❌") << " " << score << "p" << endl;
    }
    
    // SONUÇ
    cout << "\n" << string(50, '=') << endl;
    cout << "🎯 TOPLAM PUAN: " << totalScore << " / 100" << endl;
    cout << string(50, '=') << "\n" << endl;
    
    ocr->End();
    delete ocr;
    
    return 0;
}