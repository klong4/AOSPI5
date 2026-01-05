// Copyright (C) 2024 The Android Open Source Project
// Power HAL Service Entry Point

#define LOG_TAG "PowerService"

#include <android-base/logging.h>
#include <android/hardware/power/1.3/IPower.h>
#include <hidl/HidlTransportSupport.h>

using android::hardware::power::V1_3::IPower;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::OK;

int main() {
    android::base::InitLogging(nullptr);
    LOG(INFO) << "Power HAL Service starting...";

    configureRpcThreadpool(1, true);

    sp<IPower> power = IPower::getService();
    if (power == nullptr) {
        LOG(ERROR) << "Failed to get Power HAL implementation";
        return 1;
    }

    android::status_t status = power->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Failed to register Power HAL service: " << status;
        return 1;
    }

    LOG(INFO) << "Power HAL Service started successfully";
    joinRpcThreadpool();

    LOG(ERROR) << "Power HAL Service exited unexpectedly";
    return 1;
}
