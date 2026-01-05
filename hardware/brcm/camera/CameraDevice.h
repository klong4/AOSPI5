// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <hardware/camera3.h>
#include <hardware/gralloc.h>
#include <system/camera_metadata.h>
#include <CameraMetadata.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace android {
namespace hardware {
namespace camera {

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

struct StreamConfig {
    uint32_t width;
    uint32_t height;
    uint32_t format;
};

class CameraDevice {
public:
    static std::unique_ptr<CameraDevice> create(int id, const std::string& devicePath);
    ~CameraDevice();

    int getCameraInfo(struct camera_info* info);
    int open(camera3_device_t** device);
    int close();

    int getId() const { return mId; }
    const std::string& getName() const { return mName; }
    const std::vector<StreamConfig>& getSupportedConfigs() const { return mSupportedConfigs; }
    int getFacing() const { return mFacing; }

private:
    CameraDevice(int id, const std::string& devicePath);

    void enumerateFormats();
    void buildDefaultMetadata();
    
    // Camera3 operations
    int initialize(const camera3_callback_ops_t* callbackOps);
    int configureStreams(camera3_stream_configuration_t* streamList);
    const camera_metadata_t* constructDefaultRequestSettings(int type);
    int processCaptureRequest(camera3_capture_request_t* request);
    void dump(int fd);
    int flush();

    static camera3_device_ops_t sOps;

    int mId;
    std::string mDevicePath;
    std::string mName;
    std::string mDriver;
    int mFd;
    int mFacing;

    camera3_device_t mDevice;
    const camera3_callback_ops_t* mCallbackOps = nullptr;

    std::vector<StreamConfig> mSupportedConfigs;
    CameraMetadata mStaticMetadata;

    uint32_t mBufferCount;
    bool mStreaming;
    std::mutex mRequestLock;
};

}  // namespace camera
}  // namespace hardware
}  // namespace android
