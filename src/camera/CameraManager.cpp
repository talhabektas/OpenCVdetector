#include "CameraManager.h"
#include <iostream>

CameraManager::CameraManager() : isInitialized(false), currentDeviceId(-1) {
}

CameraManager::~CameraManager() {
    releaseCamera();
}

bool CameraManager::initCamera(int deviceId) {
    try {
        // Release previous camera if exists
        if (isInitialized) {
            releaseCamera();
        }
        
        // Open camera
        camera.open(deviceId);
        
        if (!camera.isOpened()) {
            std::cerr << "Hata: Kamera açılamadı (Device ID: " << deviceId << ")" << std::endl;
            return false;
        }
        
        currentDeviceId = deviceId;
        isInitialized = true;
        
        // Set default resolution (720p for better performance)
        setResolution(1280, 720);
        
        std::cout << "Kamera başarıyla başlatıldı (Device ID: " << deviceId << ")" << std::endl;
        return true;
        
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV Hatası: " << e.what() << std::endl;
        isInitialized = false;
        return false;
    }
}

cv::Mat CameraManager::captureFrame() {
    if (!isInitialized || !camera.isOpened()) {
        throw std::runtime_error("Kamera başlatılmamış veya hazır değil!");
    }
    
    cv::Mat frame;
    try {
        camera >> frame;
        
        if (frame.empty()) {
            throw std::runtime_error("Boş frame yakalandı!");
        }
        
        return frame;
        
    } catch (const cv::Exception& e) {
        throw std::runtime_error(std::string("Frame yakalama hatası: ") + e.what());
    }
}

bool CameraManager::isReady() const {
    return isInitialized && camera.isOpened();
}

void CameraManager::releaseCamera() {
    if (camera.isOpened()) {
        camera.release();
        std::cout << "Kamera kaynağı serbest bırakıldı" << std::endl;
    }
    isInitialized = false;
    currentDeviceId = -1;
}

bool CameraManager::setResolution(int width, int height) {
    if (!isInitialized) {
        return false;
    }
    
    camera.set(cv::CAP_PROP_FRAME_WIDTH, width);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    
    // Verify the resolution was set
    int actualWidth = static_cast<int>(camera.get(cv::CAP_PROP_FRAME_WIDTH));
    int actualHeight = static_cast<int>(camera.get(cv::CAP_PROP_FRAME_HEIGHT));
    
    std::cout << "Çözünürlük ayarlandı: " << actualWidth << "x" << actualHeight << std::endl;
    
    return (actualWidth == width && actualHeight == height);
}

bool CameraManager::setFPS(int fps) {
    if (!isInitialized) {
        return false;
    }
    
    camera.set(cv::CAP_PROP_FPS, fps);
    
    int actualFPS = static_cast<int>(camera.get(cv::CAP_PROP_FPS));
    std::cout << "FPS ayarlandı: " << actualFPS << std::endl;
    
    return true;
}
