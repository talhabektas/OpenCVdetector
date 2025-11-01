#!/bin/bash

# OMR System Build Script
# Bu script projeyi derler ve çalıştırır

set -e  # Hata durumunda dur

echo "======================================"
echo "  OMR System - Build Script"
echo "======================================"

# Renk kodları
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Bağımlılık kontrolü
echo -e "${YELLOW}1. Bağımlılıklar kontrol ediliyor...${NC}"

check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}✗ $1 bulunamadı!${NC}"
        return 1
    else
        echo -e "${GREEN}✓ $1 mevcut${NC}"
        return 0
    fi
}

# CMake kontrolü
if ! check_command cmake; then
    echo -e "${RED}CMake kurulu değil! Lütfen CMake 3.15+ kurun.${NC}"
    exit 1
fi

# Tesseract kontrolü
if ! check_command tesseract; then
    echo -e "${RED}Tesseract OCR kurulu değil!${NC}"
    echo "macOS: brew install tesseract tesseract-lang"
    echo "Linux: sudo apt-get install tesseract-ocr tesseract-ocr-tur"
    exit 1
fi

echo -e "${GREEN}Tesseract versiyon:${NC}"
tesseract --version | head -n 1

# OpenCV kontrolü
if ! pkg-config --exists opencv4; then
    echo -e "${RED}OpenCV 4.x kurulu değil!${NC}"
    echo "macOS: brew install opencv"
    echo "Linux: sudo apt-get install libopencv-dev"
    exit 1
fi

echo -e "${GREEN}OpenCV versiyon:${NC}"
pkg-config --modversion opencv4

# Build klasörü oluştur
echo -e "\n${YELLOW}2. Build klasörü hazırlanıyor...${NC}"
if [ -d "build" ]; then
    echo "Mevcut build klasörü temizleniyor..."
    rm -rf build
fi

mkdir -p build
cd build

# CMake yapılandırma
echo -e "\n${YELLOW}3. CMake yapılandırması...${NC}"
cmake .. || {
    echo -e "${RED}CMake yapılandırması başarısız!${NC}"
    exit 1
}

# Derleme
echo -e "\n${YELLOW}4. Derleme yapılıyor...${NC}"

# CPU sayısını al
if [[ "$OSTYPE" == "darwin"* ]]; then
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    NUM_CORES=$(nproc)
fi

echo "Paralel derleme: $NUM_CORES çekirdek kullanılıyor..."
make -j$NUM_CORES || {
    echo -e "${RED}Derleme başarısız!${NC}"
    exit 1
}

echo -e "\n${GREEN}======================================"
echo "  ✓ Derleme başarılı!"
echo "======================================${NC}"

# Yürütülebilir dosya kontrolü
if [ -f "OMR_System" ]; then
    echo -e "${GREEN}Çalıştırılabilir: ./build/OMR_System${NC}"
    
    # Çalıştırma seçeneği
    echo -e "\n${YELLOW}Şimdi çalıştırmak ister misiniz? (y/n)${NC}"
    read -r response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]; then
        echo -e "\n${GREEN}OMR System başlatılıyor...${NC}\n"
        ./OMR_System
    else
        echo -e "\n${YELLOW}Manuel çalıştırma:${NC}"
        echo "  cd build"
        echo "  ./OMR_System                    # Kamera ile"
        echo "  ./OMR_System path/to/image.jpg  # Görüntü dosyası ile"
    fi
else
    echo -e "${RED}Hata: OMR_System bulunamadı!${NC}"
    exit 1
fi
