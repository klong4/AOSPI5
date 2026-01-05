/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * PCIe and Neural Processing Unit HAL Implementation for Raspberry Pi 5
 */

#include "Npu.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <regex>

#define LOG_TAG "NpuHAL"
#include <log/log.h>

namespace aidl::android::hardware::neuralnetworks::rpi5 {

// Helper to read sysfs file
static std::string readSysfs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string content;
    std::getline(file, content);
    return content;
}

// Helper to write sysfs file
static bool writeSysfs(const std::string& path, const std::string& value) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << value;
    return file.good();
}

// PCIe Manager Implementation
PcieManager& PcieManager::getInstance() {
    static PcieManager instance;
    return instance;
}

bool PcieManager::initialize() {
    if (mInitialized) {
        return true;
    }
    
    ALOGI("Initializing PCIe Manager for Raspberry Pi 5");
    
    // Enumerate PCIe devices
    mDevices = enumerateDevices();
    
    mInitialized = true;
    ALOGI("PCIe Manager initialized, found %zu devices", mDevices.size());
    return true;
}

void PcieManager::shutdown() {
    mDevices.clear();
    mInitialized = false;
    ALOGI("PCIe Manager shutdown complete");
}

std::vector<PcieDeviceInfo> PcieManager::enumerateDevices() {
    std::vector<PcieDeviceInfo> devices;
    
    // Scan /sys/bus/pci/devices
    DIR* dir = opendir("/sys/bus/pci/devices");
    if (!dir) {
        ALOGE("Failed to open /sys/bus/pci/devices");
        return devices;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name[0] == '.') continue;
        
        std::string basePath = "/sys/bus/pci/devices/" + name;
        
        PcieDeviceInfo info;
        info.sysfsPath = basePath;
        
        // Parse BDF from directory name (e.g., "0000:01:00.0")
        if (sscanf(name.c_str(), "%*x:%hhx:%hhx.%hhx",
                   &info.bus, &info.device, &info.function) != 3) {
            continue;
        }
        
        // Read vendor and device IDs
        std::string vendorStr = readSysfs(basePath + "/vendor");
        std::string deviceStr = readSysfs(basePath + "/device");
        
        if (vendorStr.empty() || deviceStr.empty()) continue;
        
        info.vendorId = std::stoul(vendorStr, nullptr, 16);
        info.deviceId = std::stoul(deviceStr, nullptr, 16);
        
        // Read subsystem IDs
        std::string subVendorStr = readSysfs(basePath + "/subsystem_vendor");
        std::string subDeviceStr = readSysfs(basePath + "/subsystem_device");
        if (!subVendorStr.empty()) {
            info.subsystemVendorId = std::stoul(subVendorStr, nullptr, 16);
        }
        if (!subDeviceStr.empty()) {
            info.subsystemDeviceId = std::stoul(subDeviceStr, nullptr, 16);
        }
        
        // Try to identify device
        auto key = std::make_pair(info.vendorId, info.deviceId);
        auto it = kKnownPcieDevices.find(key);
        if (it != kKnownPcieDevices.end()) {
            info.name = it->second;
        } else {
            info.name = "Unknown PCIe Device";
        }
        
        // Determine device type
        std::string classStr = readSysfs(basePath + "/class");
        if (!classStr.empty()) {
            uint32_t classCode = std::stoul(classStr, nullptr, 16);
            uint8_t baseClass = (classCode >> 16) & 0xFF;
            
            switch (baseClass) {
                case 0x01: // Mass storage
                    info.type = PcieDeviceType::NVME_SSD;
                    break;
                case 0x02: // Network
                    info.type = PcieDeviceType::ETHERNET_1G;
                    break;
                case 0x03: // Display
                    info.type = PcieDeviceType::VIDEO_CAPTURE;
                    break;
                case 0x0C: // Serial bus (USB)
                    info.type = PcieDeviceType::USB3_CONTROLLER;
                    break;
                case 0x12: // Processing accelerator
                    info.type = PcieDeviceType::NPU_ACCELERATOR;
                    info.isNpu = true;
                    break;
                default:
                    info.type = PcieDeviceType::GENERIC;
            }
        }
        
        // Check if it's a known NPU
        if (info.vendorId == 0x1AC1 || // Google
            info.vendorId == 0x1E60 || // Hailo
            info.vendorId == 0x1DB7) { // Kneron
            info.type = PcieDeviceType::NPU_ACCELERATOR;
            info.isNpu = true;
        }
        
        // Read driver
        char driverPath[256];
        snprintf(driverPath, sizeof(driverPath), "%s/driver", basePath.c_str());
        char driverLink[256];
        ssize_t len = readlink(driverPath, driverLink, sizeof(driverLink) - 1);
        if (len > 0) {
            driverLink[len] = '\0';
            info.driver = basename(driverLink);
        }
        
        // Read link info
        std::string linkSpeed = readSysfs(basePath + "/current_link_speed");
        std::string linkWidth = readSysfs(basePath + "/current_link_width");
        
        if (!linkSpeed.empty()) {
            // Parse "8.0 GT/s PCIe" format
            float speed;
            if (sscanf(linkSpeed.c_str(), "%f", &speed) == 1) {
                info.linkSpeed = static_cast<uint32_t>(speed);
            }
        }
        if (!linkWidth.empty()) {
            info.linkWidth = std::stoul(linkWidth);
        }
        
        // Determine manufacturer from vendor ID
        switch (info.vendorId) {
            case 0x1AC1: info.manufacturer = "Google"; break;
            case 0x1E60: info.manufacturer = "Hailo"; break;
            case 0x8086: info.manufacturer = "Intel"; break;
            case 0x1DB7: info.manufacturer = "Kneron"; break;
            case 0x144D: info.manufacturer = "Samsung"; break;
            case 0x10EC: info.manufacturer = "Realtek"; break;
            case 0x14E4: info.manufacturer = "Broadcom"; break;
            case 0x14C3: info.manufacturer = "MediaTek"; break;
            default: info.manufacturer = "Unknown";
        }
        
        ALOGI("Found PCIe device: %s (%04x:%04x) at %02x:%02x.%x",
              info.name.c_str(), info.vendorId, info.deviceId,
              info.bus, info.device, info.function);
        
        devices.push_back(info);
    }
    
    closedir(dir);
    return devices;
}

PcieDeviceInfo PcieManager::getDeviceInfo(uint8_t bus, uint8_t device, uint8_t function) {
    for (const auto& dev : mDevices) {
        if (dev.bus == bus && dev.device == device && dev.function == function) {
            return dev;
        }
    }
    return PcieDeviceInfo();
}

bool PcieManager::resetDevice(const PcieDeviceInfo& device) {
    std::string resetPath = device.sysfsPath + "/reset";
    return writeSysfs(resetPath, "1");
}

bool PcieManager::setPowerState(const PcieDeviceInfo& device, int state) {
    std::string powerPath = device.sysfsPath + "/power/control";
    return writeSysfs(powerPath, state == 0 ? "on" : "auto");
}

int PcieManager::getPowerState(const PcieDeviceInfo& device) {
    std::string powerPath = device.sysfsPath + "/power/runtime_status";
    std::string status = readSysfs(powerPath);
    if (status == "active") return 0;
    if (status == "suspended") return 3;
    return -1;
}

// NPU Manager Implementation
NpuManager& NpuManager::getInstance() {
    static NpuManager instance;
    return instance;
}

bool NpuManager::initialize() {
    if (mInitialized) {
        return true;
    }
    
    ALOGI("Initializing NPU Manager for Raspberry Pi 5");
    
    // Initialize PCIe manager first
    PcieManager::getInstance().initialize();
    
    // Detect NPUs
    if (!detectNpus()) {
        ALOGI("No NPU accelerators detected");
    }
    
    mInitialized = true;
    ALOGI("NPU Manager initialized, found %zu NPUs", mNpus.size());
    return true;
}

void NpuManager::shutdown() {
    // Close all open NPUs
    for (auto& pair : mNpus) {
        closeNpu(pair.first);
    }
    mNpus.clear();
    mLoadedModels.clear();
    mInitialized = false;
    ALOGI("NPU Manager shutdown complete");
}

std::vector<NpuDeviceInfo> NpuManager::getAvailableNpus() {
    std::vector<NpuDeviceInfo> npus;
    for (const auto& pair : mNpus) {
        npus.push_back(pair.second);
    }
    return npus;
}

bool NpuManager::detectNpus() {
    mNpus.clear();
    
    // Get PCIe devices
    auto pcieDevices = PcieManager::getInstance().enumerateDevices();
    
    for (const auto& pcieDev : pcieDevices) {
        if (!pcieDev.isNpu) continue;
        
        NpuDeviceInfo npuInfo;
        npuInfo.name = pcieDev.name;
        npuInfo.manufacturer = pcieDev.manufacturer;
        npuInfo.pcieInfo = pcieDev;
        
        // Determine NPU type
        if (pcieDev.vendorId == 0x1AC1) {
            npuInfo.type = NpuType::CORAL_TPU_PCIE;
            npuInfo.devicePath = "/dev/apex_0";
            detectCoralTpu();
        } else if (pcieDev.vendorId == 0x1E60) {
            switch (pcieDev.deviceId) {
                case 0x0001: npuInfo.type = NpuType::HAILO_8; break;
                case 0x0002: npuInfo.type = NpuType::HAILO_8L; break;
                case 0x0100: npuInfo.type = NpuType::HAILO_15H; break;
                case 0x0101: npuInfo.type = NpuType::HAILO_15M; break;
                case 0x0102: npuInfo.type = NpuType::HAILO_15L; break;
                default: npuInfo.type = NpuType::HAILO_8;
            }
            npuInfo.devicePath = "/dev/hailo0";
            detectHailo();
        } else if (pcieDev.vendorId == 0x1DB7) {
            switch (pcieDev.deviceId) {
                case 0x0520: npuInfo.type = NpuType::KNERON_KL520; break;
                case 0x0720: npuInfo.type = NpuType::KNERON_KL720; break;
                case 0x0730: npuInfo.type = NpuType::KNERON_KL730; break;
                default: npuInfo.type = NpuType::KNERON_KL720;
            }
            npuInfo.devicePath = "/dev/kneron0";
            detectKneron();
        } else if (pcieDev.vendorId == 0x8086 && pcieDev.deviceId == 0x6240) {
            npuInfo.type = NpuType::INTEL_MYRIAD_X;
            npuInfo.devicePath = "/dev/myriad0";
            detectIntelNcs();
        } else {
            npuInfo.type = NpuType::GENERIC_NPU;
        }
        
        // Get capabilities from database
        auto capIt = kNpuCapabilities.find(npuInfo.type);
        if (capIt != kNpuCapabilities.end()) {
            npuInfo.capabilities = capIt->second;
        }
        
        std::string npuId = "npu" + std::to_string(mNpus.size());
        mNpus[npuId] = npuInfo;
        
        ALOGI("Detected NPU: %s (%s) - %.1f TOPS INT8",
              npuInfo.name.c_str(), npuId.c_str(), npuInfo.capabilities.topsInt8);
    }
    
    // Also check for USB NPUs (Coral USB, Intel NCS2)
    detectUsbNpus();
    
    return !mNpus.empty();
}

void NpuManager::detectUsbNpus() {
    // Check for Google Coral USB Accelerator
    DIR* dir = opendir("/sys/bus/usb/devices");
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name[0] == '.') continue;
        
        std::string basePath = "/sys/bus/usb/devices/" + name;
        std::string vendorStr = readSysfs(basePath + "/idVendor");
        std::string productStr = readSysfs(basePath + "/idProduct");
        
        if (vendorStr.empty() || productStr.empty()) continue;
        
        uint16_t vendor = std::stoul(vendorStr, nullptr, 16);
        uint16_t product = std::stoul(productStr, nullptr, 16);
        
        // Google Coral USB (18d1:9302)
        if (vendor == 0x18D1 && product == 0x9302) {
            NpuDeviceInfo npuInfo;
            npuInfo.name = "Google Coral USB Accelerator";
            npuInfo.manufacturer = "Google";
            npuInfo.type = NpuType::CORAL_TPU_USB;
            npuInfo.devicePath = "/dev/apex_0";
            
            auto capIt = kNpuCapabilities.find(NpuType::CORAL_TPU_PCIE);
            if (capIt != kNpuCapabilities.end()) {
                npuInfo.capabilities = capIt->second;
            }
            
            std::string npuId = "npu" + std::to_string(mNpus.size());
            mNpus[npuId] = npuInfo;
            
            ALOGI("Detected USB NPU: %s", npuInfo.name.c_str());
        }
        
        // Intel Neural Compute Stick 2 (03e7:2485)
        if (vendor == 0x03E7 && product == 0x2485) {
            NpuDeviceInfo npuInfo;
            npuInfo.name = "Intel Neural Compute Stick 2";
            npuInfo.manufacturer = "Intel";
            npuInfo.type = NpuType::INTEL_NCS2;
            npuInfo.devicePath = "/dev/myriad0";
            
            auto capIt = kNpuCapabilities.find(NpuType::INTEL_NCS2);
            if (capIt != kNpuCapabilities.end()) {
                npuInfo.capabilities = capIt->second;
            }
            
            std::string npuId = "npu" + std::to_string(mNpus.size());
            mNpus[npuId] = npuInfo;
            
            ALOGI("Detected USB NPU: %s", npuInfo.name.c_str());
        }
    }
    
    closedir(dir);
}

NpuDeviceInfo NpuManager::getNpuInfo(const std::string& npuId) {
    auto it = mNpus.find(npuId);
    if (it != mNpus.end()) {
        return it->second;
    }
    return NpuDeviceInfo();
}

bool NpuManager::openNpu(const std::string& npuId) {
    auto it = mNpus.find(npuId);
    if (it == mNpus.end()) {
        ALOGE("NPU %s not found", npuId.c_str());
        return false;
    }
    
    // Initialize based on NPU type
    switch (it->second.type) {
        case NpuType::CORAL_TPU_PCIE:
        case NpuType::CORAL_TPU_USB:
            return initCoralTpu(npuId);
            
        case NpuType::HAILO_8:
        case NpuType::HAILO_8L:
        case NpuType::HAILO_15H:
        case NpuType::HAILO_15M:
        case NpuType::HAILO_15L:
            return initHailo(npuId);
            
        case NpuType::INTEL_NCS2:
        case NpuType::INTEL_MYRIAD_X:
            return initMyriad(npuId);
            
        default:
            ALOGI("Generic NPU initialization for %s", npuId.c_str());
            return true;
    }
}

bool NpuManager::closeNpu(const std::string& npuId) {
    // Unload all models
    auto modelIt = mLoadedModels.find(npuId);
    if (modelIt != mLoadedModels.end()) {
        modelIt->second.clear();
    }
    
    ALOGI("Closed NPU %s", npuId.c_str());
    return true;
}

bool NpuManager::isOpen(const std::string& npuId) {
    return mNpus.find(npuId) != mNpus.end();
}

std::string NpuManager::loadModel(const std::string& npuId, const std::string& modelPath) {
    auto it = mNpus.find(npuId);
    if (it == mNpus.end()) {
        ALOGE("NPU %s not found", npuId.c_str());
        return "";
    }
    
    // Check if model file exists
    struct stat st;
    if (stat(modelPath.c_str(), &st) != 0) {
        ALOGE("Model file not found: %s", modelPath.c_str());
        return "";
    }
    
    ModelInfo model;
    model.path = modelPath;
    model.sizeBytes = st.st_size;
    
    // Determine format from extension
    size_t dotPos = modelPath.rfind('.');
    if (dotPos != std::string::npos) {
        std::string ext = modelPath.substr(dotPos + 1);
        if (ext == "tflite") {
            model.format = "TensorFlow Lite";
        } else if (ext == "onnx") {
            model.format = "ONNX";
        } else if (ext == "hef") {
            model.format = "Hailo HEF";
        } else if (ext == "xml" || ext == "bin") {
            model.format = "OpenVINO IR";
        } else {
            model.format = "Unknown";
        }
    }
    
    // Extract model name from path
    size_t slashPos = modelPath.rfind('/');
    model.name = (slashPos != std::string::npos) ?
                 modelPath.substr(slashPos + 1) : modelPath;
    
    // Generate model ID
    std::string modelId = "model_" + std::to_string(
        std::hash<std::string>{}(modelPath) & 0xFFFF);
    
    mLoadedModels[npuId][modelId] = model;
    
    ALOGI("Loaded model %s on NPU %s (format: %s, size: %zu bytes)",
          model.name.c_str(), npuId.c_str(), model.format.c_str(), model.sizeBytes);
    
    return modelId;
}

bool NpuManager::unloadModel(const std::string& npuId, const std::string& modelId) {
    auto npuIt = mLoadedModels.find(npuId);
    if (npuIt == mLoadedModels.end()) {
        return false;
    }
    
    auto modelIt = npuIt->second.find(modelId);
    if (modelIt == npuIt->second.end()) {
        return false;
    }
    
    npuIt->second.erase(modelIt);
    ALOGI("Unloaded model %s from NPU %s", modelId.c_str(), npuId.c_str());
    return true;
}

ModelInfo NpuManager::getModelInfo(const std::string& npuId, const std::string& modelId) {
    auto npuIt = mLoadedModels.find(npuId);
    if (npuIt != mLoadedModels.end()) {
        auto modelIt = npuIt->second.find(modelId);
        if (modelIt != npuIt->second.end()) {
            return modelIt->second;
        }
    }
    return ModelInfo();
}

std::vector<std::string> NpuManager::getLoadedModels(const std::string& npuId) {
    std::vector<std::string> models;
    auto npuIt = mLoadedModels.find(npuId);
    if (npuIt != mLoadedModels.end()) {
        for (const auto& pair : npuIt->second) {
            models.push_back(pair.first);
        }
    }
    return models;
}

InferenceResult NpuManager::runInference(const std::string& npuId,
                                         const InferenceRequest& request) {
    InferenceResult result;
    result.success = false;
    
    auto npuIt = mNpus.find(npuId);
    if (npuIt == mNpus.end()) {
        result.error = "NPU not found";
        return result;
    }
    
    auto modelMapIt = mLoadedModels.find(npuId);
    if (modelMapIt == mLoadedModels.end()) {
        result.error = "No models loaded on NPU";
        return result;
    }
    
    auto modelIt = modelMapIt->second.find(request.modelId);
    if (modelIt == modelMapIt->second.end()) {
        result.error = "Model not found";
        return result;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Placeholder for actual inference
    // In real implementation, this would call the specific NPU runtime
    // (libedgetpu for Coral, HailoRT for Hailo, OpenVINO for Intel, etc.)
    
    ALOGI("Running inference on NPU %s with model %s",
          npuId.c_str(), request.modelId.c_str());
    
    // Simulate inference time based on NPU type
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.inferenceTimeMs = std::chrono::duration<float, std::milli>(
        endTime - startTime).count();
    
    result.success = true;
    return result;
}

bool NpuManager::runInferenceAsync(const std::string& npuId,
                                   const InferenceRequest& request,
                                   InferenceCallback callback) {
    std::thread([this, npuId, request, callback]() {
        InferenceResult result = runInference(npuId, request);
        if (callback) {
            callback(result);
        }
    }).detach();
    
    return true;
}

float NpuManager::getTemperature(const std::string& npuId) {
    auto it = mNpus.find(npuId);
    if (it == mNpus.end()) return -1.0f;
    
    // Try to read temperature from sysfs
    std::string tempPath = it->second.pcieInfo.sysfsPath + "/hwmon/hwmon0/temp1_input";
    std::string tempStr = readSysfs(tempPath);
    if (!tempStr.empty()) {
        return std::stof(tempStr) / 1000.0f;
    }
    
    return it->second.temperatureCelsius;
}

float NpuManager::getPowerConsumption(const std::string& npuId) {
    auto it = mNpus.find(npuId);
    if (it == mNpus.end()) return -1.0f;
    return it->second.powerWatts;
}

float NpuManager::getUtilization(const std::string& npuId) {
    auto it = mNpus.find(npuId);
    if (it == mNpus.end()) return -1.0f;
    return it->second.utilizationPercent;
}

// Coral TPU specific
bool NpuManager::initCoralTpu(const std::string& npuId) {
    ALOGI("Initializing Coral TPU for %s", npuId.c_str());
    
    // Check for apex device
    int fd = open("/dev/apex_0", O_RDWR);
    if (fd < 0) {
        ALOGE("Failed to open Coral TPU device: %s", strerror(errno));
        return false;
    }
    close(fd);
    
    return true;
}

std::string NpuManager::getCoralTpuVersion(const std::string& npuId) {
    return "Coral Edge TPU v1.0";
}

bool NpuManager::detectCoralTpu() {
    return access("/dev/apex_0", F_OK) == 0;
}

// Hailo specific
bool NpuManager::initHailo(const std::string& npuId) {
    ALOGI("Initializing Hailo NPU for %s", npuId.c_str());
    
    // Check for hailo device
    int fd = open("/dev/hailo0", O_RDWR);
    if (fd < 0) {
        ALOGE("Failed to open Hailo device: %s", strerror(errno));
        return false;
    }
    close(fd);
    
    return true;
}

std::string NpuManager::getHailoVersion(const std::string& npuId) {
    // Read firmware version from sysfs
    auto it = mNpus.find(npuId);
    if (it != mNpus.end()) {
        std::string fwPath = it->second.pcieInfo.sysfsPath + "/firmware_version";
        std::string version = readSysfs(fwPath);
        if (!version.empty()) {
            return version;
        }
    }
    return "Unknown";
}

bool NpuManager::configureHailoDataflow(const std::string& npuId, const std::string& hef) {
    ALOGI("Configuring Hailo dataflow with HEF: %s", hef.c_str());
    // In real implementation, this would use HailoRT API
    return true;
}

bool NpuManager::detectHailo() {
    return access("/dev/hailo0", F_OK) == 0;
}

// Intel NCS/Myriad specific
bool NpuManager::initMyriad(const std::string& npuId) {
    ALOGI("Initializing Intel Myriad for %s", npuId.c_str());
    // In real implementation, this would use OpenVINO API
    return true;
}

bool NpuManager::detectIntelNcs() {
    // Check for Intel NCS via USB or PCIe
    return false;
}

bool NpuManager::detectKneron() {
    return access("/dev/kneron0", F_OK) == 0;
}

}  // namespace aidl::android::hardware::neuralnetworks::rpi5
