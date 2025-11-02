#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <stdexcept>

class CameraManager {
public:
    CameraManager();
    ~CameraManager();
    
    bool initCamera(int deviceId = 0);
    cv::Mat captureFrame();
    bool isReady() const;
    void releaseCamera();
    bool setResolution(int width, int height);
    bool setFPS(int fps);

private:
    cv::VideoCapture camera;
    bool isInitialized;
    int currentDeviceId;
    
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;
};

#endif
