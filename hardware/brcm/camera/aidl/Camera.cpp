/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * MIPI CSI-2 Camera HAL Implementation for Raspberry Pi 5
 */

#include "Camera.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <dirent.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <regex>

#define LOG_TAG "CameraHAL"
#include <log/log.h>

namespace aidl::android::hardware::camera::rpi5 {

// I2C addresses for camera probing
static const std::map<CameraSensorType, std::vector<uint8_t>> kCameraI2CAddresses = {
    // Sony IMX Series
    {CameraSensorType::IMX219, {0x10}},
    {CameraSensorType::IMX477, {0x1A}},
    {CameraSensorType::IMX708, {0x1A}},
    {CameraSensorType::IMX296, {0x1A}},
    {CameraSensorType::IMX378, {0x1A}},
    {CameraSensorType::IMX462, {0x1A}},
    {CameraSensorType::IMX519, {0x1A}},
    {CameraSensorType::IMX290, {0x1A}},
    {CameraSensorType::IMX327, {0x1A}},
    {CameraSensorType::IMX335, {0x1A}},
    {CameraSensorType::IMX415, {0x1A}},
    {CameraSensorType::IMX586, {0x1A}},
    {CameraSensorType::IMX682, {0x1A}},
    
    // OmniVision
    {CameraSensorType::OV5647, {0x36}},
    {CameraSensorType::OV9281, {0x60}},
    {CameraSensorType::OV2640, {0x30}},
    {CameraSensorType::OV5640, {0x3C}},
    {CameraSensorType::OV64A, {0x36}},
    
    // ON Semi / Aptina
    {CameraSensorType::AR0144, {0x10, 0x18}},
    {CameraSensorType::AR0234, {0x10, 0x18}},
    {CameraSensorType::AR0521, {0x36}},
    {CameraSensorType::MT9V034, {0x48, 0x58}},
    
    // Samsung
    {CameraSensorType::S5KHM3, {0x10, 0x2D}},
    {CameraSensorType::S5KGW3, {0x10}},
    
    // Thermal
    {CameraSensorType::LEPTON, {0x2A}},
    {CameraSensorType::MLX90640, {0x33}}
};

// V4L2 helper functions
static int xioctl(int fd, unsigned long request, void* arg) {
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

CameraManager& CameraManager::getInstance() {
    static CameraManager instance;
    return instance;
}

bool CameraManager::initialize() {
    if (mInitialized) {
        return true;
    }
    
    ALOGI("Initializing Camera HAL for Raspberry Pi 5");
    
    // Initialize libcamera if available
    initLibcamera();
    
    // Detect connected cameras
    if (!detectCameras()) {
        ALOGW("No cameras detected during initialization");
    }
    
    mInitialized = true;
    ALOGI("Camera HAL initialized, found %zu cameras", mCameras.size());
    return true;
}

void CameraManager::shutdown() {
    // Close all open cameras
    for (auto& pair : mCameraFds) {
        if (pair.second >= 0) {
            close(pair.second);
        }
    }
    mCameraFds.clear();
    mCameras.clear();
    mStreamingState.clear();
    mInitialized = false;
    ALOGI("Camera HAL shutdown complete");
}

std::vector<std::string> CameraManager::getAvailableCameras() {
    std::vector<std::string> cameras;
    for (const auto& pair : mCameras) {
        cameras.push_back(pair.first);
    }
    return cameras;
}

bool CameraManager::detectCameras() {
    mCameras.clear();
    
    // Enumerate V4L2 video devices
    DIR* dir = opendir("/dev");
    if (!dir) {
        ALOGE("Failed to open /dev directory");
        return false;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        
        // Check for video devices
        if (name.find("video") == 0) {
            std::string path = "/dev/" + name;
            
            int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
            if (fd < 0) {
                continue;
            }
            
            // Query device capabilities
            struct v4l2_capability cap;
            if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                // Check if it's a video capture device
                if ((cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) ||
                    (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
                    
                    ALOGI("Found camera device: %s (%s)", path.c_str(), cap.card);
                    
                    // Create camera info
                    CameraSensorInfo info;
                    info.name = reinterpret_cast<const char*>(cap.card);
                    info.manufacturer = "Unknown";
                    info.type = CameraSensorType::GENERIC_CSI;
                    info.interface = CameraInterface::CSI2_2LANE;
                    
                    // Try to identify the sensor
                    identifySensor(fd, info);
                    
                    // Query supported formats
                    queryFormats(fd, info);
                    
                    std::string cameraId = "camera" + std::to_string(mCameras.size());
                    mCameras[cameraId] = info;
                }
            }
            
            close(fd);
        }
    }
    
    closedir(dir);
    
    // Also check for unicam devices (RPi specific)
    detectUnicamCameras();
    
    return !mCameras.empty();
}

void CameraManager::detectUnicamCameras() {
    // Check for RPi camera subsystem via unicam
    const std::vector<std::string> unicamPaths = {
        "/dev/v4l-subdev0",
        "/dev/v4l-subdev1",
        "/dev/v4l-subdev2",
        "/dev/v4l-subdev3"
    };
    
    for (const auto& path : unicamPaths) {
        int fd = open(path.c_str(), O_RDWR);
        if (fd < 0) {
            continue;
        }
        
        // Query subdev info
        struct v4l2_subdev_capability cap;
        memset(&cap, 0, sizeof(cap));
        
        if (xioctl(fd, VIDIOC_SUBDEV_QUERYCAP, &cap) == 0) {
            ALOGI("Found subdev: %s", path.c_str());
            
            // Try to identify the sensor through chip info
            struct v4l2_dbg_chip_info chip;
            memset(&chip, 0, sizeof(chip));
            
            if (xioctl(fd, VIDIOC_DBG_G_CHIP_INFO, &chip) == 0) {
                ALOGI("  Chip: %s", chip.name);
            }
        }
        
        close(fd);
    }
}

void CameraManager::identifySensor(int fd, CameraSensorInfo& info) {
    // Try to identify sensor from device name
    std::string deviceName = info.name;
    std::transform(deviceName.begin(), deviceName.end(), deviceName.begin(), ::tolower);
    
    // Match known sensors
    for (const auto& knownCamera : kSupportedCameras) {
        std::string knownName = knownCamera.name;
        std::transform(knownName.begin(), knownName.end(), knownName.begin(), ::tolower);
        
        if (deviceName.find(knownCamera.driverModule) != std::string::npos) {
            info = knownCamera;
            ALOGI("Identified sensor: %s", info.name.c_str());
            return;
        }
    }
}

void CameraManager::queryFormats(int fd, CameraSensorInfo& info) {
    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    info.capabilities.supportedFormats.clear();
    
    while (xioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
        info.capabilities.supportedFormats.push_back(
            reinterpret_cast<const char*>(fmt.description));
        fmt.index++;
    }
}

CameraSensorInfo CameraManager::getCameraInfo(const std::string& cameraId) {
    auto it = mCameras.find(cameraId);
    if (it != mCameras.end()) {
        return it->second;
    }
    return CameraSensorInfo();
}

bool CameraManager::openCamera(const std::string& cameraId) {
    auto it = mCameras.find(cameraId);
    if (it == mCameras.end()) {
        ALOGE("Camera %s not found", cameraId.c_str());
        return false;
    }
    
    // Find the video device for this camera
    std::string devicePath = "/dev/video0";  // Default, should be mapped properly
    
    int fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        ALOGE("Failed to open camera device %s: %s", devicePath.c_str(), strerror(errno));
        return false;
    }
    
    mCameraFds[cameraId] = fd;
    ALOGI("Opened camera %s on %s (fd=%d)", cameraId.c_str(), devicePath.c_str(), fd);
    return true;
}

bool CameraManager::closeCamera(const std::string& cameraId) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) {
        return false;
    }
    
    // Stop streaming if active
    if (isStreaming(cameraId)) {
        stopStreaming(cameraId);
    }
    
    close(it->second);
    mCameraFds.erase(it);
    ALOGI("Closed camera %s", cameraId.c_str());
    return true;
}

bool CameraManager::isOpen(const std::string& cameraId) {
    return mCameraFds.find(cameraId) != mCameraFds.end();
}

bool CameraManager::setFormat(const std::string& cameraId, const FrameFormat& format) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) {
        return false;
    }
    
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = format.width;
    fmt.fmt.pix.height = format.height;
    
    // Map pixel format string to V4L2 fourcc
    if (format.pixelFormat == "YUYV") {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    } else if (format.pixelFormat == "NV12") {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    } else if (format.pixelFormat == "NV21") {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
    } else if (format.pixelFormat == "MJPEG") {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    } else {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }
    
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    
    if (xioctl(it->second, VIDIOC_S_FMT, &fmt) < 0) {
        ALOGE("Failed to set format: %s", strerror(errno));
        return false;
    }
    
    // Set frame rate
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = format.fps;
    
    xioctl(it->second, VIDIOC_S_PARM, &parm);
    
    ALOGI("Set format: %ux%u @ %u fps", format.width, format.height, format.fps);
    return true;
}

FrameFormat CameraManager::getFormat(const std::string& cameraId) {
    FrameFormat format;
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) {
        return format;
    }
    
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (xioctl(it->second, VIDIOC_G_FMT, &fmt) == 0) {
        format.width = fmt.fmt.pix.width;
        format.height = fmt.fmt.pix.height;
        format.bytesPerLine = fmt.fmt.pix.bytesperline;
        format.sizeImage = fmt.fmt.pix.sizeimage;
        
        // Map fourcc to string
        char fourcc[5] = {0};
        fourcc[0] = fmt.fmt.pix.pixelformat & 0xff;
        fourcc[1] = (fmt.fmt.pix.pixelformat >> 8) & 0xff;
        fourcc[2] = (fmt.fmt.pix.pixelformat >> 16) & 0xff;
        fourcc[3] = (fmt.fmt.pix.pixelformat >> 24) & 0xff;
        format.pixelFormat = fourcc;
    }
    
    // Get frame rate
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (xioctl(it->second, VIDIOC_G_PARM, &parm) == 0) {
        if (parm.parm.capture.timeperframe.numerator > 0) {
            format.fps = parm.parm.capture.timeperframe.denominator /
                        parm.parm.capture.timeperframe.numerator;
        }
    }
    
    return format;
}

std::vector<FrameFormat> CameraManager::getSupportedFormats(const std::string& cameraId) {
    std::vector<FrameFormat> formats;
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) {
        return formats;
    }
    
    // Enumerate pixel formats
    struct v4l2_fmtdesc fmtDesc;
    memset(&fmtDesc, 0, sizeof(fmtDesc));
    fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    while (xioctl(it->second, VIDIOC_ENUM_FMT, &fmtDesc) == 0) {
        // Enumerate frame sizes for this format
        struct v4l2_frmsizeenum frmSize;
        memset(&frmSize, 0, sizeof(frmSize));
        frmSize.pixel_format = fmtDesc.pixelformat;
        
        while (xioctl(it->second, VIDIOC_ENUM_FRAMESIZES, &frmSize) == 0) {
            if (frmSize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                // Enumerate frame intervals
                struct v4l2_frmivalenum frmIval;
                memset(&frmIval, 0, sizeof(frmIval));
                frmIval.pixel_format = fmtDesc.pixelformat;
                frmIval.width = frmSize.discrete.width;
                frmIval.height = frmSize.discrete.height;
                
                while (xioctl(it->second, VIDIOC_ENUM_FRAMEINTERVALS, &frmIval) == 0) {
                    FrameFormat format;
                    format.width = frmSize.discrete.width;
                    format.height = frmSize.discrete.height;
                    
                    char fourcc[5] = {0};
                    fourcc[0] = fmtDesc.pixelformat & 0xff;
                    fourcc[1] = (fmtDesc.pixelformat >> 8) & 0xff;
                    fourcc[2] = (fmtDesc.pixelformat >> 16) & 0xff;
                    fourcc[3] = (fmtDesc.pixelformat >> 24) & 0xff;
                    format.pixelFormat = fourcc;
                    
                    if (frmIval.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        format.fps = frmIval.discrete.denominator / frmIval.discrete.numerator;
                    }
                    
                    formats.push_back(format);
                    frmIval.index++;
                }
            }
            frmSize.index++;
        }
        fmtDesc.index++;
    }
    
    return formats;
}

bool CameraManager::startStreaming(const std::string& cameraId, FrameCallback callback) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) {
        return false;
    }
    
    int fd = it->second;
    
    // Request buffers
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        ALOGE("Failed to request buffers: %s", strerror(errno));
        return false;
    }
    
    // Map buffers and queue them
    std::vector<void*> buffers;
    std::vector<size_t> bufferSizes;
    
    for (uint32_t i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            ALOGE("Failed to query buffer: %s", strerror(errno));
            return false;
        }
        
        void* buffer = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, buf.m.offset);
        if (buffer == MAP_FAILED) {
            ALOGE("Failed to mmap buffer: %s", strerror(errno));
            return false;
        }
        
        buffers.push_back(buffer);
        bufferSizes.push_back(buf.length);
        
        // Queue the buffer
        if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            ALOGE("Failed to queue buffer: %s", strerror(errno));
            return false;
        }
    }
    
    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        ALOGE("Failed to start streaming: %s", strerror(errno));
        return false;
    }
    
    mStreamingState[cameraId] = true;
    ALOGI("Started streaming on camera %s", cameraId.c_str());
    
    // Start capture thread
    std::thread captureThread([this, cameraId, fd, buffers, bufferSizes, callback]() {
        while (mStreamingState[cameraId]) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            
            // Dequeue buffer
            if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
                if (errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                break;
            }
            
            // Call the callback with frame data
            if (callback) {
                uint64_t timestamp = buf.timestamp.tv_sec * 1000000000ULL +
                                    buf.timestamp.tv_usec * 1000ULL;
                callback(static_cast<uint8_t*>(buffers[buf.index]),
                        buf.bytesused, timestamp);
            }
            
            // Re-queue the buffer
            if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
                break;
            }
        }
    });
    captureThread.detach();
    
    return true;
}

bool CameraManager::stopStreaming(const std::string& cameraId) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) {
        return false;
    }
    
    mStreamingState[cameraId] = false;
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(it->second, VIDIOC_STREAMOFF, &type) < 0) {
        ALOGE("Failed to stop streaming: %s", strerror(errno));
        return false;
    }
    
    ALOGI("Stopped streaming on camera %s", cameraId.c_str());
    return true;
}

bool CameraManager::isStreaming(const std::string& cameraId) {
    auto it = mStreamingState.find(cameraId);
    return it != mStreamingState.end() && it->second;
}

// Camera controls
bool CameraManager::setExposure(const std::string& cameraId, int32_t exposure) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE;
    ctrl.value = exposure;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setGain(const std::string& cameraId, int32_t gain) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_GAIN;
    ctrl.value = gain;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setWhiteBalance(const std::string& cameraId, int32_t temperature) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    ctrl.value = temperature;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setAutofocus(const std::string& cameraId, bool enable) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_FOCUS_AUTO;
    ctrl.value = enable ? 1 : 0;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::triggerAutofocus(const std::string& cameraId) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_AUTO_FOCUS_START;
    ctrl.value = 1;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setHDR(const std::string& cameraId, bool enable) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_WIDE_DYNAMIC_RANGE;
    ctrl.value = enable ? 1 : 0;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setFlash(const std::string& cameraId, bool enable) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_FLASH_LED_MODE;
    ctrl.value = enable ? V4L2_FLASH_LED_MODE_FLASH : V4L2_FLASH_LED_MODE_NONE;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setROI(const std::string& cameraId, int x, int y, int width, int height) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_selection sel;
    memset(&sel, 0, sizeof(sel));
    sel.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sel.target = V4L2_SEL_TGT_CROP;
    sel.r.left = x;
    sel.r.top = y;
    sel.r.width = width;
    sel.r.height = height;
    
    return xioctl(it->second, VIDIOC_S_SELECTION, &sel) == 0;
}

bool CameraManager::setDigitalZoom(const std::string& cameraId, float zoom) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_ZOOM_ABSOLUTE;
    ctrl.value = static_cast<int32_t>(zoom * 100);
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setRotation(const std::string& cameraId, int degrees) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_ROTATE;
    ctrl.value = degrees;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::setMirror(const std::string& cameraId, bool horizontal, bool vertical) {
    auto it = mCameraFds.find(cameraId);
    if (it == mCameraFds.end()) return false;
    
    struct v4l2_control ctrl;
    
    ctrl.id = V4L2_CID_HFLIP;
    ctrl.value = horizontal ? 1 : 0;
    xioctl(it->second, VIDIOC_S_CTRL, &ctrl);
    
    ctrl.id = V4L2_CID_VFLIP;
    ctrl.value = vertical ? 1 : 0;
    return xioctl(it->second, VIDIOC_S_CTRL, &ctrl) == 0;
}

bool CameraManager::initLibcamera() {
    // Check if libcamera is available
    std::ifstream libcameraTest("/usr/lib/aarch64-linux-gnu/libcamera.so");
    if (!libcameraTest.good()) {
        ALOGI("libcamera not available, using V4L2 only");
        mLibcameraReady = false;
        return false;
    }
    
    mLibcameraReady = true;
    ALOGI("libcamera integration available");
    return true;
}

std::string CameraManager::getLibcameraId(const std::string& cameraId) {
    // Map our camera ID to libcamera camera ID
    auto it = mCameras.find(cameraId);
    if (it == mCameras.end()) {
        return "";
    }
    
    // libcamera uses paths like "/base/soc/i2c0mux/i2c@1/imx708@1a"
    return "/base/soc/i2c0mux/i2c@1/" + it->second.driverModule;
}

bool CameraManager::probeI2CCamera(int bus, uint8_t address, CameraSensorType expectedType) {
    char path[32];
    snprintf(path, sizeof(path), "/dev/i2c-%d", bus);
    
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return false;
    }
    
    // Set I2C slave address
    if (ioctl(fd, 0x0703 /* I2C_SLAVE */, address) < 0) {
        close(fd);
        return false;
    }
    
    // Try to read a register to verify camera presence
    uint8_t reg = 0x00;
    uint8_t value;
    if (write(fd, &reg, 1) == 1 && read(fd, &value, 1) == 1) {
        close(fd);
        return true;
    }
    
    close(fd);
    return false;
}

bool CameraManager::loadCameraDriver(const std::string& module) {
    std::string cmd = "modprobe " + module;
    return system(cmd.c_str()) == 0;
}

bool CameraManager::configureCsiPhy(int port, int lanes, uint32_t dataRate) {
    // Configure CSI-2 PHY for the specified port
    // This is hardware-specific for BCM2712
    ALOGI("Configuring CSI port %d with %d lanes at %u Mbps", port, lanes, dataRate);
    
    // The actual configuration is done through device tree overlays
    // and kernel drivers, this is just for runtime adjustments
    return true;
}

}  // namespace aidl::android::hardware::camera::rpi5
