#!/bin/bash
# Kamera capture script - kare kaydeder

cd /Users/mehmetalha/Desktop/detection

echo "📷 Kameradan fotoğraf çekiliyor..."
echo "Pencere açıldığında:"
echo "  - Kağıdı düzgün yerleştir"
echo "  - SPACE tuşuna bas → Fotoğraf kaydedilir"
echo "  - ESC → Çıkış"

./build/live_reader

echo "✅ Fotoğraf kaydedildi!"
