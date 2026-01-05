// Copyright (C) 2024 The Android Open Source Project
// USB HAL implementation for Raspberry Pi 5

#define LOG_TAG "UsbHAL"

#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <android/hardware/usb/1.3/IUsb.h>
#include <android/hardware/usb/1.3/types.h>
#include <hidl/Status.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/inotify.h>
#include "Usb.h"

namespace android {
namespace hardware {
namespace usb {
namespace V1_3 {
namespace implementation {

// USB gadget configfs paths
static const char* USB_CONTROLLER_PATH = "/sys/class/udc";
static const char* CONFIGFS_PATH = "/config/usb_gadget/g1";
static const char* USB_STATE_PATH = "/sys/class/udc/*/state";
static const char* USB_DATA_ROLE_PATH = "/sys/class/typec/port0/data_role";
static const char* USB_POWER_ROLE_PATH = "/sys/class/typec/port0/power_role";

Usb::Usb() : mCallback(nullptr) {
    LOG(INFO) << "USB HAL initialized";

    // Find USB controller
    findUsbController();

    // Start monitoring thread
    pthread_create(&mPollThread, nullptr, pollThreadFunc, this);
}

Usb::~Usb() {
    LOG(INFO) << "USB HAL destroyed";
}

void Usb::findUsbController() {
    DIR* dir = opendir(USB_CONTROLLER_PATH);
    if (!dir) {
        LOG(ERROR) << "Cannot open " << USB_CONTROLLER_PATH;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        mUsbController = entry->d_name;
        LOG(INFO) << "Found USB controller: " << mUsbController;
        break;
    }

    closedir(dir);
}

void* Usb::pollThreadFunc(void* arg) {
    Usb* usb = static_cast<Usb*>(arg);
    usb->pollLoop();
    return nullptr;
}

void Usb::pollLoop() {
    int inotifyFd = inotify_init();
    if (inotifyFd < 0) {
        LOG(ERROR) << "Failed to init inotify";
        return;
    }

    std::string statePath = std::string("/sys/class/udc/") + mUsbController + "/state";
    int wd = inotify_add_watch(inotifyFd, statePath.c_str(), IN_MODIFY);

    char buffer[512];
    while (true) {
        int length = read(inotifyFd, buffer, sizeof(buffer));
        if (length < 0) {
            LOG(ERROR) << "inotify read error";
            break;
        }

        // Notify callback about USB state change
        if (mCallback != nullptr) {
            PortStatus status;
            queryPortStatus(&status);

            hidl_vec<PortStatus> statusVec;
            statusVec.resize(1);
            statusVec[0] = status;

            auto ret = mCallback->notifyPortStatusChange(statusVec, Status::SUCCESS);
            if (!ret.isOk()) {
                LOG(ERROR) << "Failed to notify port status change";
            }
        }
    }

    close(inotifyFd);
}

Return<void> Usb::switchRole(const hidl_string& portName,
                              const V1_0::PortRole& newRole) {
    LOG(INFO) << "switchRole: port=" << portName << " role=" << static_cast<int>(newRole.type);

    std::string path;
    std::string value;

    switch (newRole.type) {
        case PortRoleType::DATA_ROLE:
            path = USB_DATA_ROLE_PATH;
            value = (newRole.role == static_cast<uint32_t>(PortDataRole::HOST)) ? "host" : "device";
            break;
        case PortRoleType::POWER_ROLE:
            path = USB_POWER_ROLE_PATH;
            value = (newRole.role == static_cast<uint32_t>(PortPowerRole::SOURCE)) ? "source" : "sink";
            break;
        default:
            LOG(ERROR) << "Unknown role type";
            if (mCallback) {
                mCallback->notifyRoleSwitchStatus(portName, newRole, Status::ERROR);
            }
            return Void();
    }

    // Try to switch role
    bool success = android::base::WriteStringToFile(value, path);

    if (mCallback) {
        mCallback->notifyRoleSwitchStatus(portName, newRole,
            success ? Status::SUCCESS : Status::ERROR);
    }

    return Void();
}

Return<void> Usb::setCallback(const sp<V1_0::IUsbCallback>& callback) {
    LOG(INFO) << "setCallback";
    mCallback = callback;
    return Void();
}

Return<void> Usb::queryPortStatus() {
    LOG(INFO) << "queryPortStatus";

    PortStatus status;
    queryPortStatus(&status);

    if (mCallback) {
        hidl_vec<PortStatus> statusVec;
        statusVec.resize(1);
        statusVec[0] = status;
        mCallback->notifyPortStatusChange(statusVec, Status::SUCCESS);
    }

    return Void();
}

void Usb::queryPortStatus(PortStatus* status) {
    status->portName = "port0";
    status->currentDataRole = PortDataRole::DEVICE;
    status->currentPowerRole = PortPowerRole::SINK;
    status->currentMode = PortMode::UFP;
    status->canChangeMode = false;
    status->canChangeDataRole = false;
    status->canChangePowerRole = false;
    status->supportedModes = PortMode::UFP;

    // Check USB state
    std::string statePath = std::string("/sys/class/udc/") + mUsbController + "/state";
    std::string state;
    if (android::base::ReadFileToString(statePath, &state)) {
        state = android::base::Trim(state);
        if (state == "configured") {
            status->currentDataRole = PortDataRole::DEVICE;
        }
    }

    // Check for Type-C
    if (access(USB_DATA_ROLE_PATH, R_OK) == 0) {
        std::string dataRole;
        if (android::base::ReadFileToString(USB_DATA_ROLE_PATH, &dataRole)) {
            dataRole = android::base::Trim(dataRole);
            status->currentDataRole = (dataRole == "host") ?
                PortDataRole::HOST : PortDataRole::DEVICE;
            status->canChangeDataRole = true;
        }
    }

    if (access(USB_POWER_ROLE_PATH, R_OK) == 0) {
        std::string powerRole;
        if (android::base::ReadFileToString(USB_POWER_ROLE_PATH, &powerRole)) {
            powerRole = android::base::Trim(powerRole);
            status->currentPowerRole = (powerRole == "source") ?
                PortPowerRole::SOURCE : PortPowerRole::SINK;
            status->canChangePowerRole = true;
        }
    }
}

// V1_1 methods
Return<void> Usb::setCallback_1_1(const sp<V1_1::IUsbCallback>& callback) {
    setCallback(callback);
    return Void();
}

// V1_2 methods
Return<void> Usb::enableContaminantPresenceDetection(const hidl_string& portName, bool enable) {
    LOG(INFO) << "enableContaminantPresenceDetection: " << portName << " enable=" << enable;
    // Not supported on Pi 5
    return Void();
}

Return<void> Usb::enableContaminantPresenceProtection(const hidl_string& portName, bool enable) {
    LOG(INFO) << "enableContaminantPresenceProtection: " << portName << " enable=" << enable;
    // Not supported on Pi 5
    return Void();
}

// V1_3 methods
Return<bool> Usb::enableUsbDataSignal(bool enable) {
    LOG(INFO) << "enableUsbDataSignal: " << enable;
    
    // Control USB gadget
    std::string udcPath = std::string(CONFIGFS_PATH) + "/UDC";
    std::string value = enable ? mUsbController : "";
    
    return android::base::WriteStringToFile(value, udcPath);
}

}  // namespace implementation
}  // namespace V1_3
}  // namespace usb
}  // namespace hardware
}  // namespace android
