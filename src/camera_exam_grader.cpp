/**
 * CAMERA EXAM GRADER - KAMERA İLE CANLI SINAV DEĞERLENDİRME
 * 
 * Kullanım:
 * - '1' tuşu: Sayfa 1'i yakala (El yazısı + True/False)
 * - '2' tuşu: Sayfa 2'yi yakala (Çoktan seçmeli)
 * - 'r' tuşu: Sonuçları göster
 * - 'ESC': Çıkış
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
char mcAnswers[2] = {'C', 'D'};

// REAL-TIME GRADING SONUÇLARI
struct GradingResult {
    bool isValid = false;
    int totalScore = 0;
    vector<string> handwritingAnswers;
    vector<int> handwritingScores;
    vector<bool> tfAnswers;
    vector<int> tfScores;
    vector<char> mcAnswers;
    vector<int> mcScores;
    Mat gradedFrame;  // Overlay ile birlikte frame
};

GradingResult lastGradingResult;  // Son puanlama sonucu

Mat capturedPage1, capturedPage2;
bool page1Ready = false, page2Ready = false;

// Otomatik yakalama için
int stableFrameCount = 0;  // Kağıt kaç frame sabit kaldı
const int REQUIRED_STABLE_FRAMES = 15;  // 15 frame sabit kalması lazım (daha sıkı kontrol)
vector<Point2f> lastDetectedCorners;
vector<vector<Point2f>> recentCorners;  // Son 20 frame'in köşeleri
bool autoCaptureMode = true;  // Otomatik yakalama modu
bool processingPage = false;  // Şu anda sayfa işleniyor mu?
int cooldownFrames = 0;  // Kağıt yakalandıktan sonra cooldown period
const int COOLDOWN_FRAMES = 90;  // 3 saniye cooldown (90 frame @ 30 FPS)

// Manuel köşe seçimi için
vector<Point2f> manualCorners;
bool selectingCorners = false;
Mat tempFrame;

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (!selectingCorners) return;
    
    if (event == EVENT_LBUTTONDOWN && manualCorners.size() < 4) {
        manualCorners.push_back(Point2f(x, y));
        cout << "📍 Köşe " << manualCorners.size() << ": (" << x << ", " << y << ")" << endl;
        
        if (manualCorners.size() == 4) {
            cout << "✅ 4 köşe seçildi! ENTER'a basın." << endl;
        }
    }
}

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
    // DEBUG: ROI kontrolü
    if (roi.empty()) {
        cout << "  [DEBUG OCR] ROI BOŞ!" << endl;
        return "";
    }
    if (roi.cols < 10 || roi.rows < 10) {
        cout << "  [DEBUG OCR] ROI çok küçük: " << roi.cols << "x" << roi.rows << endl;
        return "";
    }
    
    // HAM GÖRÜNTÜ - Preprocessing TAMAMEN KALDIRILDI (bulanıklaştırıyor, gri lekeler oluşturuyor)
    Mat preprocessed;
    
    // 1. Grayscale'e çevir
    if (roi.channels() == 3) {
    cvtColor(roi, preprocessed, COLOR_BGR2GRAY);
    } else {
        preprocessed = roi.clone();
    }
    
    // DEBUG: Preprocessing öncesi kontrol
    Scalar meanBefore = mean(preprocessed);
    cout << "  [DEBUG OCR] Preprocessing öncesi ortalama: " << meanBefore[0] << endl;
    
    // IŞIK DÜZELTMESİ - Karanlık ortamlarda OCR başarısını artırır
    // Adım 1: Karanlıksa gamma correction ile parlatma
    if (meanBefore[0] < 120) {
        preprocessed.convertTo(preprocessed, -1, 1.3, 25); // Contrast + Brightness boost
        cout << "  [DEBUG OCR] Karanlık görüntü tespit edildi, gamma correction uygulandı" << endl;
    }
    
    // Adım 2: CLAHE (Adaptive Histogram Equalization) - Lokal kontrast artırma
    Ptr<CLAHE> clahe = createCLAHE(2.0, Size(8, 8));
    clahe->apply(preprocessed, preprocessed);
    
    // BASİTLEŞTİRİLMİŞ PREPROCESSING - Sadece 3 adım!
    
    // 1. Resize 3x (4x çok büyük, Tesseract için 3x yeterli)
    resize(preprocessed, preprocessed, Size(), 3.0, 3.0, INTER_CUBIC);
    
    // 2. OTSU threshold - basit ve etkili!
    threshold(preprocessed, preprocessed, 0, 255, THRESH_BINARY | THRESH_OTSU);
    
    // 3. Border ekle - OCR için alan ekle
    copyMakeBorder(preprocessed, preprocessed, 20, 20, 20, 20, 
                  BORDER_CONSTANT, Scalar(255));
    
    // DEBUG: Preprocessing sonrası kontrol
    Scalar meanAfter = mean(preprocessed);
    cout << "  [DEBUG OCR] Preprocessing sonrası ortalama: " << meanAfter[0] 
         << ", Boyut: " << preprocessed.cols << "x" << preprocessed.rows 
         << " (BASİT pipeline: resize 3x + OTSU threshold + border)" << endl;
    
    // Tesseract'a gönder - final_grader gibi
    Pix* pix = matToPix(preprocessed);
    if (!pix) {
        cout << "  [DEBUG OCR] Pix dönüşümü BAŞARISIZ!" << endl;
        return "";
    }
    
    ocr->SetImage(pix);
    
    // final_grader ile AYNI ayarlar - PSM_RAW_LINE ve whitelist YOK
    ocr->SetPageSegMode(tesseract::PSM_RAW_LINE); // final_grader ile aynı
    
    // Whitelist KALDIRILDI - final_grader'da yok, tüm karakterlere izin ver
    
    char* raw = ocr->GetUTF8Text();
    string text = raw ? raw : "";
    delete[] raw;
    pixDestroy(&pix);
    
    // DEBUG: Ham OCR sonucunu yazdır
    cout << "  [DEBUG OCR] Ham sonuç: \"" << text << "\"" << endl;
    
    // Eğer birden fazla satır varsa, sadece son satırı al (cevap satırı)
    // Çünkü ROI'de hala soru metni varsa, ilk satır soru, son satır cevap
    if (!text.empty()) {
        size_t lastNewline = text.find_last_of("\n\r");
        if (lastNewline != string::npos && lastNewline < text.length() - 1) {
            // Son satırı al
            text = text.substr(lastNewline + 1);
            cout << "  [DEBUG OCR] Birden fazla satır bulundu, son satır alındı: \"" << text << "\"" << endl;
        }
    }
    
    // Metin temizleme
    transform(text.begin(), text.end(), text.begin(), ::tolower);
    text.erase(remove_if(text.begin(), text.end(),
                        [](char c) { return c == '\n' || c == '\r'; }),
              text.end());
    
    // Baştaki ekstra karakterleri temizle (örn: "w", "wd", "wr" gibi)
    // Sadece alfabetik karakterler ve boşlukları tut
    if (!text.empty()) {
        // Baştan itibaren ilk geçerli karakteri bul (alfabetik veya boşluk)
        size_t start = 0;
        while (start < text.length() && !isalpha(text[start]) && text[start] != ' ') {
            start++;
        }
        if (start > 0) {
            text = text.substr(start);
            cout << "  [DEBUG OCR] Baştaki " << start << " karakter temizlendi: \"" << text << "\"" << endl;
        }
        
        // Sadece alfabetik karakterler ve boşlukları tut (noktalama ve özel karakterleri kaldır)
        string cleaned;
        for (char c : text) {
            if (isalpha(c) || c == ' ') {
                cleaned += c;
            }
        }
        text = cleaned;
        
        // Baştaki/sondaki boşlukları temizle
        if (!text.empty()) {
    text.erase(0, text.find_first_not_of(" \t"));
    text.erase(text.find_last_not_of(" \t") + 1);
        }
    }
    
    cout << "  [DEBUG OCR] Temizlenmiş sonuç: \"" << text << "\"" << endl;
    
    return text;
}

// X PATTERN DETECTION - Köşegen çizgileri tespit et (True/False için)
bool detectXPattern(const Mat& roi) {
    Mat gray;
    if (roi.channels() == 3) {
        cvtColor(roi, gray, COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }
    
    // OTSU threshold
    Mat binary;
    threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    
    // HoughLines ile köşegen çizgileri bul
    vector<Vec4i> lines;
    HoughLinesP(binary, lines, 1, CV_PI/180, 10, 10, 5); // Threshold 5→10, minLength 3→10 (daha katı!)
    
    // Köşegen çizgileri say (45° ± 30° açılı)
    int diagonalLines = 0;
    for (const auto& line : lines) {
        double dx = line[2] - line[0];
        double dy = line[3] - line[1];
        if (abs(dx) < 1 && abs(dy) < 1) continue; // Çok kısa çizgi
        
        double angle = atan2(abs(dy), abs(dx)) * 180.0 / CV_PI;
        
        // Köşegen açılar: 30° - 60° arası (daha dar aralık!)
        if (angle > 30 && angle < 60) {
            diagonalLines++;
        }
    }
    
    // EN AZ 4 köşegen çizgi olmalı (X için 2 çizgi, ama gürültü için 4!)
    return diagonalLines >= 4;
}

bool isMarked(const Mat& img, Rect roi) {
    if (roi.x < 0 || roi.y < 0 || 
        roi.x + roi.width > img.cols || 
        roi.y + roi.height > img.rows) {
        return false;
    }
    
    Mat r = img(roi);
    
    // X PATTERN DETECTION KALDIRILDI - yanlış pozitif veriyor!
    // Sadece fill ratio kullan
    
    // Fill ratio kontrolü
    Mat gray;
    cvtColor(r, gray, COLOR_BGR2GRAY);
    
    // OTSU threshold
    Mat thresh;
    threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    
    // Morfolojik işlemler - küçük gürültüleri temizle
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(thresh, thresh, MORPH_CLOSE, kernel);
    
    int filled = countNonZero(thresh);
    int total = thresh.rows * thresh.cols;
    double fillRatio = (double)filled / total;
    
    // True/False için %30 threshold (X işareti için)
    // %5 çok düşük, boş kutular bile işaretli sayılıyor!
    return fillRatio > 0.30;
}

int compareAnswer(string student, string correct) {
    if (student.empty()) return 0;
    
    // Türkçe karakter normalizasyonu (final_grader ile aynı)
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
    
    // Çok kısa cevapları reddet (1-2 karakter = muhtemelen hata)
    if (normStudent.length() < 3 && normCorrect.length() > 3) {
        return 0; // "a" gibi kısa cevaplar "ankara" ile eşleşmemeli
    }
    
    // Tam eşleşme
    if (normStudent == normCorrect) return 12;
    
    // Alt string kontrolü - ama çok kısa değilse
    if (normStudent.length() >= normCorrect.length() * 0.5) {
    if (normStudent.find(normCorrect) != string::npos || 
        normCorrect.find(normStudent) != string::npos) return 12;
    }
    
    // Levenshtein distance benzeri basit kontrol (final_grader ile aynı)
    int matchCount = 0;
    for (size_t i = 0; i < min(normStudent.length(), normCorrect.length()); i++) {
        if (normStudent[i] == normCorrect[i]) matchCount++;
    }
    
    double matchRatio = (double)matchCount / normCorrect.length();
    if (matchRatio > 0.8) return 12;  // %80+ karakter eşleşmesi
    if (matchRatio > 0.6) return 8;   // %60+ karakter eşleşmesi
    
    // Karakter benzerliği (final_grader ile aynı)
    int common = 0;
    for (char c : normStudent) {
        if (normCorrect.find(c) != string::npos) common++;
    }
    if (common > normCorrect.length() * 0.5) return 6;
    
    return 0;
}

Mat resizeToTemplate(const Mat& img, Size targetSize) {
    Mat resized;
    resize(img, resized, targetSize, 0, 0, INTER_LINEAR);
    return resized;
}

// Kağıt doğrulama fonksiyonu - gerçekten kağıt mı kontrol et
bool isValidPaper(const Mat& image, const vector<Point2f>& corners) {
    if (corners.size() != 4) return false;
    
    // 1. BOYUT ORANI KONTROLÜ - A4 kağıt yaklaşık 1:1.414 (dikey) veya 1.414:1 (yatay)
    double width1 = norm(corners[0] - corners[1]);
    double width2 = norm(corners[2] - corners[3]);
    double height1 = norm(corners[0] - corners[3]);
    double height2 = norm(corners[1] - corners[2]);
    
    double avgWidth = (width1 + width2) / 2.0;
    double avgHeight = (height1 + height2) / 2.0;
    
    double aspectRatio = avgWidth / avgHeight;
    if (aspectRatio < 1.0) aspectRatio = 1.0 / aspectRatio; // Her zaman > 1
    
    // A4 oranı yaklaşık 1.414, ama çok esnek tutalım: 1.0 - 2.5 arası
    if (aspectRatio < 0.8 || aspectRatio > 3.0) {
        return false; // Kağıt oranı değil
    }
    
    // 2. BEYAZLIK KONTROLÜ - Kağıt çok beyaz olmalı (daha esnek)
    vector<Point> cornerPoints = {
        Point((int)corners[0].x, (int)corners[0].y),
        Point((int)corners[1].x, (int)corners[1].y),
        Point((int)corners[2].x, (int)corners[2].y),
        Point((int)corners[3].x, (int)corners[3].y)
    };
    
    Rect bbox = cv::boundingRect(cornerPoints);
    
    // ROI'yi güvenli şekilde al - biraz padding ekle
    int padding = 10;
    bbox.x = max(0, bbox.x - padding);
    bbox.y = max(0, bbox.y - padding);
    bbox.width = min(image.cols - bbox.x, bbox.width + 2*padding);
    bbox.height = min(image.rows - bbox.y, bbox.height + 2*padding);
    
    if (bbox.width < 50 || bbox.height < 50) {
        return false; // Çok küçük
    }
    
    Mat roi = image(bbox);
    Mat grayRoi;
    cvtColor(roi, grayRoi, COLOR_BGR2GRAY);
    
    // Ortalama parlaklık kontrolü - kağıt beyaz olmalı (çok esnek: >100)
    Scalar meanBrightness = mean(grayRoi);
    if (meanBrightness[0] < 100) { // Çok karanlık, kağıt değil
        return false;
    }
    
    // Beyaz pixel oranı kontrolü - %25'ten fazla beyaz olmalı (çok esnek)
    Mat whiteMask;
    threshold(grayRoi, whiteMask, 160, 255, THRESH_BINARY); // Daha düşük threshold
    double whiteRatio = (double)countNonZero(whiteMask) / (grayRoi.rows * grayRoi.cols);
    if (whiteRatio < 0.25) { // %25'ten az beyaz, kağıt değil
        return false;
    }
    
    // 3. MERKEZ KONTROLÜ - Kağıt görüntünün merkezine yakın olmalı (daha esnek)
    Point2f center(0, 0);
    for (const auto& p : corners) center += p;
    center *= (1.0 / corners.size());
    
    Point2f imageCenter(image.cols / 2.0, image.rows / 2.0);
    double distFromCenter = norm(center - imageCenter);
    double maxDist = sqrt(image.cols * image.cols + image.rows * image.rows) * 0.6; // %60 mesafe (daha esnek)
    
    // Merkez kontrolü opsiyonel - çok katı değil
    // if (distFromCenter > maxDist) {
    //     return false; // Çok kenarda, muhtemelen kağıt değil
    // }
    
    // 4. BOYUT KONTROLÜ - Çok küçük veya çok büyük olmamalı (daha esnek)
    double area = contourArea(cornerPoints);
    
    double imageArea = image.rows * image.cols;
    double areaRatio = area / imageArea;
    
    // Kağıt görüntünün %5-70'i kadar olabilir (daha geniş aralık)
    if (areaRatio < 0.05 || areaRatio > 0.75) {
        return false;
    }
    
    return true; // Tüm kontrollerden geçti!
}

// BEYAZ KAĞIT TESPİTİ - HSV RENK FİLTRESİ + MORFOLOJİK İŞLEMLER
vector<Point2f> detectPaperCorners(const Mat& image) {
    vector<Point2f> bestCorners;
    double bestArea = 0;
    
    static int frameCount = 0;
    frameCount++;
    
    // 1. BGR to HSV
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);
    
    // 2. BEYAZ RENK MASKESİ OLUŞTUR (Gevşetildi - daha fazla beyaz alan yakala)
    // Beyaz: S (saturation) düşük, V (value) yüksek
    // Threshold'ları gevşettik: V 200 → 150, S 30 → 50 (daha fazla beyaz alan)
    Mat whiteMask;
    inRange(hsv, Scalar(0, 0, 150), Scalar(180, 50, 255), whiteMask); // Beyaz alanlar (gevşetildi)
    
    // Maskeyi biraz genişlet (morfolojik açılım)
    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(whiteMask, whiteMask, MORPH_CLOSE, kernel, Point(-1, -1), 2);
    morphologyEx(whiteMask, whiteMask, MORPH_OPEN, kernel, Point(-1, -1), 1);
    
    // 3. GAUSSIAN BLUR (maskelenmiş görüntü üzerinde)
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Mat maskedGray;
    gray.copyTo(maskedGray, whiteMask); // Sadece beyaz alanları kullan
    
    Mat blurred;
    GaussianBlur(maskedGray, blurred, Size(9, 9), 0);
    
    // 4. CANNY EDGE DETECTION
    Mat edges;
    Canny(blurred, edges, 50, 150);
    
    // 5. MORFOLOJİK İŞLEMLER (Dilate + Erode)
    Mat morphKernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    dilate(edges, edges, morphKernel, Point(-1, -1), 2); // Kenarları birleştir
    erode(edges, edges, morphKernel, Point(-1, -1), 1);  // Gürültüyü azalt
    
    // 6. FIND CONTOURS
    vector<vector<Point>> contours;
    findContours(edges, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    // DEBUG: Kontur sayısını yazdır
    int totalContours = contours.size();
    if (frameCount % 30 == 0) {
        cout << "DEBUG: Toplam " << totalContours << " kontur bulundu (beyaz mask sonrası)" << endl;
    }
    
    // 7. APPROXPOLYDP + FİLTRELEME
    double minAreaThreshold = image.rows * image.cols * 0.15; // %15 minimum alan
    double maxAreaThreshold = image.rows * image.cols * 0.75; // %75 maksimum alan
    
    vector<pair<double, int>> contourInfo; // (area, corner_count) - DEBUG için
    
    for (const auto& contour : contours) {
        double area = contourArea(contour);
        
        // Minimum/Maksimum alan kontrolü
        if (area < minAreaThreshold || area > maxAreaThreshold) continue;
        
        // 7. APPROXPOLYDP - EPSILON 0.05-0.08 ARASI
        double peri = arcLength(contour, true);
            vector<Point> approx;
        
        // Epsilon değerlerini dene (0.05, 0.06, 0.07, 0.08)
        for (double epsilon = 0.05; epsilon <= 0.08; epsilon += 0.01) {
            approxPolyDP(contour, approx, peri * epsilon, true);
            
            // 8. SADECE 4 KÖŞELİ OLANLARI AL
            if (approx.size() == 4) {
                // DEBUG: Kontur bilgisini kaydet
                contourInfo.push_back({area, 4});
                
                // 4 köşeyi Point2f'e çevir
                vector<Point2f> testCorners;
                for (const auto& p : approx) {
                    testCorners.push_back(Point2f(p.x, p.y));
                }
                
                // 9. DİKDÖRTGENSELİK KONTROLLERİ (DEBUG LOG İLE)
                
                // 9.1. Convex kontrolü - KALDIRILDI (çok sıkı)
                // vector<Point> contourPoints(contour.begin(), contour.end());
                // if (!isContourConvex(contourPoints)) continue;
                
                // 9.2. Köşelerin birbirine çok yakın olmaması
                bool validCorners = true;
                for (int i = 0; i < 4; i++) {
                    for (int j = i + 1; j < 4; j++) {
                        double dist = norm(testCorners[i] - testCorners[j]);
                        if (dist < 50) { // En az 50 pixel uzaklık
                            validCorners = false;
                            break;
                        }
                    }
                    if (!validCorners) break;
                }
                if (!validCorners) {
                    if (frameCount % 30 == 0 && area > minAreaThreshold) {
                        cout << "  [DEBUG] Kontur reddedildi: Köşeler çok yakın" << endl;
                    }
                    continue;
                }
                
                // 9.3. Boyut hesaplama
                double width1 = norm(testCorners[0] - testCorners[1]);
                double width2 = norm(testCorners[2] - testCorners[3]);
                double height1 = norm(testCorners[0] - testCorners[3]);
                double height2 = norm(testCorners[1] - testCorners[2]);
                
                double avgWidth = (width1 + width2) / 2.0;
                double avgHeight = (height1 + height2) / 2.0;
                
                // 9.4. Aspect ratio kontrolü (Gevşetildi: 0.3-3.0)
                double aspectRatio = avgWidth / avgHeight;
                if (aspectRatio < 1.0) aspectRatio = 1.0 / aspectRatio;
                
                if (aspectRatio < 0.3 || aspectRatio > 3.0) {
                    if (frameCount % 30 == 0 && area > minAreaThreshold) {
                        cout << "  [DEBUG] Kontur reddedildi: Aspect ratio " << aspectRatio 
                             << " (0.3-3.0 aralığında değil)" << endl;
                    }
                    continue;
                }
                
                // 9.5. Genişlik/yükseklik farkı kontrolü (dikdörtgen olmalı)
                double widthDiff = abs(width1 - width2) / max(width1, width2);
                double heightDiff = abs(height1 - height2) / max(height1, height2);
                
                if (widthDiff > 0.3 || heightDiff > 0.3) {
                    if (frameCount % 30 == 0 && area > minAreaThreshold) {
                        cout << "  [DEBUG] Kontur reddedildi: Genişlik farkı " << (widthDiff*100) 
                             << "%, Yükseklik farkı " << (heightDiff*100) << "% (>30%)" << endl;
                    }
                    continue;
    }
    
                // 9.6. Açı kontrolü (Gevşetildi: 50-130 derece)
                bool validAngles = true;
                vector<double> angles;
                for (int i = 0; i < 4; i++) {
                    Point2f p1 = testCorners[i];
                    Point2f p2 = testCorners[(i + 1) % 4];
                    Point2f p3 = testCorners[(i + 2) % 4];
                    
                    Point2f v1 = p2 - p1;
                    Point2f v2 = p3 - p2;
                    
                    double dot = v1.x * v2.x + v1.y * v2.y;
                    double mag1 = norm(v1);
                    double mag2 = norm(v2);
                    
                    if (mag1 > 0 && mag2 > 0) {
                        double cosAngle = dot / (mag1 * mag2);
                        cosAngle = max(-1.0, min(1.0, cosAngle)); // Clamp to [-1, 1]
                        double angle = acos(cosAngle) * 180.0 / CV_PI;
                        angles.push_back(angle);
                        
                        // Açı 50-130 derece arası olmalı (90° ± 40°)
                        if (angle < 50 || angle > 130) {
                            validAngles = false;
                        }
                    }
                }
                
                if (!validAngles) {
                    if (frameCount % 30 == 0 && area > minAreaThreshold) {
                        cout << "  [DEBUG] Kontur reddedildi: Açılar ";
                        for (double a : angles) cout << a << "° ";
                        cout << "(50-130° aralığında değil)" << endl;
                    }
                    continue;
                }
                
                // TÜM KONTROLLERDEN GEÇTİ - DEBUG LOG
                if (frameCount % 30 == 0 && area > minAreaThreshold) {
                    cout << "  [DEBUG] ✅ Tüm kontrollerden geçti!" << endl;
                    cout << "    - Aspect ratio: " << aspectRatio << " ✓" << endl;
                    cout << "    - Genişlik farkı: " << (widthDiff*100) << "%, Yükseklik farkı: " << (heightDiff*100) << "% ✓" << endl;
                    cout << "    - Açılar: ";
                    for (double a : angles) cout << a << "° ";
                    cout << "✓" << endl;
                }
                
                // TÜM KONTROLLERDEN GEÇTİ - En büyük alanı seç
                if (area > bestArea) {
                    bestArea = area;
                    bestCorners = testCorners;
                }
                
                break; // 4 köşe bulundu, diğer epsilon değerlerini dene
            }
        }
    }
    
    // DEBUG: Her 30 frame'de bir bilgi yazdır
    if (frameCount % 30 == 0) {
        cout << "DEBUG: Toplam " << totalContours << " kontur bulundu" << endl;
        cout << "DEBUG: Minimum alan eşiği: " << minAreaThreshold 
             << " (" << (minAreaThreshold / (image.rows * image.cols) * 100) << "% of frame)" << endl;
        
        // En büyük 5 konturu göster
        sort(contourInfo.begin(), contourInfo.end(), greater<pair<double, int>>());
        cout << "DEBUG: En büyük 5 kontur:" << endl;
        for (size_t i = 0; i < min((size_t)5, contourInfo.size()); i++) {
            double area = contourInfo[i].first;
            int corners = contourInfo[i].second;
            cout << "  " << (i+1) << ". Alan: " << area 
                 << " (" << (area / (image.rows * image.cols) * 100) << "%), Köşe: " << corners << endl;
        }
        
        if (bestCorners.size() == 4) {
            cout << "DEBUG: ✅ Kağıt tespit edildi! Alan: " << bestArea 
                 << " (" << (bestArea / (image.rows * image.cols) * 100) << "% of frame)" << endl;
        } else {
            cout << "DEBUG: ❌ Kağıt tespit edilemedi" << endl;
        }
    }
    
    // Eğer geçerli kağıt bulunamazsa, boş döndür
    if (bestCorners.size() != 4 || bestArea == 0) {
        return vector<Point2f>();
    }
    
    // Köşeleri DOĞRU SIRALAMA: top-left, top-right, bottom-right, bottom-left
    // 1. Önce y koordinatına göre sırala (üst 2, alt 2 ayır)
    vector<Point2f> sortedByY = bestCorners;
    sort(sortedByY.begin(), sortedByY.end(), 
         [](const Point2f& a, const Point2f& b) { return a.y < b.y; });
    
    // 2. Üst 2'yi x'e göre sırala (sol=top-left, sağ=top-right)
    vector<Point2f> top(2);
    top[0] = sortedByY[0];
    top[1] = sortedByY[1];
    if (top[0].x > top[1].x) swap(top[0], top[1]);
    
    // 3. Alt 2'yi x'e göre sırala (sol=bottom-left, sağ=bottom-right)
    vector<Point2f> bottom(2);
    bottom[0] = sortedByY[2];
    bottom[1] = sortedByY[3];
    if (bottom[0].x > bottom[1].x) swap(bottom[0], bottom[1]);
    
    // 4. Final sıralama: top-left, top-right, bottom-right, bottom-left
    vector<Point2f> ordered(4);
    ordered[0] = top[0];      // top-left
    ordered[1] = top[1];      // top-right
    ordered[2] = bottom[1];   // bottom-right
    ordered[3] = bottom[0];   // bottom-left
    
    return ordered;
}

// Görüntü iyileştirme fonksiyonu - kamera görüntüleri için (DENGELİ VERSİYON)
Mat enhanceImage(const Mat& image) {
    Mat enhanced = image.clone();
    
    // 1. Hafif gürültü azaltma - bilateral filter (kenarları korur)
    Mat denoised;
    bilateralFilter(enhanced, denoised, 9, 75, 75); // Orta seviye
    
    // 2. Hafif kontrast artırma - CLAHE
    Mat gray;
    cvtColor(denoised, gray, COLOR_BGR2GRAY);
    Ptr<CLAHE> clahe = createCLAHE(3.0, Size(8, 8)); // Dengeli
    Mat enhancedGray;
    clahe->apply(gray, enhancedGray);
    
    // 3. Hafif sharpening (unsharp masking) - çok agresif değil
    Mat blurred;
    GaussianBlur(enhancedGray, blurred, Size(0, 0), 2);
    Mat sharpened;
    addWeighted(enhancedGray, 1.3, blurred, -0.3, 0, sharpened); // Hafif
    
    // 4. Grayscale'i BGR'ye geri çevir
    cvtColor(sharpened, enhanced, COLOR_GRAY2BGR);
    
    return enhanced;
}

// OVERLAY - Puanlama sonuçlarını kamera görüntüsüne çiz
Mat drawGradingOverlay(const Mat& frame, const GradingResult& result) {
    Mat overlay = frame.clone();
    
    if (!result.isValid) {
        return overlay;
    }
    
    // Şeffaf arka plan için overlay
    Mat background = overlay.clone();
    rectangle(background, Point(10, 10), Point(450, 380), Scalar(0, 0, 0), FILLED);
    addWeighted(overlay, 0.7, background, 0.3, 0, overlay);
    
    int yPos = 40;
    int lineHeight = 35;
    
    // TOPLAM SKOR (BÜYÜK)
    string scoreText = "TOPLAM: " + to_string(result.totalScore) + "/100";
    Scalar scoreColor = result.totalScore >= 50 ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
    putText(overlay, scoreText, Point(20, yPos), 
           FONT_HERSHEY_DUPLEX, 1.2, scoreColor, 3);
    yPos += lineHeight + 10;
    
    // EL YAZISI SONUÇLARI
    putText(overlay, "EL YAZISI:", Point(20, yPos), 
           FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    yPos += lineHeight - 5;
    
    for (size_t i = 0; i < result.handwritingAnswers.size(); i++) {
        string text = "S" + to_string(i + 1) + ": " + result.handwritingAnswers[i];
        string pointsText = " (" + to_string(result.handwritingScores[i]) + "p)";
        
        Scalar color = result.handwritingScores[i] >= 10 ? Scalar(0, 255, 0) : 
                      (result.handwritingScores[i] > 0 ? Scalar(0, 255, 255) : Scalar(0, 0, 255));
        
        string mark = result.handwritingScores[i] >= 10 ? " ✓" : 
                     (result.handwritingScores[i] > 0 ? " ~" : " ✗");
        
        putText(overlay, text + pointsText + mark, Point(30, yPos), 
               FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        yPos += lineHeight - 5;
    }
    
    yPos += 10;
    
    // TRUE/FALSE SONUÇLARI
    putText(overlay, "D/Y:", Point(20, yPos), 
           FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    yPos += lineHeight - 5;
    
    for (size_t i = 0; i < result.tfAnswers.size(); i++) {
        string answer = result.tfAnswers[i] ? "TRUE" : "FALSE";
        string text = "S" + to_string(i + 1) + ": " + answer + " (" + to_string(result.tfScores[i]) + "p)";
        
        Scalar color = result.tfScores[i] > 0 ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
        string mark = result.tfScores[i] > 0 ? " ✓" : " ✗";
        
        putText(overlay, text + mark, Point(30, yPos), 
               FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        yPos += lineHeight - 5;
    }
    
    yPos += 10;
    
    // MULTIPLE CHOICE SONUÇLARI
    putText(overlay, "C.SECMELI:", Point(20, yPos), 
           FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 255), 2);
    yPos += lineHeight - 5;
    
    for (size_t i = 0; i < result.mcAnswers.size(); i++) {
        string text = "S" + to_string(i + 1) + ": " + string(1, result.mcAnswers[i]) + 
                     " (" + to_string(result.mcScores[i]) + "p)";
        
        Scalar color = result.mcScores[i] > 0 ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
        string mark = result.mcScores[i] > 0 ? " ✓" : " ✗";
        
        putText(overlay, text + mark, Point(30, yPos), 
               FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        yPos += lineHeight - 5;
    }
    
    return overlay;
}

Mat applyPerspectiveTransform(const Mat& image, const vector<Point2f>& corners, Size outputSize) {
    if (corners.size() != 4) return image.clone();
    
    // Görüntüyü önce iyileştir
    Mat enhanced = enhanceImage(image);
    
    vector<Point2f> dst = {
        Point2f(0, 0),
        Point2f(outputSize.width, 0),
        Point2f(outputSize.width, outputSize.height),
        Point2f(0, outputSize.height)
    };
    
    // EN İYİ KALİTE İÇİN: getPerspectiveTransform kullan (findHomography bazen bozuyor)
    Mat transform = getPerspectiveTransform(corners, dst);
    
    Mat result;
    
    // INTER_LANCZOS4 interpolation kullan - EN YÜKSEK KALİTE (scanned foto gibi)
    // Ama eğer çok büyükse INTER_CUBIC kullan (performans için)
    int interpolation = (outputSize.width > 2000 || outputSize.height > 2000) ? 
                       INTER_CUBIC : INTER_LANCZOS4;
    warpPerspective(enhanced, result, transform, outputSize, interpolation, BORDER_CONSTANT, Scalar(255, 255, 255));
    
    // YÖN KONTROLÜ: Eğer kağıt yanlış yönde (döndürülmüş) ise düzelt
    // Beklenen boyut: width < height (dikey kağıt)
    // Eğer width > height ise, kağıt yatay dönmüş demektir
    if (result.cols > result.rows && outputSize.width < outputSize.height) {
        // 90 derece saat yönünün tersine döndür
        Mat rotated;
        rotate(result, rotated, ROTATE_90_COUNTERCLOCKWISE);
        
        // Boyutu hedef boyuta getir - LANCZOS4 ile
        if (rotated.cols != outputSize.width || rotated.rows != outputSize.height) {
            resize(rotated, result, outputSize, 0, 0, INTER_LANCZOS4);
        } else {
            result = rotated;
        }
        
        cout << "⚠️  Kağıt yönü düzeltildi (90° döndürüldü)" << endl;
    }
    
    // KRİTİK: Perspektif düzeltme sonrası TAM BOYUT KONTROLÜ
    // final_grader ROI koordinatları için görüntü TAM OLARAK hedef boyutta olmalı
    if (result.cols != outputSize.width || result.rows != outputSize.height) {
        resize(result, result, outputSize, 0, 0, INTER_LANCZOS4);
        cout << "  [DEBUG] Görüntü boyutu düzeltildi: " << result.cols << "x" << result.rows 
             << " → " << outputSize.width << "x" << outputSize.height << endl;
    }
    
    // Son iyileştirme - scanned foto kalitesi için
    Mat final;
    Mat gray;
    cvtColor(result, gray, COLOR_BGR2GRAY);
    
    // Kontrast artırma - scanned görüntüler için önemli
    Ptr<CLAHE> finalClahe = createCLAHE(2.5, Size(8, 8));
    Mat contrastEnhanced;
    finalClahe->apply(gray, contrastEnhanced);
    
    // Çok hafif sharpening - aşırı değil
    Mat blurred;
    GaussianBlur(contrastEnhanced, blurred, Size(0, 0), 1.5);
    Mat sharpened;
    addWeighted(contrastEnhanced, 1.15, blurred, -0.15, 0, sharpened);
    
    cvtColor(sharpened, final, COLOR_GRAY2BGR);
    
    // KRİTİK: Son iyileştirme adımlarından SONRA kesin olarak hedef boyuta resize et
    // final_grader ROI koordinatları için görüntü TAM OLARAK hedef boyutta olmalı
    if (final.cols != outputSize.width || final.rows != outputSize.height) {
        resize(final, final, outputSize, 0, 0, INTER_LANCZOS4);
        cout << "  [DEBUG] Son iyileştirme sonrası boyut düzeltildi: " << final.cols << "x" << final.rows 
             << " → " << outputSize.width << "x" << outputSize.height << endl;
    }
    
    return final;
}

void processResults(tesseract::TessBaseAPI* ocr) {
    if (!page1Ready || !page2Ready) {
        cout << "\n❌ Her iki sayfayı da çekin! (1 ve 2 tuşları)" << endl;
        return;
    }
    
    // GradingResult'ı temizle ve başlat
    lastGradingResult = GradingResult();
    lastGradingResult.isValid = true;
    lastGradingResult.totalScore = 0;
    
    // Sayfa 1'i perspective correction ile düzelt ve template boyutuna getir
    cout << "\n🔍 Sayfa 1'de kağıt tespiti yapılıyor..." << endl;
    vector<Point2f> corners1 = detectPaperCorners(capturedPage1);
    Mat correctedPage1 = applyPerspectiveTransform(capturedPage1, corners1, Size(1238, 1800));
    
    // KRİTİK DEBUG: Görüntü boyutunu kontrol et
    cout << "  [DEBUG] Corrected page 1 boyutu: " << correctedPage1.cols << "x" << correctedPage1.rows 
         << " (Hedef: 1238x1800)" << endl;
    
    // Eğer hala yanlış boyuttaysa kesin olarak resize et
    if (correctedPage1.cols != 1238 || correctedPage1.rows != 1800) {
        resize(correctedPage1, correctedPage1, Size(1238, 1800), 0, 0, INTER_LANCZOS4);
        cout << "  [DEBUG] Sayfa 1 boyutu düzeltildi: " << correctedPage1.cols << "x" << correctedPage1.rows << endl;
    }
    
    // Debug için kaydet
    imwrite("corrected_page1.jpg", correctedPage1);
    cout << "✅ Sayfa 1 düzeltildi (corrected_page1.jpg)" << endl;
    
    int totalScore = 0;
    
    cout << "\n🎓 EXAM GRADER - KAMERA VERSİYONU\n" << endl;
    cout << "📄 SAYFA 1 (El Yazısı + True/False)\n" << endl;
    cout << "📝 El Yazısı:\n" << endl;
    
    // EL YAZISI ROI'leri - GENİŞLİK ARTTIRILDI (sağ taraf kesiliyordu!)
    // Yazıların TAMAMI görünsün diye genişlik +100px
    vector<Rect> handwriting = {
        Rect(234, 328, 450, 77),   // Soru 1: ANKARA (genişlik: 332→450, +118px)
        Rect(240, 494, 450, 94),   // Soru 2: ATATÜRK (genişlik: 328→450, +122px)
        Rect(257, 641, 600, 106),  // Soru 3: ASYA AVRUPA (genişlik: 487→600, +113px)
        Rect(284, 796, 500, 112),  // Soru 4: ARJANTİN (genişlik: 380→500, +120px)
        Rect(294, 945, 500, 92)    // Soru 5: SAKARYA (genişlik: 374→500, +126px)
    };
    
    for (int i = 0; i < 5; i++) {
        // ROI sınır kontrolü
        Rect roi = handwriting[i];
        if (roi.x < 0 || roi.y < 0 || 
            roi.x + roi.width > correctedPage1.cols || 
            roi.y + roi.height > correctedPage1.rows) {
            cout << "  Soru " << (i+1) << ": ROI SINIRLARIN DIŞINDA! (ROI: " 
                 << roi.x << "," << roi.y << "," << roi.width << "," << roi.height 
                 << " | Görüntü: " << correctedPage1.cols << "x" << correctedPage1.rows << ")" << endl;
            continue;
        }
        
        // ROI sınır kontrolü
        if (roi.x < 0 || roi.y < 0 || 
            roi.x + roi.width > correctedPage1.cols || 
            roi.y + roi.height > correctedPage1.rows) {
            cout << "  [DEBUG] Soru " << (i+1) << " ROI SINIRLARIN DIŞINDA!" << endl;
            cout << "    ROI: (" << roi.x << ", " << roi.y << ", " << roi.width << ", " << roi.height << ")" << endl;
            cout << "    Görüntü: " << correctedPage1.cols << "x" << correctedPage1.rows << endl;
            continue;
        }
        
        Mat roiImage = correctedPage1(roi);
        
        // DEBUG: Ham ROI'yi kaydet (daha açıklayıcı isim)
        string debugRoiPath = "debug_q" + to_string(i+1) + "_roi.jpg";
        imwrite(debugRoiPath, roiImage);
        cout << "  [DEBUG] Soru " << (i+1) << " ROI kaydedildi: " << debugRoiPath 
             << " (" << roiImage.cols << "x" << roiImage.rows << ")" << endl;
        
        // DEBUG: ROI'nin ortalama parlaklığını kontrol et
        Mat grayCheck;
        cvtColor(roiImage, grayCheck, COLOR_BGR2GRAY);
        Scalar meanBrightness = mean(grayCheck);
        cout << "  [DEBUG] Soru " << (i+1) << " ROI ortalama parlaklık: " << meanBrightness[0] << endl;
        
        // DEBUG: final_grader gibi ham ROI'yi de kaydet (debug_q1.jpg gibi)
        string debugPath = "debug_q" + to_string(i+1) + ".jpg";
        imwrite(debugPath, roiImage);
        cout << "  [DEBUG] Soru " << (i+1) << " ham ROI kaydedildi: " << debugPath 
             << " (" << roiImage.cols << "x" << roiImage.rows << ")" << endl;
        
        // DEBUG: Preprocessing sonrası görüntüyü kaydet (OCR fonksiyonu ile AYNI - minimal preprocessing)
        Mat gray;
        cvtColor(roiImage, gray, COLOR_BGR2GRAY);
        Mat preprocessed;
        resize(gray, preprocessed, Size(), 4.0, 4.0, INTER_LINEAR); // OCR ile AYNI (4x, INTER_LINEAR - daha keskin)
        copyMakeBorder(preprocessed, preprocessed, 30, 30, 30, 30, 
                      BORDER_CONSTANT, Scalar(255)); // OCR ile AYNI (30px border)
        string preprocessedPath = "debug_q" + to_string(i+1) + "_preprocessed.jpg";
        imwrite(preprocessedPath, preprocessed);
        cout << "  [DEBUG] Soru " << (i+1) << " Preprocessed kaydedildi: " << preprocessedPath 
             << " (" << preprocessed.cols << "x" << preprocessed.rows << ", resize 4x + border, INTER_LINEAR)" << endl;
        
        string answer = ocrText(ocr, roiImage);
        
        // Debug: OCR sonucunu da yazdır
        cout << "  [DEBUG] Soru " << (i+1) << " OCR ham sonucu: \"" << answer << "\"" << endl;
        int score = compareAnswer(answer, answerKey[i+1]);
        totalScore += score;
        
        // GradingResult'a kaydet
        lastGradingResult.handwritingAnswers.push_back(answer);
        lastGradingResult.handwritingScores.push_back(score);
        lastGradingResult.totalScore += score;
        
        cout << "  Soru " << (i+1) << ": \"" << answer << "\"";
        cout << " (Doğru: \"" << answerKey[i+1] << "\")";
        cout << " → " << (score >= 10 ? "✅" : score > 0 ? "⚠️" : "❌");
        cout << " " << score << "p" << endl;
    }
    
    // TRUE/FALSE
    cout << "\n✓/✗ True/False:\n" << endl;
    
    // TRUE/FALSE checkbox koordinatları - final_grader ile aynı
    // final_grader: {Rect(145, 1223, 45, 45), Rect(315, 1223, 45, 45)} - Soru 1
    // final_grader: {Rect(140, 1326, 184, 103), Rect(424, 1319, 201, 104)} - Soru 2
    // TRUE sol, FALSE sağ
    vector<pair<Rect, Rect>> tfCheckboxes = {
        {Rect(145, 1223, 45, 45), Rect(315, 1223, 45, 45)},  // Soru 1: TRUE (sol), FALSE (sağ)
        {Rect(140, 1326, 184, 103), Rect(424, 1319, 201, 104)} // Soru 2: TRUE (sol), FALSE (sağ)
    };
    
    // final_grader ile AYNI mantık - basit ve çalışıyor!
    for (int i = 0; i < 2; i++) {
        // DEBUG: Fill ratio'ları göster (final_grader gibi)
        Mat trueROI = correctedPage1(tfCheckboxes[i].first);
        Mat falseROI = correctedPage1(tfCheckboxes[i].second);
        
        Mat grayT, grayF, threshT, threshF;
        cvtColor(trueROI, grayT, COLOR_BGR2GRAY);
        cvtColor(falseROI, grayF, COLOR_BGR2GRAY);
        threshold(grayT, threshT, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        threshold(grayF, threshF, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        
        double trueFill = (double)countNonZero(threshT) / (threshT.rows * threshT.cols);
        double falseFill = (double)countNonZero(threshF) / (threshF.rows * threshF.cols);
        
        cout << "  [DEBUG] Soru " << (i+1) << ": TRUE=" << (trueFill*100) << "% | FALSE=" << (falseFill*100) << "%" << endl;
        
        // DÜZELT İLMİŞ MANTIK: En yüksek fill ratio'yu seç (threshold: %25)
        // Eğer ikisi de %25'in altındaysa boş bırakılmış
        bool studentAnswer;
        if (trueFill < 0.25 && falseFill < 0.25) {
            // İkisi de boş
            studentAnswer = false;  // Default FALSE (boş)
            cout << "  ⚠️  Soru " << (i+1) << ": İkisi de boş (TRUE=" << (trueFill*100) 
                 << "%, FALSE=" << (falseFill*100) << "%)" << endl;
        } else {
            // En yüksek fill ratio'yu seç
            studentAnswer = (trueFill > falseFill);
            cout << "  [DEBUG] Soru " << (i+1) << " seçim: " << (studentAnswer ? "TRUE" : "FALSE")
                 << " (TRUE=" << (trueFill*100) << "% vs FALSE=" << (falseFill*100) << "%)" << endl;
        }
        
        bool correct = (studentAnswer == tfAnswers[i]);
        int score = correct ? 10 : 0;
        totalScore += score;
        
        // GradingResult'a kaydet
        lastGradingResult.tfAnswers.push_back(studentAnswer);
        lastGradingResult.tfScores.push_back(score);
        lastGradingResult.totalScore += score;
        
        cout << "  Soru " << (i+1) << ": " << (studentAnswer ? "TRUE" : "FALSE");
        cout << " → " << (correct ? "✅" : "❌") << " " << score << "p" << endl;
    }
    
    // Sayfa 2'yi perspective correction ile düzelt
    cout << "\n🔍 Sayfa 2'de kağıt tespiti yapılıyor..." << endl;
    vector<Point2f> corners2 = detectPaperCorners(capturedPage2);
    Mat correctedPage2 = applyPerspectiveTransform(capturedPage2, corners2, Size(1232, 1782));
    
    // KRİTİK DEBUG: Görüntü boyutunu kontrol et
    cout << "  [DEBUG] Corrected page 2 boyutu: " << correctedPage2.cols << "x" << correctedPage2.rows 
         << " (Hedef: 1232x1782)" << endl;
    
    // Eğer hala yanlış boyuttaysa kesin olarak resize et
    if (correctedPage2.cols != 1232 || correctedPage2.rows != 1782) {
        resize(correctedPage2, correctedPage2, Size(1232, 1782), 0, 0, INTER_LANCZOS4);
        cout << "  [DEBUG] Sayfa 2 boyutu düzeltildi: " << correctedPage2.cols << "x" << correctedPage2.rows << endl;
    }
    
    // Debug için kaydet
    imwrite("corrected_page2.jpg", correctedPage2);
    cout << "✅ Sayfa 2 düzeltildi (corrected_page2.jpg)" << endl;
    
    cout << "\n📄 SAYFA 2 (Çoktan Seçmeli)\n" << endl;
    cout << "🔘 Çoktan Seçmeli:\n" << endl;
    
    vector<vector<Rect>> mcOptions = {
        {
            Rect(90, 310, 80, 30),      // A
            Rect(90, 337, 80, 30),      // B
            Rect(85, 356, 90, 35),      // C - genişletildi (X: 90→85, W: 80→90)
            Rect(90, 387, 80, 30)       // D
        },
        {
            Rect(100, 503, 90, 25),     // A
            Rect(100, 530, 140, 25),    // B
            Rect(100, 557, 90, 25),     // C
            Rect(100, 581, 140, 35)     // D
        }
    };
    
    char options[] = {'A', 'B', 'C', 'D'};
    
    for (int q = 0; q < 2; q++) {
        char marked = '?';
        double maxFill = 0.0;
        vector<double> fillRatios(4, 0.0);
        
        cout << "  [DEBUG] Soru " << (q+1) << " şıkları:" << endl;
        
        for (int opt = 0; opt < 4; opt++) {
            Rect roi = mcOptions[q][opt];
            
            // ROI sınır kontrolü
            if (roi.x < 0 || roi.y < 0 || 
                roi.x + roi.width > correctedPage2.cols || 
                roi.y + roi.height > correctedPage2.rows) {
                cout << "    " << options[opt] << ": ROI SINIRLARIN DIŞINDA!" << endl;
                continue;
            }
            
            Mat r = correctedPage2(roi);
            
            // Debug: ROI'yi kaydet
            string debugPath = "debug_mc_q" + to_string(q+1) + "_" + string(1, options[opt]) + ".jpg";
            imwrite(debugPath, r);
            
            // DAIRE KONTURU TESPİTİ - Multiple choice için
            Mat gray;
            cvtColor(r, gray, COLOR_BGR2GRAY);
            
            // OTSU threshold
            Mat binary;
            threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
            
            // Konturları bul
            vector<vector<Point>> contours;
            findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
            
            // Daire/elips şeklinde kontur ara
            bool hasCircle = false;
            for (const auto& contour : contours) {
                if (contour.size() < 5) continue; // fitEllipse için en az 5 nokta gerekli
                
                double area = contourArea(contour);
                double peri = arcLength(contour, true);
                if (peri < 1) continue;
                
                // Circularity kontrolü (daireye ne kadar yakın?)
                double circularity = 4.0 * CV_PI * area / (peri * peri);
                
                // Daire şeklinde mi? (circularity > 0.7)
                if (circularity > 0.7 && area > r.rows * r.cols * 0.1) {
                    hasCircle = true;
                    break;
                }
            }
            
            // Fill ratio hesapla (fallback)
            Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
            morphologyEx(binary, binary, MORPH_CLOSE, kernel);
            int filled = countNonZero(binary);
            int total = binary.rows * binary.cols;
            double fillRatio = (double)filled / total;
            
            // Daire varsa fill ratio'yu artır (daire içi dolu olabilir)
            if (hasCircle) {
                fillRatio = max(fillRatio, 0.35); // Minimum %35 fill
            }
            
            fillRatios[opt] = fillRatio;
            
            cout << "    " << options[opt] << ": " << (fillRatio*100) << "%";
            if (hasCircle) cout << " (DAİRE BULUNDU)";
            cout << endl;
        }
        
        // EN YÜKSEK FILL RATIO'YU BUL
        int maxIndex = 0;
        for (int opt = 1; opt < 4; opt++) {
            if (fillRatios[opt] > fillRatios[maxIndex]) {
                maxIndex = opt;
            }
        }
        
        maxFill = fillRatios[maxIndex];
        
        // MANTIK: En yüksek fill ratio'ya sahip şıkkı seç
        // 1. Eğer diğer şıklardan %10'dan fazla fark varsa ve minimum %25 fill varsa → SEÇ
        // 2. Eğer fark küçükse ama en yüksek %30+ ise → SEÇ
        // 3. Eğer en yüksek %30'dan azsa → SEÇME
        
        bool hasSignificantDifference = true;
        for (int opt = 0; opt < 4; opt++) {
            if (opt != maxIndex && fillRatios[maxIndex] - fillRatios[opt] < 0.10) {
                hasSignificantDifference = false;
                break;
            }
        }
        
        if (hasSignificantDifference && maxFill > 0.25) {
            // En yüksek şık diğerlerinden %10+ fazla ve minimum %25 fill var
            marked = options[maxIndex];
            cout << "  [DEBUG] Soru " << (q+1) << " - " << marked << " seçildi (fill: " 
                 << (maxFill*100) << "%, diğerlerinden %10+ fazla)" << endl;
        } else if (maxFill > 0.30) {
            // Fark küçük ama en yüksek %30+ ise seç (gevşetildi: %25 → %30)
            marked = options[maxIndex];
            cout << "  [DEBUG] Soru " << (q+1) << " - " << marked << " seçildi (fill: " 
                 << (maxFill*100) << "%, threshold: %30, fark: " 
                 << (hasSignificantDifference ? "yeterli" : "yetersiz") << ")" << endl;
        } else {
            // En yüksek %30'dan azsa seçme
            marked = '?';
            cout << "  ⚠️  Hiçbir şık yeterince işaretli değil (max: " << (maxFill*100) 
                 << "%, threshold: %30)" << endl;
        }
        
        bool correct = (marked == mcAnswers[q]);
        int score = correct ? 10 : 0;
        totalScore += score;
        
        // GradingResult'a kaydet
        lastGradingResult.mcAnswers.push_back(marked);
        lastGradingResult.mcScores.push_back(score);
        lastGradingResult.totalScore += score;
        
        cout << "  Soru " << (q+1) << ": " << marked;
        cout << " (Doğru: " << mcAnswers[q] << ", Fill: " << (maxFill*100) << "%)";
        cout << " → " << (correct ? "✅" : "❌") << " " << score << "p" << endl;
    }
    
    cout << "\n==================================================" << endl;
    cout << "🎯 TOPLAM PUAN: " << totalScore << " / 100" << endl;
    cout << "==================================================" << endl;
}

int main() {
    VideoCapture camera(0);
    
    if (!camera.isOpened()) {
        cerr << "❌ Kamera açılamadı!" << endl;
        return -1;
    }
    
    // Kamera çözünürlüğünü artır (daha iyi kalite için)
    camera.set(CAP_PROP_FRAME_WIDTH, 1920);
    camera.set(CAP_PROP_FRAME_HEIGHT, 1080);
    camera.set(CAP_PROP_AUTO_EXPOSURE, 0.25); // Manuel exposure kontrolü
    camera.set(CAP_PROP_AUTOFOCUS, 1); // Otomatik odak
    
    tesseract::TessBaseAPI* ocr = new tesseract::TessBaseAPI();
    // final_grader ile AYNI dil ayarı (çalışıyor!)
    if (ocr->Init(NULL, "tur", tesseract::OEM_LSTM_ONLY)) {
        cerr << "❌ OCR hatası!" << endl;
        return -1;
    }
    // PSM modu ocrText fonksiyonunda ayarlanacak (PSM_RAW_LINE - final_grader gibi)
    
    cout << "\n📹 KAMERA SINAV DEĞERLENDİRME SİSTEMİ - OTOMATIK MOD\n" << endl;
    cout << "🎯 TAM OTOMATIK MOD: Kağıdı kameraya gösterin, otomatik yakalanacak!\n" << endl;
    cout << "Kontroller:" << endl;
    cout << "  Otomatik: Kağıdı gösterin, sistem otomatik yakalar" << endl;
    cout << "  m - Manuel moda geç (1/2 tuşları ile yakala)" << endl;
    cout << "  a - Otomatik moda geri dön" << endl;
    cout << "  ESC - Çıkış\n" << endl;
    
    namedWindow("Camera Exam Grader", WINDOW_NORMAL);
    setMouseCallback("Camera Exam Grader", mouseCallback);
    
    Mat frame;
    
    while (true) {
        camera >> frame;
        
        if (frame.empty()) break;
        
        Mat display = frame.clone();
        
        // COOLDOWN PERIOD - Kağıt yakalandıktan sonra bir süre algılama yapma
        if (cooldownFrames > 0) {
            cooldownFrames--;
            string cooldownMsg = "Kagit yakalandi! Hazirlaniyor... (" + to_string(cooldownFrames) + " frame)";
            putText(display, cooldownMsg, Point(10, 30), 
                   FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255), 2);
        }
        
        // OTOMATIK YAKALAMA MODU - Cooldown sırasında algılama yapma
        if (autoCaptureMode && !processingPage && !selectingCorners && cooldownFrames == 0) {
            try {
                // DEBUG: Tüm konturları çiz
                Mat gray;
                cvtColor(frame, gray, COLOR_BGR2GRAY);
                Mat hsv;
                cvtColor(frame, hsv, COLOR_BGR2HSV);
                Mat whiteMask;
                inRange(hsv, Scalar(0, 0, 200), Scalar(180, 50, 255), whiteMask);
                Mat blurred;
                GaussianBlur(gray, blurred, Size(9, 9), 0);
                Mat edges;
                Canny(blurred, edges, 30, 100);
                Mat kernel = getStructuringElement(MORPH_RECT, Size(7, 7));
                Mat combined;
                bitwise_and(edges, whiteMask, combined);
                dilate(combined, combined, kernel, Point(-1, -1), 3);
                vector<vector<Point>> debugContours;
                findContours(combined, debugContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                
                // Tüm konturları mavi ile çiz
                for (size_t i = 0; i < debugContours.size(); i++) {
                    double area = contourArea(debugContours[i]);
                    if (area > frame.rows * frame.cols * 0.05) {
                        drawContours(display, debugContours, i, Scalar(255, 0, 0), 2);
                    }
                }
                
                // Debug bilgilerini ekrana yaz
                string debugText = "Konturlar: " + to_string(debugContours.size());
                putText(display, debugText, Point(10, display.rows - 60), 
                       FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
                
                vector<Point2f> detectedCorners = detectPaperCorners(frame);
                
                // KAĞIT TESPİT EDİLDİYSE DEVAM ET
                if (detectedCorners.size() == 4) {
                    // STABİLİTE KONTROLÜ - Son 20 frame'deki köşeleri karşılaştır
                    recentCorners.push_back(detectedCorners);
                    if (recentCorners.size() > REQUIRED_STABLE_FRAMES) {
                        recentCorners.erase(recentCorners.begin());
                    }
                    
                    // Köşelerin sabit kalıp kalmadığını kontrol et
                    bool cornersStable = false;
                    if (recentCorners.size() >= REQUIRED_STABLE_FRAMES) {
                        // Son N frame'in köşeleri birbirine yakın mı?
                        cornersStable = true;
                        double maxDeviation = frame.cols * 0.05; // %5 tolerans (daha sıkı)
                        
                        for (int i = 0; i < 4; i++) {
                            // Bu köşenin son 20 frame'deki pozisyonlarını kontrol et
                            vector<double> xPositions, yPositions;
                            for (const auto& corners : recentCorners) {
                                xPositions.push_back(corners[i].x);
                                yPositions.push_back(corners[i].y);
                            }
                            
                            // Ortalama pozisyon
                            double avgX = 0, avgY = 0;
                            for (double x : xPositions) avgX += x;
                            for (double y : yPositions) avgY += y;
                            avgX /= xPositions.size();
                            avgY /= yPositions.size();
                            
                            // Standart sapma kontrolü
                            double varianceX = 0, varianceY = 0;
                            for (double x : xPositions) varianceX += (x - avgX) * (x - avgX);
                            for (double y : yPositions) varianceY += (y - avgY) * (y - avgY);
                            double stdDevX = sqrt(varianceX / xPositions.size());
                            double stdDevY = sqrt(varianceY / yPositions.size());
                            
                            // Eğer standart sapma çok büyükse, stabil değil
                            if (stdDevX > maxDeviation || stdDevY > maxDeviation) {
                                cornersStable = false;
                                break;
                            }
                        }
                    }
                    
                    if (cornersStable) {
                        stableFrameCount++;
                        // Ortalama köşeleri kullan (daha stabil)
                        vector<Point2f> avgCorners(4);
                        for (int i = 0; i < 4; i++) {
                            double avgX = 0, avgY = 0;
                            for (const auto& corners : recentCorners) {
                                avgX += corners[i].x;
                                avgY += corners[i].y;
                            }
                            avgCorners[i] = Point2f(avgX / recentCorners.size(), avgY / recentCorners.size());
                        }
                        detectedCorners = avgCorners;
                    } else {
                        stableFrameCount = 0;
                    }
                    
                    // Kağıt köşelerini yeşil çizgi ile göster
                    Scalar color = (stableFrameCount >= REQUIRED_STABLE_FRAMES) ? 
                                   Scalar(0, 255, 255) : Scalar(0, 255, 0); // Sarı = hazır, yeşil = tespit
                    
                    for (int i = 0; i < 4; i++) {
                        Point p1(static_cast<int>(detectedCorners[i].x), static_cast<int>(detectedCorners[i].y));
                        Point p2(static_cast<int>(detectedCorners[(i+1)%4].x), static_cast<int>(detectedCorners[(i+1)%4].y));
                        line(display, p1, p2, color, 3);
                        circle(display, p1, 8, color, -1);
                    }
                    
                    // İlerleme çubuğu göster
                    int barWidth = 200;
                    int barHeight = 20;
                    int barX = display.cols - barWidth - 20;
                    int barY = 30;
                    
                    double progress = min(1.0, (double)stableFrameCount / REQUIRED_STABLE_FRAMES);
                    rectangle(display, Point(barX, barY), Point(barX + barWidth, barY + barHeight), 
                             Scalar(100, 100, 100), -1);
                    rectangle(display, Point(barX, barY), 
                             Point(barX + (int)(barWidth * progress), barY + barHeight), 
                             Scalar(0, 255, 0), -1);
                    
                    string statusMsg;
                    if (stableFrameCount >= REQUIRED_STABLE_FRAMES) {
                        statusMsg = "Kagit hazir! Yakalaniyor...";
                        putText(display, statusMsg, Point(10, 30), 
                               FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 255), 2);
                        
                        // OTOMATIK YAKALAMA!
                        if (!page1Ready) {
                            processingPage = true;
                            cout << "\n🎯 SAYFA 1 OTOMATIK YAKALANDI!" << endl;
                            capturedPage1 = applyPerspectiveTransform(frame, detectedCorners, Size(1238, 1800));
                            imwrite("captured_page1.jpg", capturedPage1);
                            page1Ready = true;
                            stableFrameCount = 0;
                            lastDetectedCorners.clear();
                            recentCorners.clear();
                            cooldownFrames = COOLDOWN_FRAMES;  // Cooldown başlat
                            processingPage = false;
                            cout << "✅ Sayfa 1 kaydedildi. Şimdi Sayfa 2'yi gösterin..." << endl;
                            cout << "⏳ Lütfen bekleyin, sistem hazırlanıyor..." << endl;
                        } else if (!page2Ready) {
                            processingPage = true;
                            cout << "\n🎯 SAYFA 2 OTOMATIK YAKALANDI!" << endl;
                            capturedPage2 = applyPerspectiveTransform(frame, detectedCorners, Size(1232, 1782));
                            imwrite("captured_page2.jpg", capturedPage2);
                            page2Ready = true;
                            stableFrameCount = 0;
                            lastDetectedCorners.clear();
                            recentCorners.clear();
                            cooldownFrames = COOLDOWN_FRAMES;  // Cooldown başlat
                            processingPage = false;
                            cout << "✅ Sayfa 2 kaydedildi. Otomatik değerlendirme başlıyor..." << endl;
                            
                            // Perspektif düzeltilmiş sayfaları kaydet
                            imwrite("corrected_page1.jpg", capturedPage1);
                            imwrite("corrected_page2.jpg", capturedPage2);
                            
                            // PROCESS RESULTS - Terminal çıktısı + GradingResult dolduruluyor
                            waitKey(500);
                            processResults(ocr);
                            
                            // OVERLAY'İ GÖSTER - 10 saniye bekle
                            if (lastGradingResult.isValid) {
                                cout << "\n✅ Değerlendirme tamamlandı! Overlay ekranda gösteriliyor..." << endl;
                                cout << "📊 CANLI SONUÇLAR: " << lastGradingResult.totalScore << "/100" << endl;
                                
                                Mat resultDisplay = drawGradingOverlay(frame.clone(), lastGradingResult);
                                imshow("Camera Exam Grader", resultDisplay);
                                cout << "⏳ Overlay 10 saniye gösteriliyor... (ESC ile atla)" << endl;
                                
                                int key = waitKey(10000);  // 10 saniye overlay göster (ESC ile skip)
                                if (key == 27) {
                                    cout << "⏭️  Overlay atlandı" << endl;
                                }
                            }
                            page1Ready = false;
                            page2Ready = false;
                            
                            // Stabiliteyi sıfırla
                            stableFrameCount = 0;
                            recentCorners.clear();
                            lastDetectedCorners.clear();
                            cooldownFrames = COOLDOWN_FRAMES;  // Yeni sınav için cooldown
                        }
                    } else {
                        statusMsg = "Kagit tespit edildi - Sabit tutun... (" + 
                                   to_string(stableFrameCount) + "/" + 
                                   to_string(REQUIRED_STABLE_FRAMES) + ")";
                        putText(display, statusMsg, Point(10, 30), 
                               FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
                    }
                } else {
                    // Kağıt tespit edilemedi - stabiliteyi sıfırla
                    stableFrameCount = 0;
                    recentCorners.clear();
                    lastDetectedCorners.clear();
                    if (!page1Ready) {
                        putText(display, "Sayfa 1 icin BEYAZ KAGIDI gosterin...", 
                               Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 165, 255), 2);
                    } else if (!page2Ready) {
                        putText(display, "Sayfa 2 icin BEYAZ KAGIDI gosterin...", 
                               Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 165, 255), 2);
                    }
                }
            } catch (...) {
                stableFrameCount = 0;
                recentCorners.clear();
                lastDetectedCorners.clear();
            }
        }
        
        // Sayfa 1 yakalandıysa ve cooldown bittiyse, sadece sayfa 2 için algılama yap
        if (page1Ready && !page2Ready && cooldownFrames == 0 && autoCaptureMode) {
            // Sayfa 2 için hazır mesajı göster
            string readyMsg = "Sayfa 1 hazir! Sayfa 2'yi gosterin...";
            putText(display, readyMsg, Point(10, 30), 
                   FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        }
        
        // Manuel mod overlay'i
        if (!autoCaptureMode && !selectingCorners) {
            try {
                vector<Point2f> detectedCorners = detectPaperCorners(frame);
                // KAĞIT TESPİT EDİLDİYSE GÖSTER
                if (detectedCorners.size() == 4) {
                    // Kağıt köşelerini yeşil çizgi ile göster
                    for (int i = 0; i < 4; i++) {
                        Point p1(static_cast<int>(detectedCorners[i].x), static_cast<int>(detectedCorners[i].y));
                        Point p2(static_cast<int>(detectedCorners[(i+1)%4].x), static_cast<int>(detectedCorners[(i+1)%4].y));
                        line(display, p1, p2, Scalar(0, 255, 0), 2);
                        circle(display, p1, 5, Scalar(0, 255, 0), -1);
                    }
                    putText(display, "Kagit tespit edildi - 1 veya 2'ye basin", 
                           Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
                } else {
                    putText(display, "BEYAZ KAGIT gosterin - C'ye basin (manuel)", 
                           Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 165, 255), 2);
                }
            } catch (...) {
                // Tespit başarısız olursa sessizce devam et
            }
        }
        
        // Manuel köşe seçimi modunda köşeleri göster
        if (selectingCorners) {
            for (size_t i = 0; i < manualCorners.size(); i++) {
                Point corner(static_cast<int>(manualCorners[i].x), static_cast<int>(manualCorners[i].y));
                circle(display, corner, 10, Scalar(0, 0, 255), -1);
                putText(display, to_string(i+1), Point(corner.x + 15, corner.y + 15),
                       FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0, 0, 255), 3);
                
                if (i > 0) {
                    Point prevCorner(static_cast<int>(manualCorners[i-1].x), static_cast<int>(manualCorners[i-1].y));
                    line(display, prevCorner, corner, Scalar(0, 255, 0), 3);
                }
            }
            
            // 4 köşe seçildiyse kapatma çizgisi
            if (manualCorners.size() == 4) {
                Point first(static_cast<int>(manualCorners[0].x), static_cast<int>(manualCorners[0].y));
                Point last(static_cast<int>(manualCorners[3].x), static_cast<int>(manualCorners[3].y));
                line(display, first, last, Scalar(0, 255, 0), 3);
            }
            
            string msg = "Kose " + to_string(manualCorners.size() + 1) + "/4 secin (Sol-ust, Sag-ust, Sag-alt, Sol-alt)";
            putText(display, msg, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 255), 2);
            
            if (manualCorners.size() == 4) {
                putText(display, "ENTER'a basin veya 1/2 ile yakalayin", 
                       Point(10, 100), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
            }
        }
        
        // Durum göstergesi
        string status = "";
        if (page1Ready) status += "[✓ Sayfa 1] ";
        else status += "[✗ Sayfa 1] ";
        
        if (page2Ready) status += "[✓ Sayfa 2]";
        else status += "[✗ Sayfa 2]";
        
        int statusY = autoCaptureMode ? 60 : 30;
        putText(display, status, Point(10, statusY), 
               FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        
        // Mod göstergesi
        string modeText = autoCaptureMode ? "[OTOMATIK MOD] - m: Manuel" : "[MANUEL MOD] - a: Otomatik";
        putText(display, modeText, Point(10, display.rows - 50), 
               FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 0), 2);
        
        putText(display, "1:Sayfa1 | 2:Sayfa2 | C:Manuel | R:Degerlendir | ESC:Cikis", 
               Point(10, display.rows - 20), 
               FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
        
        // REAL-TIME GRADING OVERLAY - Eğer sonuçlar varsa göster
        if (lastGradingResult.isValid) {
            display = drawGradingOverlay(display, lastGradingResult);
        }
        
        imshow("Camera Exam Grader", display);
        
        int key = waitKey(1);
        
        if (key == 27) { // ESC
            break;
        }
        else if (key == 'm' || key == 'M') {
            autoCaptureMode = false;
            stableFrameCount = 0;
            recentCorners.clear();
            lastDetectedCorners.clear();
            cout << "📌 Manuel moda geçildi" << endl;
        }
        else if (key == 'a' || key == 'A') {
            autoCaptureMode = true;
            stableFrameCount = 0;
            recentCorners.clear();
            lastDetectedCorners.clear();
            cout << "🤖 Otomatik moda geçildi" << endl;
        }
        else if (key == 'c' || key == 'C') {
            selectingCorners = true;
            manualCorners.clear();
            tempFrame = frame.clone();
            cout << "\n📍 Manuel köşe seçimi: 4 köşeyi sırayla tıklayın (sol-üst, sağ-üst, sağ-alt, sol-alt)" << endl;
        }
        else if (key == 13 && selectingCorners && manualCorners.size() == 4) { // ENTER
            selectingCorners = false;
            cout << "✅ Köşeler seçildi!" << endl;
        }
        else if (key == '1') {
            if (selectingCorners && manualCorners.size() == 4) {
                // Manuel köşelerle perspective transform uygula
                capturedPage1 = applyPerspectiveTransform(tempFrame, manualCorners, Size(1238, 1800));
                imwrite("captured_page1.jpg", capturedPage1);
                selectingCorners = false;
                manualCorners.clear();
                cout << "✅ Sayfa 1 yakalandı (Manuel köşelerle)!" << endl;
            } else {
                // Otomatik köşe tespiti ile
                vector<Point2f> corners = detectPaperCorners(frame);
                if (corners.size() == 4) {
                    capturedPage1 = applyPerspectiveTransform(frame, corners, Size(1238, 1800));
                    cout << "✅ Sayfa 1 yakalandı (Otomatik tespit)!" << endl;
                } else {
                    // Köşe tespit edilemezse ham görüntüyü kullan
                capturedPage1 = frame.clone();
                    cout << "⚠️  Sayfa 1 yakalandı (Köşe tespiti başarısız, ham görüntü kullanılıyor)" << endl;
                }
                imwrite("captured_page1.jpg", capturedPage1);
            }
            page1Ready = true;
        }
        else if (key == '2') {
            if (selectingCorners && manualCorners.size() == 4) {
                // Manuel köşelerle perspective transform uygula
                capturedPage2 = applyPerspectiveTransform(tempFrame, manualCorners, Size(1232, 1782));
                imwrite("captured_page2.jpg", capturedPage2);
                selectingCorners = false;
                manualCorners.clear();
                cout << "✅ Sayfa 2 yakalandı (Manuel köşelerle)!" << endl;
            } else {
                // Otomatik köşe tespiti ile
                vector<Point2f> corners = detectPaperCorners(frame);
                if (corners.size() == 4) {
                    capturedPage2 = applyPerspectiveTransform(frame, corners, Size(1232, 1782));
                    cout << "✅ Sayfa 2 yakalandı (Otomatik tespit)!" << endl;
                } else {
                    // Köşe tespit edilemezse ham görüntüyü kullan
                capturedPage2 = frame.clone();
                    cout << "⚠️  Sayfa 2 yakalandı (Köşe tespiti başarısız, ham görüntü kullanılıyor)" << endl;
                }
                imwrite("captured_page2.jpg", capturedPage2);
            }
            page2Ready = true;
        }
        else if (key == 'r' || key == 'R') {
            processResults(ocr);
        }
    }
    
    camera.release();
    ocr->End();
    delete ocr;
    destroyAllWindows();
    
    return 0;
}
