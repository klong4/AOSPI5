// Copyright (C) 2024 The Android Open Source Project
// Camera Metadata utilities

#define LOG_TAG "CameraMetadata"

#include <android-base/logging.h>
#include <system/camera_metadata.h>
#include "CameraMetadata.h"
#include "CameraDevice.h"

namespace android {
namespace hardware {
namespace camera {

using ::android::hardware::camera::common::V1_0::helper::CameraMetadata;

void CameraMetadataHelper::buildStaticMetadata(CameraDevice* device, CameraMetadata* metadata) {
    // Sensor info
    const auto& configs = device->getSupportedConfigs();
    
    // Find max resolution
    int32_t maxWidth = 0, maxHeight = 0;
    for (const auto& config : configs) {
        if (config.width > static_cast<uint32_t>(maxWidth)) {
            maxWidth = config.width;
            maxHeight = config.height;
        }
    }

    int32_t activeArraySize[] = {0, 0, maxWidth, maxHeight};
    metadata->update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArraySize, 4);

    int32_t pixelArraySize[] = {maxWidth, maxHeight};
    metadata->update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixelArraySize, 2);

    int32_t physicalSize[] = {4800, 3600}; // 4.8mm x 3.6mm typical sensor
    metadata->update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE, physicalSize, 2);

    // Lens info
    float aperture = 2.0f;
    metadata->update(ANDROID_LENS_INFO_AVAILABLE_APERTURES, &aperture, 1);

    float focalLength = 3.04f;
    metadata->update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, &focalLength, 1);

    float minFocusDistance = 0.0f;
    metadata->update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE, &minFocusDistance, 1);

    float hyperfocalDistance = 0.0f;
    metadata->update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE, &hyperfocalDistance, 1);

    uint8_t lensFacing = device->getFacing() == CAMERA_FACING_FRONT ?
                         ANDROID_LENS_FACING_FRONT : ANDROID_LENS_FACING_BACK;
    metadata->update(ANDROID_LENS_FACING, &lensFacing, 1);

    // Scaler info - available stream configurations
    std::vector<int32_t> streamConfigs;
    for (const auto& config : configs) {
        // YUV420
        streamConfigs.push_back(HAL_PIXEL_FORMAT_YCbCr_420_888);
        streamConfigs.push_back(config.width);
        streamConfigs.push_back(config.height);
        streamConfigs.push_back(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);

        // JPEG
        streamConfigs.push_back(HAL_PIXEL_FORMAT_BLOB);
        streamConfigs.push_back(config.width);
        streamConfigs.push_back(config.height);
        streamConfigs.push_back(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);

        // IMPLEMENTATION_DEFINED
        streamConfigs.push_back(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs.push_back(config.width);
        streamConfigs.push_back(config.height);
        streamConfigs.push_back(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }
    metadata->update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                     streamConfigs.data(), streamConfigs.size());

    // Min frame durations
    std::vector<int64_t> frameDurations;
    for (const auto& config : configs) {
        int64_t duration = 33333333LL; // 30fps

        frameDurations.push_back(HAL_PIXEL_FORMAT_YCbCr_420_888);
        frameDurations.push_back(config.width);
        frameDurations.push_back(config.height);
        frameDurations.push_back(duration);

        frameDurations.push_back(HAL_PIXEL_FORMAT_BLOB);
        frameDurations.push_back(config.width);
        frameDurations.push_back(config.height);
        frameDurations.push_back(duration);

        frameDurations.push_back(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        frameDurations.push_back(config.width);
        frameDurations.push_back(config.height);
        frameDurations.push_back(duration);
    }
    metadata->update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                     frameDurations.data(), frameDurations.size());

    // Stall durations for JPEG
    std::vector<int64_t> stallDurations;
    for (const auto& config : configs) {
        int64_t stallDuration = 100000000LL; // 100ms for JPEG

        stallDurations.push_back(HAL_PIXEL_FORMAT_BLOB);
        stallDurations.push_back(config.width);
        stallDurations.push_back(config.height);
        stallDurations.push_back(stallDuration);

        stallDurations.push_back(HAL_PIXEL_FORMAT_YCbCr_420_888);
        stallDurations.push_back(config.width);
        stallDurations.push_back(config.height);
        stallDurations.push_back(0);

        stallDurations.push_back(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        stallDurations.push_back(config.width);
        stallDurations.push_back(config.height);
        stallDurations.push_back(0);
    }
    metadata->update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
                     stallDurations.data(), stallDurations.size());

    // Max digital zoom
    float maxZoom = 4.0f;
    metadata->update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxZoom, 1);

    // Cropping type
    uint8_t croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    metadata->update(ANDROID_SCALER_CROPPING_TYPE, &croppingType, 1);

    // Request capabilities
    uint8_t capabilities[] = {
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
    };
    metadata->update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                     capabilities, sizeof(capabilities));

    // Supported hardware level
    uint8_t hwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    metadata->update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL, &hwLevel, 1);

    // Max JPEG size
    int32_t maxJpegSize = maxWidth * maxHeight * 3 / 2;
    metadata->update(ANDROID_JPEG_MAX_SIZE, &maxJpegSize, 1);

    // Control modes
    uint8_t controlModes[] = {
        ANDROID_CONTROL_MODE_OFF,
        ANDROID_CONTROL_MODE_AUTO,
    };
    metadata->update(ANDROID_CONTROL_AVAILABLE_MODES, controlModes, sizeof(controlModes));

    // AE modes
    uint8_t aeModes[] = {
        ANDROID_CONTROL_AE_MODE_OFF,
        ANDROID_CONTROL_AE_MODE_ON,
    };
    metadata->update(ANDROID_CONTROL_AE_AVAILABLE_MODES, aeModes, sizeof(aeModes));

    // AWB modes
    uint8_t awbModes[] = {
        ANDROID_CONTROL_AWB_MODE_OFF,
        ANDROID_CONTROL_AWB_MODE_AUTO,
    };
    metadata->update(ANDROID_CONTROL_AWB_AVAILABLE_MODES, awbModes, sizeof(awbModes));

    // AF modes (fixed focus)
    uint8_t afModes[] = {
        ANDROID_CONTROL_AF_MODE_OFF,
    };
    metadata->update(ANDROID_CONTROL_AF_AVAILABLE_MODES, afModes, sizeof(afModes));

    // Scene modes
    uint8_t sceneModes[] = {
        ANDROID_CONTROL_SCENE_MODE_DISABLED,
    };
    metadata->update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES, sceneModes, sizeof(sceneModes));

    // Effect modes
    uint8_t effectModes[] = {
        ANDROID_CONTROL_EFFECT_MODE_OFF,
    };
    metadata->update(ANDROID_CONTROL_AVAILABLE_EFFECTS, effectModes, sizeof(effectModes));

    // Sync max latency
    int32_t maxLatency = ANDROID_SYNC_MAX_LATENCY_UNKNOWN;
    metadata->update(ANDROID_SYNC_MAX_LATENCY, &maxLatency, 1);

    // Request max num output streams
    int32_t maxNumOutputStreams[] = {0, 2, 1}; // raw, processed, processed stalling
    metadata->update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, maxNumOutputStreams, 3);

    // Pipeline max depth
    uint8_t maxPipelineDepth = 4;
    metadata->update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH, &maxPipelineDepth, 1);

    // Partial result count
    int32_t partialResultCount = 1;
    metadata->update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &partialResultCount, 1);

    LOG(INFO) << "Built static metadata for camera " << device->getId();
}

const camera_metadata_t* CameraMetadataHelper::buildRequestTemplate(int type, CameraMetadata* staticMeta) {
    static CameraMetadata requestMeta;
    requestMeta.clear();

    // Control mode
    uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    requestMeta.update(ANDROID_CONTROL_MODE, &controlMode, 1);

    // AE mode
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    requestMeta.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    // AWB mode
    uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    requestMeta.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    // AF mode
    uint8_t afMode = ANDROID_CONTROL_AF_MODE_OFF;
    requestMeta.update(ANDROID_CONTROL_AF_MODE, &afMode, 1);

    // Capture intent based on template type
    uint8_t captureIntent;
    switch (type) {
        case CAMERA3_TEMPLATE_PREVIEW:
            captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
            break;
        case CAMERA3_TEMPLATE_STILL_CAPTURE:
            captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
            break;
        case CAMERA3_TEMPLATE_VIDEO_RECORD:
            captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
            break;
        case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
            captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
            break;
        default:
            captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
            break;
    }
    requestMeta.update(ANDROID_CONTROL_CAPTURE_INTENT, &captureIntent, 1);

    // JPEG quality
    uint8_t jpegQuality = 95;
    requestMeta.update(ANDROID_JPEG_QUALITY, &jpegQuality, 1);

    // JPEG thumbnail quality
    uint8_t thumbQuality = 85;
    requestMeta.update(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbQuality, 1);

    // JPEG thumbnail size
    int32_t thumbSize[] = {320, 240};
    requestMeta.update(ANDROID_JPEG_THUMBNAIL_SIZE, thumbSize, 2);

    return requestMeta.getAndLock();
}

}  // namespace camera
}  // namespace hardware
}  // namespace android
