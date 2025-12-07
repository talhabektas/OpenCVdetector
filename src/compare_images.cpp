#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    // Orijinal page1 (90/100 kazandıran)
    Mat original = imread("page1.png");
    
    // Manuel düzeltilmiş (kameradan)
    Mat corrected = imread("manual_corrected.jpg");
    
    if (original.empty()) {
        cerr << "page1.png bulunamadı!" << endl;
        return -1;
    }
    
    if (corrected.empty()) {
        cerr << "manual_corrected.jpg bulunamadı!" << endl;
        return -1;
    }
    
    cout << "\n📊 GÖRÜNTÜ KARŞILAŞTIRMASI\n" << endl;
    cout << "Orijinal page1.png: " << original.cols << "x" << original.rows << endl;
    cout << "Manuel düzeltilmiş: " << corrected.cols << "x" << corrected.rows << endl;
    
    // Aynı boyuta getir
    Mat correctedResized;
    resize(corrected, correctedResized, original.size());
    
    // Yan yana koy
    Mat comparison;
    hconcat(original, correctedResized, comparison);
    
    // ROI bölgelerini çiz (ilk 3 soru)
    vector<Rect> rois = {
        Rect(234, 328, 312, 77),   // Soru 1
        Rect(240, 494, 308, 94),   // Soru 2
        Rect(257, 641, 467, 106)   // Soru 3
    };
    
    // Orijinalde yeşil, düzeltilmişte kırmızı çerçeve
    for (const Rect& roi : rois) {
        rectangle(comparison, roi, Scalar(0, 255, 0), 2);  // Sol taraf (orijinal)
        
        Rect roiRight = roi;
        roiRight.x += original.cols;  // Sağ tarafa kaydır
        rectangle(comparison, roiRight, Scalar(0, 0, 255), 2);  // Sağ taraf (düzeltilmiş)
    }
    
    // Etiketler
    putText(comparison, "ORIJINAL (90/100)", Point(20, 50),
           FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 255, 0), 3);
    
    putText(comparison, "MANUEL DUZELTILMIS (0/80)", 
           Point(original.cols + 20, 50),
           FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0, 0, 255), 3);
    
    // Küçült (ekrana sığması için)
    Mat comparisonSmall;
    resize(comparison, comparisonSmall, Size(), 0.4, 0.4);
    
    // Kaydet
    imwrite("comparison.jpg", comparison);
    imwrite("comparison_small.jpg", comparisonSmall);
    
    cout << "\n✅ Karşılaştırma kaydedildi: comparison.jpg" << endl;
    cout << "✅ Küçük versiyon: comparison_small.jpg" << endl;
    
    // Göster
    namedWindow("Karsilastirma", WINDOW_NORMAL);
    imshow("Karsilastirma", comparisonSmall);
    
    cout << "\nYeşil çerçeveler = Orijinal ROI'ler (çalışıyor)" << endl;
    cout << "Kırmızı çerçeveler = Manuel düzeltilmişteki aynı koordinatlar (çalışmıyor)" << endl;
    cout << "\nHerhangi bir tuşa basın..." << endl;
    
    waitKey(0);
    
    return 0;
}
