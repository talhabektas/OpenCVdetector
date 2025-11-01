#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <stdexcept>

/**
 * @class CameraManager
 * @brief Manages camera initialization, frame capture, and resource cleanup
 * 
 * This class provides a clean interface for camera operations following RAII principles.
 */
class CameraManager {
public:
    /**
     * @brief Default constructor
     */
    CameraManager();
    
    /**
     * @brief Destructor - ensures camera is released
     */
    ~CameraManager();
    
    /**
     * @brief Initialize camera with specified device ID
     * @param deviceId Camera device ID (default: 0 for built-in camera)
     * @return true if initialization successful, false otherwise
     */
    bool initCamera(int deviceId = 0);
    
    /**
     * @brief Capture a single frame from the camera
     * @return cv::Mat containing the captured frame
     * @throws std::runtime_error if camera is not initialized or frame capture fails
     */
    cv::Mat captureFrame();
    
    /**
     * @brief Check if camera is currently open and ready
     * @return true if camera is ready, false otherwise
     */
    bool isReady() const;
    
    /**
     * @brief Release camera resources
     */
    void releaseCamera();
    
    /**
     * @brief Set camera resolution
     * @param width Desired width
     * @param height Desired height
     * @return true if successful, false otherwise
     */
    bool setResolution(int width, int height);
    
    /**
     * @brief Set camera FPS
     * @param fps Desired frames per second
     * @return true if successful, false otherwise
     */
    bool setFPS(int fps);

private:
    cv::VideoCapture camera;
    bool isInitialized;
    int currentDeviceId;
    
    // Disable copy constructor and assignment operator
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;
};

#endif // CAMERA_MANAGER_H
