#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    // Orijinal ve kameradan düzeltilmiş görüntüleri oku
    Mat original = imread("page1.png");
    Mat camera = imread("corrected_page1.jpg");
    
    if (original.empty() || camera.empty()) {
        cerr << "Görüntüler yüklenemedi!" << endl;
        return -1;
    }
    
    cout << "\n📊 ROI KARŞILAŞTIRMASI\n" << endl;
    cout << "Orijinal: " << original.cols << "x" << original.rows << endl;
    cout << "Kamera: " << camera.cols << "x" << camera.rows << endl;
    
    // El yazısı ROI'leri
    vector<Rect> rois = {
        Rect(234, 328, 312, 77),   // Soru 1
        Rect(240, 494, 308, 94),   // Soru 2
        Rect(257, 641, 467, 106)   // Soru 3
    };
    
    for (size_t i = 0; i < rois.size(); i++) {
        Rect roi = rois[i];
        
        // Orijinalden ROI
        Mat origROI = original(roi).clone();
        
        // Kameradan ROI
        Mat camROI = camera(roi).clone();
        
        // Yan yana koy
        Mat comparison;
        hconcat(origROI, camROI, comparison);
        
        // Etiketler
        putText(comparison, "ORIJINAL", Point(10, 30),
               FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
        
        putText(comparison, "KAMERA", Point(origROI.cols + 10, 30),
               FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255), 2);
        
        // Kaydet
        string filename = "roi_comparison_q" + to_string(i+1) + ".jpg";
        imwrite(filename, comparison);
        
        cout << "✅ Soru " << (i+1) << " karşılaştırması: " << filename << endl;
    }
    
    cout << "\n🔍 True/False ROI karşılaştırması:\n" << endl;
    
    // True/False Soru 1
    Rect tf1_false(315, 1223, 45, 45);
    Mat origTF1 = original(tf1_false).clone();
    Mat camTF1 = camera(tf1_false).clone();
    
    Mat tfComp;
    hconcat(origTF1, camTF1, tfComp);
    
    // 4x büyüt
    resize(tfComp, tfComp, Size(), 4.0, 4.0, INTER_NEAREST);
    
    putText(tfComp, "ORIG", Point(10, 30),
           FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
    putText(tfComp, "CAM", Point(origTF1.cols*4 + 10, 30),
           FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255), 2);
    
    imwrite("tf_comparison_q1_false.jpg", tfComp);
    cout << "✅ T/F Soru 1 (FALSE kutusu): tf_comparison_q1_false.jpg" << endl;
    
    return 0;
}