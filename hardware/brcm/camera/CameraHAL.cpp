// Copyright (C) 2024 The Android Open Source Project
// Camera HAL implementation for Raspberry Pi 5

#define LOG_TAG "CameraHAL"

#include <android-base/logging.h>
#include <hardware/camera3.h>
#include <hardware/camera_common.h>
#include "CameraHAL.h"
#include "CameraDevice.h"

namespace android {
namespace hardware {
namespace camera {

static CameraHAL gCameraHAL;

CameraHAL::CameraHAL() : mCallbacks(nullptr) {
    LOG(INFO) << "CameraHAL constructor";
    enumerateCameras();
}

CameraHAL::~CameraHAL() {
    LOG(INFO) << "CameraHAL destructor";
}

void CameraHAL::enumerateCameras() {
    // Enumerate V4L2 cameras
    for (int i = 0; i < 10; i++) {
        std::string devicePath = "/dev/video" + std::to_string(i);
        std::unique_ptr<CameraDevice> camera = CameraDevice::create(i, devicePath);
        if (camera) {
            mCameras.push_back(std::move(camera));
            LOG(INFO) << "Found camera at " << devicePath;
        }
    }
    LOG(INFO) << "Enumerated " << mCameras.size() << " cameras";
}

int CameraHAL::getNumberOfCameras() {
    return static_cast<int>(mCameras.size());
}

int CameraHAL::getCameraInfo(int cameraId, struct camera_info* info) {
    if (cameraId < 0 || cameraId >= static_cast<int>(mCameras.size())) {
        return -EINVAL;
    }
    return mCameras[cameraId]->getCameraInfo(info);
}

int CameraHAL::setCallbacks(const camera_module_callbacks_t* callbacks) {
    mCallbacks = callbacks;
    return 0;
}

int CameraHAL::openCamera(int cameraId, camera3_device_t** device) {
    if (cameraId < 0 || cameraId >= static_cast<int>(mCameras.size())) {
        return -EINVAL;
    }
    return mCameras[cameraId]->open(device);
}

void CameraHAL::notifyCameraStatus(int cameraId, camera_device_status_t status) {
    if (mCallbacks && mCallbacks->camera_device_status_change) {
        mCallbacks->camera_device_status_change(mCallbacks, cameraId, status);
    }
}

// Camera module functions
static int get_number_of_cameras() {
    return gCameraHAL.getNumberOfCameras();
}

static int get_camera_info(int camera_id, struct camera_info* info) {
    return gCameraHAL.getCameraInfo(camera_id, info);
}

static int set_callbacks(const camera_module_callbacks_t* callbacks) {
    return gCameraHAL.setCallbacks(callbacks);
}

static int open_device(const hw_module_t* module, const char* name, hw_device_t** device) {
    if (name == nullptr) {
        return -EINVAL;
    }
    int cameraId = std::stoi(name);
    return gCameraHAL.openCamera(cameraId, reinterpret_cast<camera3_device_t**>(device));
}

static void get_vendor_tag_ops(vendor_tag_ops_t* ops) {
    // No vendor tags supported
}

static int open_legacy(const struct hw_module_t* module, const char* id,
                       uint32_t halVersion, struct hw_device_t** device) {
    return -ENOSYS;
}

static int set_torch_mode(const char* camera_id, bool enabled) {
    return -ENOSYS;
}

static int init() {
    return 0;
}

static struct hw_module_methods_t camera_module_methods = {
    .open = open_device,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = CAMERA_MODULE_API_VERSION_2_5,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "Raspberry Pi 5 Camera HAL",
        .author = "The Android Open Source Project",
        .methods = &camera_module_methods,
        .dso = nullptr,
        .reserved = {0},
    },
    .get_number_of_cameras = get_number_of_cameras,
    .get_camera_info = get_camera_info,
    .set_callbacks = set_callbacks,
    .get_vendor_tag_ops = get_vendor_tag_ops,
    .open_legacy = open_legacy,
    .set_torch_mode = set_torch_mode,
    .init = init,
    .reserved = {0},
};

}  // namespace camera
}  // namespace hardware
}  // namespace android
