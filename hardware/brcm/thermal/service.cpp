// Copyright (C) 2024 The Android Open Source Project
// Thermal HAL Service Entry Point

#define LOG_TAG "ThermalService"

#include <android-base/logging.h>
#include <android/hardware/thermal/2.0/IThermal.h>
#include <hidl/HidlTransportSupport.h>

using android::hardware::thermal::V2_0::IThermal;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::OK;

int main() {
    android::base::InitLogging(nullptr);
    LOG(INFO) << "Thermal HAL Service starting...";

    configureRpcThreadpool(1, true);

    sp<IThermal> thermal = IThermal::getService();
    if (thermal == nullptr) {
        LOG(ERROR) << "Failed to get Thermal HAL implementation";
        return 1;
    }

    android::status_t status = thermal->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Failed to register Thermal HAL service: " << status;
        return 1;
    }

    LOG(INFO) << "Thermal HAL Service started successfully";
    joinRpcThreadpool();

    LOG(ERROR) << "Thermal HAL Service exited unexpectedly";
    return 1;
}
