// Copyright (C) 2024 The Android Open Source Project
// Camera Device implementation

#define LOG_TAG "CameraDevice"

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "CameraDevice.h"
#include "CameraMetadata.h"

namespace android {
namespace hardware {
namespace camera {

std::unique_ptr<CameraDevice> CameraDevice::create(int id, const std::string& devicePath) {
    int fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        return nullptr;
    }

    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        close(fd);
        return nullptr;
    }

    // Check if it's a video capture device
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close(fd);
        return nullptr;
    }

    close(fd);

    auto device = std::make_unique<CameraDevice>(id, devicePath);
    device->mName = reinterpret_cast<const char*>(cap.card);
    device->mDriver = reinterpret_cast<const char*>(cap.driver);

    // Determine if front or back camera based on name
    std::string name = android::base::ToLower(device->mName);
    if (name.find("front") != std::string::npos || 
        name.find("user") != std::string::npos) {
        device->mFacing = CAMERA_FACING_FRONT;
    } else {
        device->mFacing = CAMERA_FACING_BACK;
    }

    device->enumerateFormats();
    device->buildDefaultMetadata();

    return device;
}

CameraDevice::CameraDevice(int id, const std::string& devicePath)
    : mId(id),
      mDevicePath(devicePath),
      mFd(-1),
      mFacing(CAMERA_FACING_BACK),
      mBufferCount(0),
      mStreaming(false) {
    mDevice.common.version = CAMERA_DEVICE_API_VERSION_3_5;
    mDevice.ops = &sOps;
    mDevice.priv = this;
}

CameraDevice::~CameraDevice() {
    if (mFd >= 0) {
        close(mFd);
    }
}

void CameraDevice::enumerateFormats() {
    int fd = open(mDevicePath.c_str(), O_RDWR);
    if (fd < 0) return;

    // Enumerate supported formats
    struct v4l2_fmtdesc fmtdesc = {};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        // Enumerate frame sizes for this format
        struct v4l2_frmsizeenum frmsize = {};
        frmsize.pixel_format = fmtdesc.pixelformat;

        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                StreamConfig config = {
                    .width = frmsize.discrete.width,
                    .height = frmsize.discrete.height,
                    .format = fmtdesc.pixelformat,
                };
                mSupportedConfigs.push_back(config);
            }
            frmsize.index++;
        }
        fmtdesc.index++;
    }

    // Add default if none found
    if (mSupportedConfigs.empty()) {
        mSupportedConfigs.push_back({1920, 1080, V4L2_PIX_FMT_YUYV});
        mSupportedConfigs.push_back({1280, 720, V4L2_PIX_FMT_YUYV});
        mSupportedConfigs.push_back({640, 480, V4L2_PIX_FMT_YUYV});
    }

    close(fd);
}

void CameraDevice::buildDefaultMetadata() {
    CameraMetadata::buildStaticMetadata(this, &mStaticMetadata);
}

int CameraDevice::getCameraInfo(struct camera_info* info) {
    info->facing = mFacing;
    info->orientation = (mFacing == CAMERA_FACING_FRONT) ? 270 : 90;
    info->device_version = CAMERA_DEVICE_API_VERSION_3_5;
    info->static_camera_characteristics = mStaticMetadata.getAndLock();
    info->resource_cost = 50;
    info->conflicting_devices = nullptr;
    info->conflicting_devices_length = 0;
    return 0;
}

int CameraDevice::open(camera3_device_t** device) {
    mFd = ::open(mDevicePath.c_str(), O_RDWR);
    if (mFd < 0) {
        LOG(ERROR) << "Failed to open camera device: " << mDevicePath;
        return -ENODEV;
    }

    *device = &mDevice;
    return 0;
}

int CameraDevice::close() {
    if (mFd >= 0) {
        ::close(mFd);
        mFd = -1;
    }
    return 0;
}

int CameraDevice::initialize(const camera3_callback_ops_t* callbackOps) {
    mCallbackOps = callbackOps;
    return 0;
}

int CameraDevice::configureStreams(camera3_stream_configuration_t* streamList) {
    if (streamList == nullptr || streamList->num_streams == 0) {
        return -EINVAL;
    }

    for (uint32_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t* stream = streamList->streams[i];
        stream->usage |= GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        stream->max_buffers = 4;
    }

    return 0;
}

const camera_metadata_t* CameraDevice::constructDefaultRequestSettings(int type) {
    return CameraMetadata::buildRequestTemplate(type, &mStaticMetadata);
}

int CameraDevice::processCaptureRequest(camera3_capture_request_t* request) {
    if (request == nullptr) {
        return -EINVAL;
    }

    // Process request asynchronously
    std::lock_guard<std::mutex> lock(mRequestLock);
    
    // For now, just send back dummy results
    camera3_capture_result_t result = {};
    result.frame_number = request->frame_number;
    result.result = request->settings;
    result.num_output_buffers = request->num_output_buffers;
    result.output_buffers = request->output_buffers;
    result.partial_result = 1;

    mCallbackOps->process_capture_result(mCallbackOps, &result);

    return 0;
}

void CameraDevice::dump(int fd) {
    dprintf(fd, "Camera Device %d: %s\n", mId, mName.c_str());
    dprintf(fd, "  Device Path: %s\n", mDevicePath.c_str());
    dprintf(fd, "  Driver: %s\n", mDriver.c_str());
    dprintf(fd, "  Facing: %s\n", mFacing == CAMERA_FACING_FRONT ? "front" : "back");
    dprintf(fd, "  Supported configs: %zu\n", mSupportedConfigs.size());
}

int CameraDevice::flush() {
    return 0;
}

// Camera3 device operations
camera3_device_ops_t CameraDevice::sOps = {
    .initialize = [](const struct camera3_device* dev,
                     const camera3_callback_ops_t* ops) -> int {
        auto* device = static_cast<CameraDevice*>(dev->priv);
        return device->initialize(ops);
    },
    .configure_streams = [](const struct camera3_device* dev,
                            camera3_stream_configuration_t* stream_list) -> int {
        auto* device = static_cast<CameraDevice*>(dev->priv);
        return device->configureStreams(stream_list);
    },
    .register_stream_buffers = nullptr,
    .construct_default_request_settings = [](const struct camera3_device* dev,
                                              int type) -> const camera_metadata_t* {
        auto* device = static_cast<CameraDevice*>(dev->priv);
        return device->constructDefaultRequestSettings(type);
    },
    .process_capture_request = [](const struct camera3_device* dev,
                                   camera3_capture_request_t* request) -> int {
        auto* device = static_cast<CameraDevice*>(dev->priv);
        return device->processCaptureRequest(request);
    },
    .get_metadata_vendor_tag_ops = nullptr,
    .dump = [](const struct camera3_device* dev, int fd) {
        auto* device = static_cast<CameraDevice*>(dev->priv);
        device->dump(fd);
    },
    .flush = [](const struct camera3_device* dev) -> int {
        auto* device = static_cast<CameraDevice*>(dev->priv);
        return device->flush();
    },
    .reserved = {0},
};

}  // namespace camera
}  // namespace hardware
}  // namespace android
