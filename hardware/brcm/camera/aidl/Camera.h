/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIPI CSI-2 Camera HAL for Raspberry Pi 5
 * Supports cameras from all major manufacturers
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

namespace aidl::android::hardware::camera::rpi5 {

// Camera sensor types
enum class CameraSensorType {
    // Sony IMX Series
    IMX219,         // Pi Camera Module v2 (8MP)
    IMX477,         // Pi HQ Camera (12.3MP)
    IMX708,         // Pi Camera Module 3 (12MP)
    IMX296,         // Pi Global Shutter Camera
    IMX378,         // 12.3MP (similar to HQ)
    IMX462,         // Starlight sensor (2MP)
    IMX519,         // Arducam 16MP AF
    IMX283,         // 20MP Full Frame
    IMX290,         // 2MP Starlight
    IMX327,         // 2MP Starlight
    IMX335,         // 5MP
    IMX415,         // 8MP
    IMX577,         // 12MP
    IMX586,         // 48MP
    IMX682,         // 64MP
    
    // OmniVision Series
    OV5647,         // Pi Camera Module v1 (5MP)
    OV9281,         // 1MP Global Shutter
    OV2640,         // 2MP
    OV2680,         // 2MP
    OV2685,         // 2MP
    OV2710,         // 2MP
    OV4689,         // 4MP
    OV5640,         // 5MP AF
    OV5648,         // 5MP
    OV7251,         // VGA Global Shutter
    OV7740,         // VGA
    OV8856,         // 8MP
    OV8858,         // 8MP
    OV8865,         // 8MP
    OV13850,        // 13MP
    OV13855,        // 13MP
    OV13858,        // 13MP
    OV13B10,        // 13MP
    OV16825,        // 16MP
    OV64A,          // 64MP
    
    // Samsung Series
    S5K3L6,         // 13MP
    S5K4H7,         // 8MP
    S5K5E9,         // 5MP
    S5KGM1,         // 48MP
    S5KGM2,         // 48MP
    S5KGW1,         // 64MP
    S5KGW3,         // 64MP
    S5KHM2,         // 108MP
    S5KHM3,         // 108MP
    S5KJN1,         // 50MP
    S5K2L7,         // 12MP
    S5K3M5,         // 13MP
    
    // ON Semiconductor / Aptina
    AR0130,         // 1.2MP Global Shutter
    AR0144,         // 1MP Global Shutter
    AR0234,         // 2.3MP Global Shutter
    AR0330,         // 3.1MP
    AR0521,         // 5.1MP
    AR0522,         // 5.1MP Global Shutter
    AR1335,         // 13MP
    AR1820,         // 18MP
    MT9V034,        // VGA Global Shutter
    MT9M114,        // 1.2MP
    
    // Hynix
    HI556,          // 5MP
    HI846,          // 8MP
    HI1336,         // 13MP
    HI3516,         // 2MP
    
    // GalaxyCore
    GC2145,         // 2MP
    GC2385,         // 2MP
    GC5035,         // 5MP
    GC8034,         // 8MP
    GC08A3,         // 8MP
    GC13A0,         // 13MP
    
    // Superpix
    SP2509,         // 2MP
    SP250A,         // 2MP
    
    // Infrared / Thermal
    LEPTON,         // FLIR Lepton Thermal
    MLX90640,       // Melexis Thermal Array
    AMG8833,        // Panasonic GridEYE
    
    // TOF (Time of Flight)
    VL53L0X,        // ST ToF Ranging
    VL53L1X,        // ST ToF Ranging
    TMF8801,        // AMS ToF
    
    // Generic/Unknown
    GENERIC_CSI,
    UNKNOWN
};

// Camera interface type
enum class CameraInterface {
    CSI2_2LANE,     // 2-lane MIPI CSI-2
    CSI2_4LANE,     // 4-lane MIPI CSI-2
    USB_UVC,        // USB Video Class
    PARALLEL,       // Parallel interface
    LVDS            // LVDS interface
};

// Camera capabilities
struct CameraCapabilities {
    uint32_t maxWidth;
    uint32_t maxHeight;
    uint32_t maxFps;
    bool hasAutofocus;
    bool hasOIS;            // Optical Image Stabilization
    bool hasFlash;
    bool isGlobalShutter;
    bool hasHDR;
    bool hasRAW;
    float minFocalLength;
    float maxFocalLength;
    float sensorSize;       // diagonal in mm
    float pixelSize;        // microns
    uint32_t bitDepth;
    std::vector<std::string> supportedFormats;
};

// Camera sensor info structure
struct CameraSensorInfo {
    std::string name;
    std::string manufacturer;
    CameraSensorType type;
    CameraInterface interface;
    uint8_t i2cAddress;
    uint32_t width;
    uint32_t height;
    CameraCapabilities capabilities;
    std::string dtOverlay;
    std::string driverModule;
};

// Frame format
struct FrameFormat {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    std::string pixelFormat;    // YUYV, NV12, NV21, MJPEG, etc.
    uint32_t bytesPerLine;
    uint32_t sizeImage;
};

// Camera frame callback
using FrameCallback = std::function<void(const uint8_t* data, size_t size, uint64_t timestamp)>;

// Camera Manager class
class CameraManager {
public:
    static CameraManager& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Camera enumeration
    std::vector<std::string> getAvailableCameras();
    bool detectCameras();
    CameraSensorInfo getCameraInfo(const std::string& cameraId);
    
    // Camera operations
    bool openCamera(const std::string& cameraId);
    bool closeCamera(const std::string& cameraId);
    bool isOpen(const std::string& cameraId);
    
    // Configuration
    bool setFormat(const std::string& cameraId, const FrameFormat& format);
    FrameFormat getFormat(const std::string& cameraId);
    std::vector<FrameFormat> getSupportedFormats(const std::string& cameraId);
    
    // Streaming
    bool startStreaming(const std::string& cameraId, FrameCallback callback);
    bool stopStreaming(const std::string& cameraId);
    bool isStreaming(const std::string& cameraId);
    
    // Controls
    bool setExposure(const std::string& cameraId, int32_t exposure);
    bool setGain(const std::string& cameraId, int32_t gain);
    bool setWhiteBalance(const std::string& cameraId, int32_t temperature);
    bool setAutofocus(const std::string& cameraId, bool enable);
    bool triggerAutofocus(const std::string& cameraId);
    bool setHDR(const std::string& cameraId, bool enable);
    bool setFlash(const std::string& cameraId, bool enable);
    
    // Advanced controls
    bool setROI(const std::string& cameraId, int x, int y, int width, int height);
    bool setDigitalZoom(const std::string& cameraId, float zoom);
    bool setRotation(const std::string& cameraId, int degrees);
    bool setMirror(const std::string& cameraId, bool horizontal, bool vertical);
    
    // libcamera integration
    bool initLibcamera();
    std::string getLibcameraId(const std::string& cameraId);
    
private:
    CameraManager() = default;
    ~CameraManager() = default;
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;
    
    bool probeI2CCamera(int bus, uint8_t address, CameraSensorType expectedType);
    bool loadCameraDriver(const std::string& module);
    bool configureCsiPhy(int port, int lanes, uint32_t dataRate);
    
    std::map<std::string, CameraSensorInfo> mCameras;
    std::map<std::string, int> mCameraFds;
    std::map<std::string, bool> mStreamingState;
    bool mInitialized = false;
    bool mLibcameraReady = false;
};

// Supported camera database
static const std::vector<CameraSensorInfo> kSupportedCameras = {
    // Raspberry Pi Official Cameras
    {
        "Pi Camera Module v1", "Raspberry Pi", CameraSensorType::OV5647,
        CameraInterface::CSI2_2LANE, 0x36, 2592, 1944,
        {2592, 1944, 90, false, false, false, false, false, true, 3.04f, 3.04f, 3.67f, 1.4f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SBGGR10"}},
        "ov5647", "ov5647"
    },
    {
        "Pi Camera Module v2", "Raspberry Pi", CameraSensorType::IMX219,
        CameraInterface::CSI2_2LANE, 0x10, 3280, 2464,
        {3280, 2464, 30, false, false, false, false, true, true, 3.04f, 3.04f, 4.60f, 1.12f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10"}},
        "imx219", "imx219"
    },
    {
        "Pi Camera Module 3", "Raspberry Pi", CameraSensorType::IMX708,
        CameraInterface::CSI2_2LANE, 0x1A, 4608, 2592,
        {4608, 2592, 30, true, false, false, false, true, true, 2.75f, 2.75f, 7.4f, 1.4f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10", "SRGGB12"}},
        "imx708", "imx708"
    },
    {
        "Pi Camera Module 3 Wide", "Raspberry Pi", CameraSensorType::IMX708,
        CameraInterface::CSI2_2LANE, 0x1A, 4608, 2592,
        {4608, 2592, 30, true, false, false, false, true, true, 1.6f, 1.6f, 7.4f, 1.4f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10", "SRGGB12"}},
        "imx708_wide", "imx708"
    },
    {
        "Pi Camera Module 3 NoIR", "Raspberry Pi", CameraSensorType::IMX708,
        CameraInterface::CSI2_2LANE, 0x1A, 4608, 2592,
        {4608, 2592, 30, true, false, false, false, true, true, 2.75f, 2.75f, 7.4f, 1.4f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10", "SRGGB12"}},
        "imx708_noir", "imx708"
    },
    {
        "Pi HQ Camera", "Raspberry Pi", CameraSensorType::IMX477,
        CameraInterface::CSI2_2LANE, 0x1A, 4056, 3040,
        {4056, 3040, 30, false, false, false, false, true, true, 6.0f, 6.0f, 7.9f, 1.55f, 12,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB12"}},
        "imx477", "imx477"
    },
    {
        "Pi Global Shutter Camera", "Raspberry Pi", CameraSensorType::IMX296,
        CameraInterface::CSI2_2LANE, 0x1A, 1456, 1088,
        {1456, 1088, 60, false, false, false, true, false, true, 2.8f, 2.8f, 6.3f, 3.45f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10"}},
        "imx296", "imx296"
    },
    
    // Arducam Cameras
    {
        "Arducam 16MP AF", "Arducam", CameraSensorType::IMX519,
        CameraInterface::CSI2_2LANE, 0x1A, 4656, 3496,
        {4656, 3496, 30, true, false, false, false, true, true, 4.28f, 4.28f, 7.4f, 1.22f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10"}},
        "arducam_imx519", "imx519"
    },
    {
        "Arducam 64MP Hawk-eye", "Arducam", CameraSensorType::OV64A,
        CameraInterface::CSI2_4LANE, 0x36, 9152, 6944,
        {9152, 6944, 10, true, false, false, false, true, true, 4.7f, 4.7f, 9.0f, 0.8f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SBGGR10"}},
        "arducam_ov64a40", "ov64a40"
    },
    {
        "Arducam 12MP IMX378", "Arducam", CameraSensorType::IMX378,
        CameraInterface::CSI2_2LANE, 0x1A, 4032, 3024,
        {4032, 3024, 30, false, false, false, false, true, true, 4.0f, 4.0f, 7.81f, 1.55f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10"}},
        "arducam_imx378", "imx378"
    },
    {
        "Arducam OV9281 Global Shutter", "Arducam", CameraSensorType::OV9281,
        CameraInterface::CSI2_2LANE, 0x60, 1280, 800,
        {1280, 800, 120, false, false, false, true, false, true, 2.8f, 2.8f, 4.0f, 3.0f, 10,
         {"YUYV", "UYVY", "GREY", "Y10"}},
        "arducam_ov9281", "ov9281"
    },
    
    // Waveshare Cameras
    {
        "Waveshare IMX219-77", "Waveshare", CameraSensorType::IMX219,
        CameraInterface::CSI2_2LANE, 0x10, 3280, 2464,
        {3280, 2464, 30, false, false, false, false, true, true, 2.72f, 2.72f, 4.60f, 1.12f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10"}},
        "waveshare_imx219", "imx219"
    },
    {
        "Waveshare IMX219-160", "Waveshare", CameraSensorType::IMX219,
        CameraInterface::CSI2_2LANE, 0x10, 3280, 2464,
        {3280, 2464, 30, false, false, false, false, true, true, 1.87f, 1.87f, 4.60f, 1.12f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB10"}},
        "waveshare_imx219_160", "imx219"
    },
    {
        "Waveshare IMX477-77", "Waveshare", CameraSensorType::IMX477,
        CameraInterface::CSI2_2LANE, 0x1A, 4056, 3040,
        {4056, 3040, 30, false, false, false, false, true, true, 6.0f, 6.0f, 7.9f, 1.55f, 12,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB12"}},
        "waveshare_imx477", "imx477"
    },
    
    // Industrial/Machine Vision Cameras
    {
        "ON Semi AR0144 Global Shutter", "ON Semiconductor", CameraSensorType::AR0144,
        CameraInterface::CSI2_2LANE, 0x10, 1280, 800,
        {1280, 800, 60, false, false, false, true, false, true, 3.6f, 3.6f, 4.8f, 3.0f, 10,
         {"YUYV", "GREY", "Y10", "SGRBG10"}},
        "ar0144", "ar0144"
    },
    {
        "ON Semi AR0234 Global Shutter", "ON Semiconductor", CameraSensorType::AR0234,
        CameraInterface::CSI2_2LANE, 0x10, 1920, 1200,
        {1920, 1200, 120, false, false, false, true, false, true, 3.6f, 3.6f, 5.6f, 2.0f, 10,
         {"YUYV", "GREY", "Y10", "SGRBG10"}},
        "ar0234", "ar0234"
    },
    {
        "ON Semi AR0521", "ON Semiconductor", CameraSensorType::AR0521,
        CameraInterface::CSI2_2LANE, 0x36, 2592, 1944,
        {2592, 1944, 60, false, false, false, true, false, true, 3.45f, 3.45f, 6.55f, 2.2f, 12,
         {"YUYV", "SGRBG12"}},
        "ar0521", "ar0521"
    },
    {
        "Aptina MT9V034 Global Shutter", "ON Semiconductor", CameraSensorType::MT9V034,
        CameraInterface::CSI2_2LANE, 0x48, 752, 480,
        {752, 480, 60, false, false, false, true, false, true, 3.6f, 3.6f, 4.51f, 6.0f, 10,
         {"YUYV", "GREY"}},
        "mt9v034", "mt9v034"
    },
    
    // Starlight / Low-light Cameras
    {
        "IMX462 Starlight", "Sony", CameraSensorType::IMX462,
        CameraInterface::CSI2_2LANE, 0x1A, 1920, 1080,
        {1920, 1080, 30, false, false, false, false, true, true, 2.9f, 2.9f, 6.46f, 2.9f, 12,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB12"}},
        "imx462", "imx462"
    },
    {
        "IMX290 Starlight", "Sony", CameraSensorType::IMX290,
        CameraInterface::CSI2_2LANE, 0x1A, 1920, 1080,
        {1920, 1080, 30, false, false, false, false, true, true, 2.9f, 2.9f, 6.46f, 2.9f, 12,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB12"}},
        "imx290", "imx290"
    },
    {
        "IMX327 Starlight", "Sony", CameraSensorType::IMX327,
        CameraInterface::CSI2_2LANE, 0x1A, 1920, 1080,
        {1920, 1080, 30, false, false, false, false, true, true, 2.9f, 2.9f, 6.46f, 2.9f, 12,
         {"YUYV", "UYVY", "NV12", "NV21", "SRGGB12"}},
        "imx327", "imx327"
    },
    
    // OmniVision Common Sensors
    {
        "OV5640 Autofocus", "OmniVision", CameraSensorType::OV5640,
        CameraInterface::CSI2_2LANE, 0x3C, 2592, 1944,
        {2592, 1944, 15, true, false, false, false, false, true, 3.5f, 3.5f, 4.59f, 1.4f, 10,
         {"YUYV", "UYVY", "NV12", "NV21", "SBGGR8"}},
        "ov5640", "ov5640"
    },
    {
        "OV2640", "OmniVision", CameraSensorType::OV2640,
        CameraInterface::CSI2_2LANE, 0x30, 1600, 1200,
        {1600, 1200, 15, false, false, false, false, false, false, 3.5f, 3.5f, 4.0f, 2.2f, 8,
         {"YUYV", "UYVY", "JPEG"}},
        "ov2640", "ov2640"
    },
    
    // Thermal Cameras
    {
        "FLIR Lepton 3.5", "FLIR", CameraSensorType::LEPTON,
        CameraInterface::CSI2_2LANE, 0x2A, 160, 120,
        {160, 120, 9, false, false, false, false, false, true, 1.0f, 1.0f, 2.0f, 12.0f, 14,
         {"Y14", "Y16", "RGB888"}},
        "flir_lepton", "lepton"
    },
    {
        "MLX90640 Thermal Array", "Melexis", CameraSensorType::MLX90640,
        CameraInterface::CSI2_2LANE, 0x33, 32, 24,
        {32, 24, 16, false, false, false, false, false, true, 1.0f, 1.0f, 0.5f, 0.0f, 16,
         {"Y16"}},
        "mlx90640", "mlx90640"
    },
    
    // High Resolution Sensors
    {
        "Samsung S5KHM3 108MP", "Samsung", CameraSensorType::S5KHM3,
        CameraInterface::CSI2_4LANE, 0x10, 12000, 9000,
        {12000, 9000, 10, true, true, true, false, true, true, 7.0f, 7.0f, 10.0f, 0.64f, 10,
         {"YUYV", "NV12", "SGRBG10"}},
        "s5khm3", "s5khm3"
    },
    {
        "Sony IMX586 48MP", "Sony", CameraSensorType::IMX586,
        CameraInterface::CSI2_4LANE, 0x1A, 8000, 6000,
        {8000, 6000, 30, true, false, false, false, true, true, 4.3f, 4.3f, 8.0f, 0.8f, 10,
         {"YUYV", "NV12", "SRGGB10"}},
        "imx586", "imx586"
    },
    {
        "Sony IMX682 64MP", "Sony", CameraSensorType::IMX682,
        CameraInterface::CSI2_4LANE, 0x1A, 9248, 6944,
        {9248, 6944, 15, true, false, false, false, true, true, 4.5f, 4.5f, 9.0f, 0.8f, 10,
         {"YUYV", "NV12", "SRGGB10"}},
        "imx682", "imx682"
    }
};

}  // namespace aidl::android::hardware::camera::rpi5
