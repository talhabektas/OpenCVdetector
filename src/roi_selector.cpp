/**
 * ROI SELECTOR - 4 NOKTA İLE DİKDÖRTGEN SEÇİMİ
 * 
 * Kullanım: ./roi_selector <image>
 * Her soru için 4 köşe işaretle (sol-üst, sağ-üst, sağ-alt, sol-alt)
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

struct ROIData {
    vector<Point> points;
    string label;
};

vector<ROIData> roiList;
ROIData currentROI;
Mat displayImage;
Mat originalImage;

void drawROIs() {
    displayImage = originalImage.clone();
    
    // Grid çiz
    for (int x = 0; x < displayImage.cols; x += 100) {
        line(displayImage, Point(x, 0), Point(x, displayImage.rows), Scalar(0, 255, 0), 1);
        putText(displayImage, to_string(x), Point(x + 5, 30), 
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
    }
    for (int y = 0; y < displayImage.rows; y += 100) {
        line(displayImage, Point(0, y), Point(displayImage.cols, y), Scalar(0, 255, 0), 1);
        putText(displayImage, to_string(y), Point(5, y + 20), 
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
    }
    
    // Tamamlanmış ROI'leri çiz
    for (const auto& roi : roiList) {
        if (roi.points.size() == 4) {
            vector<Point> pts = roi.points;
            line(displayImage, pts[0], pts[1], Scalar(0, 255, 255), 2);
            line(displayImage, pts[1], pts[2], Scalar(0, 255, 255), 2);
            line(displayImage, pts[2], pts[3], Scalar(0, 255, 255), 2);
            line(displayImage, pts[3], pts[0], Scalar(0, 255, 255), 2);
            
            // Label yaz
            putText(displayImage, roi.label, pts[0] + Point(5, -5),
                   FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255), 2);
        }
    }
    
    // Şu anki ROI'deki noktaları çiz
    for (size_t i = 0; i < currentROI.points.size(); i++) {
        circle(displayImage, currentROI.points[i], 5, Scalar(0, 0, 255), -1);
        putText(displayImage, to_string(i+1), currentROI.points[i] + Point(10, 10),
               FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 0, 255), 2);
        
        if (i > 0) {
            line(displayImage, currentROI.points[i-1], currentROI.points[i], 
                Scalar(255, 0, 0), 2);
        }
    }
    
    // Bilgi metni
    string info = "Soru " + to_string(roiList.size() + 1) + " - ";
    info += "Nokta " + to_string(currentROI.points.size() + 1) + "/4";
    if (currentROI.points.size() == 4) {
        info = "4 nokta tamam! ENTER'a bas veya yeni soru icin tikla";
    }
    
    rectangle(displayImage, Point(0, 0), Point(600, 40), Scalar(0, 0, 0), -1);
    putText(displayImage, info, Point(10, 25),
           FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
    
    imshow("ROI Selector", displayImage);
}

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        if (currentROI.points.size() < 4) {
            currentROI.points.push_back(Point(x, y));
            cout << "📍 Nokta " << currentROI.points.size() << ": X=" << x << ", Y=" << y << endl;
            
            if (currentROI.points.size() == 4) {
                currentROI.label = "Soru " + to_string(roiList.size() + 1);
                roiList.push_back(currentROI);
                
                // Rect hesapla ve yazdır
                int minX = min({currentROI.points[0].x, currentROI.points[1].x, 
                               currentROI.points[2].x, currentROI.points[3].x});
                int maxX = max({currentROI.points[0].x, currentROI.points[1].x, 
                               currentROI.points[2].x, currentROI.points[3].x});
                int minY = min({currentROI.points[0].y, currentROI.points[1].y, 
                               currentROI.points[2].y, currentROI.points[3].y});
                int maxY = max({currentROI.points[0].y, currentROI.points[1].y, 
                               currentROI.points[2].y, currentROI.points[3].y});
                
                cout << "\n✅ Soru " << roiList.size() << " ROI:" << endl;
                cout << "   Rect(" << minX << ", " << minY << ", " 
                     << (maxX - minX) << ", " << (maxY - minY) << ")" << endl;
                cout << endl;
                
                // Yeni ROI başlat
                currentROI.points.clear();
            }
            
            drawROIs();
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Kullanim: " << argv[0] << " <image>" << endl;
        return -1;
    }
    
    originalImage = imread(argv[1]);
    if (originalImage.empty()) {
        cerr << "Goruntu okunamadi!" << endl;
        return -1;
    }
    
    cout << "\n📐 ROI SELECTOR - 4 NOKTA İLE DİKDÖRTGEN SEÇİMİ\n" << endl;
    cout << "Her soru için 4 köşe noktasını işaretleyin:" << endl;
    cout << "  1. Sol-üst köşe" << endl;
    cout << "  2. Sağ-üst köşe" << endl;
    cout << "  3. Sağ-alt köşe" << endl;
    cout << "  4. Sol-alt köşe" << endl;
    cout << "\n5 soru için toplamda 20 nokta işaretleyeceksiniz." << endl;
    cout << "ESC ile çıkın.\n" << endl;
    
    namedWindow("ROI Selector", WINDOW_NORMAL);
    resizeWindow("ROI Selector", 1200, 900);
    setMouseCallback("ROI Selector", mouseCallback);
    
    drawROIs();
    
    while (true) {
        int key = waitKey(1);
        if (key == 27) break; // ESC
    }
    
    // Sonuçları yazdır
    cout << "\n\n========================================" << endl;
    cout << "TÜM ROI KOORDİNATLARI (C++ KODU):" << endl;
    cout << "========================================\n" << endl;
    cout << "vector<Rect> handwriting = {" << endl;
    
    for (size_t i = 0; i < roiList.size(); i++) {
        const auto& roi = roiList[i];
        if (roi.points.size() == 4) {
            int minX = min({roi.points[0].x, roi.points[1].x, 
                           roi.points[2].x, roi.points[3].x});
            int maxX = max({roi.points[0].x, roi.points[1].x, 
                           roi.points[2].x, roi.points[3].x});
            int minY = min({roi.points[0].y, roi.points[1].y, 
                           roi.points[2].y, roi.points[3].y});
            int maxY = max({roi.points[0].y, roi.points[1].y, 
                           roi.points[2].y, roi.points[3].y});
            
            cout << "    Rect(" << minX << ", " << minY << ", " 
                 << (maxX - minX) << ", " << (maxY - minY) << ")";
            if (i < roiList.size() - 1) cout << ",";
            cout << "   // Soru " << (i+1) << endl;
        }
    }
    
    cout << "};" << endl;
    
    return 0;
}