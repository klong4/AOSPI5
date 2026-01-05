// Copyright (C) 2024 The Android Open Source Project

#pragma once

#include <CameraMetadata.h>
#include <system/camera_metadata.h>
#include <hardware/gralloc.h>

namespace android {
namespace hardware {
namespace camera {

class CameraDevice;

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

class CameraMetadataHelper {
public:
    static void buildStaticMetadata(CameraDevice* device, CameraMetadata* metadata);
    static const camera_metadata_t* buildRequestTemplate(int type, CameraMetadata* staticMeta);
};

}  // namespace camera
}  // namespace hardware
}  // namespace android
