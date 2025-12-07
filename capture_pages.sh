#!/bin/bash
# Kağıtları kaydet ve koordinatları bul

cd /Users/mehmetalha/Desktop/detection

echo "📸 KAĞIT FOTOĞRAF KAYDEDICI"
echo ""
echo "Kullanım:"
echo "  1. Program açılacak"
echo "  2. SAYFA 1'i tut (el yazısı + T/F)"
echo "  3. SPACE tuşuna bas → 'page1.jpg' kaydedilir"
echo "  4. SAYFA 2'yi tut (çoktan seçmeli)"
echo "  5. SPACE tuşuna bas → 'page2.jpg' kaydedilir"
echo "  6. ESC → Çıkış"
echo ""
echo "Sonra koordinatları bulacağız ve manuel ayarlayacağız."
echo ""
read -p "Devam? (Enter)"

# Basit kamera capture programı yazalım
cat > /tmp/capture.cpp << 'EOF'
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;

int main() {
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Kamera açılamadı!" << std::endl;
        return -1;
    }
    
    int count = 1;
    
    std::cout << "SPACE - Fotoğraf çek, ESC - Çıkış" << std::endl;
    
    while (true) {
        Mat frame;
        cap >> frame;
        
        if (frame.empty()) break;
        
        putText(frame, "SPACE - Cek", Point(30, 40), 
               FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
        
        imshow("Capture", frame);
        
        int key = waitKey(1);
        if (key == 27) break; // ESC
        if (key == 32) { // SPACE
            std::string filename = "page" + std::to_string(count) + ".jpg";
            imwrite(filename, frame);
            std::cout << "✅ Kaydedildi: " << filename << std::endl;
            count++;
        }
    }
    
    cap.release();
    destroyAllWindows();
    return 0;
}
EOF

# Derle
g++ /tmp/capture.cpp -o /tmp/capture $(pkg-config --cflags --libs opencv4) 2>/dev/null

if [ $? -eq 0 ]; then
    /tmp/capture
    
    echo ""
    echo "✅ Fotoğraflar kaydedildi!"
    echo ""
    echo "Şimdi koordinatları bulalım:"
    
    if [ -f "page1.jpg" ]; then
        echo "  page1.jpg → image_preview ile aç ve koordinatları not et"
        ./build/image_preview page1.jpg &
    fi
    
else
    echo "❌ Derleme hatası. Manuel olarak fotoğraf çek ve kaydet."
fi
EOF
chmod +x /Users/mehmetalha/Desktop/detection/capture_pages.sh
