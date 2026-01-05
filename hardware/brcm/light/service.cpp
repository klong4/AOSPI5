// Copyright (C) 2024 The Android Open Source Project

#define LOG_TAG "LightService"

#include <android-base/logging.h>
#include <android/hardware/light/2.0/ILight.h>
#include <hidl/HidlTransportSupport.h>
#include "Light.h"

using android::hardware::light::V2_0::ILight;
using android::hardware::light::V2_0::implementation::Light;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::OK;

int main() {
    android::base::InitLogging(nullptr);
    LOG(INFO) << "Light HAL Service starting...";

    configureRpcThreadpool(1, true);

    sp<ILight> light = new Light();
    android::status_t status = light->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Failed to register Light HAL service: " << status;
        return 1;
    }

    LOG(INFO) << "Light HAL Service started successfully";
    joinRpcThreadpool();

    LOG(ERROR) << "Light HAL Service exited unexpectedly";
    return 1;
}
