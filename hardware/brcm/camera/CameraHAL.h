// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <hardware/camera3.h>
#include <hardware/camera_common.h>
#include <memory>
#include <vector>

namespace android {
namespace hardware {
namespace camera {

class CameraDevice;

class CameraHAL {
public:
    CameraHAL();
    ~CameraHAL();

    int getNumberOfCameras();
    int getCameraInfo(int cameraId, struct camera_info* info);
    int setCallbacks(const camera_module_callbacks_t* callbacks);
    int openCamera(int cameraId, camera3_device_t** device);
    void notifyCameraStatus(int cameraId, camera_device_status_t status);

private:
    void enumerateCameras();

    std::vector<std::unique_ptr<CameraDevice>> mCameras;
    const camera_module_callbacks_t* mCallbacks;
};

}  // namespace camera
}  // namespace hardware
}  // namespace android
