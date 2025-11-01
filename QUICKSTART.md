# Hızlı Başlangıç Kılavuzu

## 🚀 5 Dakikada OMR System

### 1. Bağımlılıkları Yükleyin

**macOS:**
```bash
brew install opencv tesseract tesseract-lang cmake
```

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libopencv-dev tesseract-ocr tesseract-ocr-tur libtesseract-dev libleptonica-dev
```

### 2. Projeyi Derleyin

```bash
cd /Users/mehmetalha/Desktop/detection
./build.sh
```

Build script otomatik olarak:
- ✓ Bağımlılıkları kontrol eder
- ✓ Build klasörü oluşturur
- ✓ CMake yapılandırması yapar
- ✓ Paralel derleme yapar
- ✓ Çalıştırma seçeneği sunar

### 3. Çalıştırın

**Seçenek 1: Kamera ile**
```bash
cd build
./OMR_System
```
- SPACE tuşu ile fotoğraf çekin
- ESC ile çıkış yapın

**Seçenek 2: Görüntü dosyası ile**
```bash
./OMR_System path/to/exam_image.jpg
```

### 4. Sonuçları Kontrol Edin

Otomatik oluşturulan dosyalar:
- `exam_YYYYMMDD_HHMMSS_results.txt` - Detaylı rapor
- `exam_YYYYMMDD_HHMMSS_results.csv` - CSV formatında
- `exam_YYYYMMDD_HHMMSS_results.jpg` - Görsel sonuç

## 📝 Cevap Anahtarı Düzenleme

`answer_key.txt` dosyasını düzenleyin:

```text
# Çoktan seçmeli: questionNum,MC,answer (0=A, 1=B, 2=C, 3=D, 4=E)
1,MC,2

# Boşluk doldurma: questionNum,FILL,text
11,FILL,Istanbul

# Doğru/Yanlış: questionNum,TF,answer (T=Doğru, F=Yanlış)
16,TF,T
```

## 🔧 Sorun mu Yaşıyorsunuz?

### Tesseract bulunamadı
```bash
# macOS
brew install tesseract tesseract-lang

# Linux
sudo apt-get install tesseract-ocr tesseract-ocr-tur
```

### OpenCV bulunamadı
```bash
# macOS
brew install opencv

# Linux
sudo apt-get install libopencv-dev
```

### Kamera açılmıyor
- macOS: System Preferences > Security & Privacy > Camera
- Görüntü dosyası ile test edin: `./OMR_System test.jpg`

## 📚 Daha Fazla Bilgi

Detaylı dokümantasyon için `README.md` dosyasına bakın.

## 💡 İpuçları

1. **En İyi Sonuçlar İçin:**
   - Kağıdı düz tutun
   - İyi ışık koşulları sağlayın
   - Net el yazısı kullanın
   - Balonları tamamen doldurun

2. **Performans:**
   - Release mode: `cmake -DCMAKE_BUILD_TYPE=Release ..`
   - Multi-threading otomatik aktif

3. **Özelleştirme:**
   - `main.cpp`: Kamera ID, dosya yolları
   - `SheetStructureAnalyzer.cpp`: Soru bölgeleri
   - `BubbleDetector.cpp`: Doluluk eşiği

---

🎉 Başarılar! OMR System kullanıma hazır!
