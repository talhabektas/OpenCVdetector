#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

// Global değişkenler
vector<Point2f> corners;
Mat sourceImage;
string windowName = "Kose Sec";

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN && corners.size() < 4) {
        corners.push_back(Point2f(x, y));
        cout << "📍 Köşe " << corners.size() << ": (" << x << ", " << y << ")" << endl;
        
        // Görsel feedback
        Mat temp = sourceImage.clone();
        for (size_t i = 0; i < corners.size(); i++) {
            circle(temp, Point(corners[i].x, corners[i].y), 10, Scalar(0, 0, 255), -1);
            putText(temp, to_string(i+1), 
                   Point(corners[i].x + 15, corners[i].y + 15),
                   FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 0, 255), 3);
            
            if (i > 0) {
                line(temp, corners[i-1], corners[i], Scalar(0, 255, 0), 3);
            }
        }
        
        // Talimat göster
        vector<string> instructions = {
            "1. Sol-ust koseyi sec",
            "2. Sag-ust koseyi sec", 
            "3. Sag-alt koseyi sec",
            "4. Sol-alt koseyi sec"
        };
        
        string currentInstruction = (corners.size() < 4) ? 
            instructions[corners.size()] : "ENTER'a basin";
        
        putText(temp, currentInstruction, 
               Point(10, 50), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 255), 2);
        
        imshow(windowName, temp);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Kullanım: " << argv[0] << " <image_path>" << endl;
        return -1;
    }
    
    sourceImage = imread(argv[1]);
    if (sourceImage.empty()) {
        cerr << "Görüntü açılamadı!" << endl;
        return -1;
    }
    
    // Görüntüyü küçült (ekrana sığması için)
    double scale = 0.5;
    resize(sourceImage, sourceImage, Size(), scale, scale);
    
    cout << "\n🎯 MANUEL PERSPECTIVE TRANSFORM TEST\n" << endl;
    cout << "Talimatlar:" << endl;
    cout << "  1. Sol-üst köşeyi tıklayın" << endl;
    cout << "  2. Sağ-üst köşeyi tıklayın" << endl;
    cout << "  3. Sağ-alt köşeyi tıklayın" << endl;
    cout << "  4. Sol-alt köşeyi tıklayın" << endl;
    cout << "  5. ENTER'a basın" << endl;
    cout << "  ESC - İptal\n" << endl;
    
    namedWindow(windowName);
    setMouseCallback(windowName, mouseCallback);
    
    Mat display = sourceImage.clone();
    putText(display, "1. Sol-ust koseyi sec", 
           Point(10, 50), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 255), 2);
    imshow(windowName, display);
    
    while (true) {
        int key = waitKey(1);
        
        if (key == 27) { // ESC
            cout << "İptal edildi." << endl;
            return 0;
        }
        else if (key == 13 && corners.size() == 4) { // ENTER
            break;
        }
    }
    
    cout << "\n✅ 4 köşe seçildi!" << endl;
    
    // Hedef boyut (template boyutları)
    int targetWidth = 1238;
    int targetHeight = 1800;
    
    if (string(argv[1]).find("page2") != string::npos) {
        targetWidth = 1232;
        targetHeight = 1782;
    }
    
    // Hedef köşeler (düz dikdörtgen)
    vector<Point2f> dstCorners = {
        Point2f(0, 0),
        Point2f(targetWidth - 1, 0),
        Point2f(targetWidth - 1, targetHeight - 1),
        Point2f(0, targetHeight - 1)
    };
    
    // Perspective transform matrisini hesapla
    Mat transformMatrix = getPerspectiveTransform(corners, dstCorners);
    
    // Transform uygula
    Mat corrected;
    warpPerspective(sourceImage, corrected, transformMatrix, Size(targetWidth, targetHeight));
    
    // Sonucu kaydet
    string outputPath = "manual_corrected.jpg";
    imwrite(outputPath, corrected);
    
    cout << "✅ Düzeltilmiş görüntü kaydedildi: " << outputPath << endl;
    cout << "📐 Boyut: " << corrected.cols << "x" << corrected.rows << endl;
    
    // Göster
    namedWindow("Orijinal (kucultulmus)", WINDOW_NORMAL);
    imshow("Orijinal (kucultulmus)", sourceImage);
    
    namedWindow("Duzeltilmis", WINDOW_NORMAL);
    imshow("Duzeltilmis", corrected);
    
    cout << "\nHerhangi bir tuşa basın..." << endl;
    waitKey(0);
    
    return 0;
}
