/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * PCIe and Neural Processing Unit HAL for Raspberry Pi 5
 * Supports various NPU accelerators connected via PCIe
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

namespace aidl::android::hardware::neuralnetworks::rpi5 {

// Neural accelerator types
enum class NpuType {
    // Google Coral TPU
    CORAL_TPU_USB,          // Coral USB Accelerator
    CORAL_TPU_PCIE,         // Coral M.2/PCIe Accelerator
    CORAL_TPU_MINI_PCIE,    // Coral Mini PCIe Accelerator
    CORAL_TPU_DUAL_EDGE,    // Coral Dual Edge TPU
    
    // Intel/Movidius
    INTEL_NCS2,             // Neural Compute Stick 2 (Myriad X)
    INTEL_MYRIAD_X,         // Myriad X VPU
    
    // Hailo
    HAILO_8,                // Hailo-8 (26 TOPS)
    HAILO_8L,               // Hailo-8L (13 TOPS)
    HAILO_15H,              // Hailo-15H (20 TOPS)
    HAILO_15M,              // Hailo-15M (11 TOPS)
    HAILO_15L,              // Hailo-15L (6 TOPS)
    
    // Rockchip NPU
    ROCKCHIP_NPU,           // RK3588 NPU (via adapter)
    
    // Amlogic NPU
    AMLOGIC_NPU,            // A311D NPU (via adapter)
    
    // NVIDIA
    NVIDIA_JETSON_NANO,     // Jetson Nano (via carrier)
    
    // Kneron
    KNERON_KL520,           // KL520 NPU
    KNERON_KL720,           // KL720 NPU
    KNERON_KL730,           // KL730 NPU
    
    // Blaize
    BLAIZE_PATHFINDER,      // Pathfinder P1600
    
    // MemryX
    MEMRYX_MX3,             // MX3 NPU
    
    // SiMa.ai
    SIMA_MLSoC,             // MLSoC
    
    // Syntiant
    SYNTIANT_NDP120,        // NDP120
    SYNTIANT_NDP200,        // NDP200
    
    // Generic PCIe AI Accelerator
    GENERIC_NPU,
    UNKNOWN
};

// PCIe device types
enum class PcieDeviceType {
    // Storage
    NVME_SSD,
    SATA_CONTROLLER,
    
    // Network
    ETHERNET_1G,
    ETHERNET_2_5G,
    ETHERNET_5G,
    ETHERNET_10G,
    WIFI_6,
    WIFI_6E,
    WIFI_7,
    
    // AI/ML Accelerators
    NPU_ACCELERATOR,
    
    // USB Controllers
    USB3_CONTROLLER,
    USB4_CONTROLLER,
    
    // Video
    VIDEO_CAPTURE,
    
    // Other
    FPGA,
    GENERIC,
    UNKNOWN
};

// PCIe device info
struct PcieDeviceInfo {
    std::string name;
    std::string manufacturer;
    PcieDeviceType type;
    uint16_t vendorId;
    uint16_t deviceId;
    uint16_t subsystemVendorId;
    uint16_t subsystemDeviceId;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    std::string driver;
    std::string sysfsPath;
    uint32_t linkSpeed;     // GT/s
    uint32_t linkWidth;     // lanes
    size_t memorySize;
    bool isNpu;
};

// NPU capabilities
struct NpuCapabilities {
    float topsInt8;         // TOPS for INT8
    float topsInt16;        // TOPS for INT16
    float topsFp16;         // TOPS for FP16
    float topsFp32;         // TOPS for FP32
    size_t memoryMB;
    bool supportsInt4;
    bool supportsInt8;
    bool supportsInt16;
    bool supportsFp16;
    bool supportsBf16;
    bool supportsFp32;
    bool supportsDynamic;   // Dynamic shapes
    bool supportsNHWC;
    bool supportsNCHW;
    std::vector<std::string> supportedOperations;
    std::vector<std::string> supportedFrameworks;
};

// NPU device info
struct NpuDeviceInfo {
    std::string name;
    std::string manufacturer;
    NpuType type;
    std::string serial;
    std::string firmware;
    NpuCapabilities capabilities;
    PcieDeviceInfo pcieInfo;
    std::string devicePath;
    float temperatureCelsius;
    float powerWatts;
    float utilizationPercent;
};

// Model info for inference
struct ModelInfo {
    std::string name;
    std::string path;
    std::string format;     // TFLite, ONNX, OpenVINO, etc.
    size_t sizeBytes;
    std::vector<std::pair<std::string, std::vector<int>>> inputs;
    std::vector<std::pair<std::string, std::vector<int>>> outputs;
};

// Inference request
struct InferenceRequest {
    std::string modelId;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> inputs;
    bool measureTiming;
};

// Inference result
struct InferenceResult {
    bool success;
    std::string error;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> outputs;
    float inferenceTimeMs;
    float preprocessTimeMs;
    float postprocessTimeMs;
};

// Callback types
using InferenceCallback = std::function<void(const InferenceResult&)>;

// PCIe Manager class
class PcieManager {
public:
    static PcieManager& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Device enumeration
    std::vector<PcieDeviceInfo> enumerateDevices();
    PcieDeviceInfo getDeviceInfo(uint8_t bus, uint8_t device, uint8_t function);
    
    // Link management
    bool resetDevice(const PcieDeviceInfo& device);
    bool setLinkSpeed(const PcieDeviceInfo& device, uint32_t speedGTs);
    
    // Power management
    bool setPowerState(const PcieDeviceInfo& device, int state);
    int getPowerState(const PcieDeviceInfo& device);
    
private:
    PcieManager() = default;
    bool mInitialized = false;
    std::vector<PcieDeviceInfo> mDevices;
};

// Neural Processing Unit Manager class
class NpuManager {
public:
    static NpuManager& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // NPU enumeration
    std::vector<NpuDeviceInfo> getAvailableNpus();
    bool detectNpus();
    NpuDeviceInfo getNpuInfo(const std::string& npuId);
    
    // NPU operations
    bool openNpu(const std::string& npuId);
    bool closeNpu(const std::string& npuId);
    bool isOpen(const std::string& npuId);
    
    // Model management
    std::string loadModel(const std::string& npuId, const std::string& modelPath);
    bool unloadModel(const std::string& npuId, const std::string& modelId);
    ModelInfo getModelInfo(const std::string& npuId, const std::string& modelId);
    std::vector<std::string> getLoadedModels(const std::string& npuId);
    
    // Inference
    InferenceResult runInference(const std::string& npuId, const InferenceRequest& request);
    bool runInferenceAsync(const std::string& npuId, const InferenceRequest& request,
                          InferenceCallback callback);
    
    // Monitoring
    float getTemperature(const std::string& npuId);
    float getPowerConsumption(const std::string& npuId);
    float getUtilization(const std::string& npuId);
    
    // Coral TPU specific
    bool initCoralTpu(const std::string& npuId);
    std::string getCoralTpuVersion(const std::string& npuId);
    
    // Hailo specific
    bool initHailo(const std::string& npuId);
    std::string getHailoVersion(const std::string& npuId);
    bool configureHailoDataflow(const std::string& npuId, const std::string& hef);
    
    // Intel NCS/Myriad specific
    bool initMyriad(const std::string& npuId);
    
private:
    NpuManager() = default;
    ~NpuManager() = default;
    NpuManager(const NpuManager&) = delete;
    NpuManager& operator=(const NpuManager&) = delete;
    
    bool detectCoralTpu();
    bool detectHailo();
    bool detectIntelNcs();
    bool detectKneron();
    
    std::map<std::string, NpuDeviceInfo> mNpus;
    std::map<std::string, std::map<std::string, ModelInfo>> mLoadedModels;
    bool mInitialized = false;
};

// Known PCIe Vendor/Device IDs
static const std::map<std::pair<uint16_t, uint16_t>, std::string> kKnownPcieDevices = {
    // Google Coral
    {{0x1AC1, 0x089A}, "Google Coral Edge TPU"},
    
    // Hailo
    {{0x1E60, 0x0001}, "Hailo-8"},
    {{0x1E60, 0x0002}, "Hailo-8L"},
    {{0x1E60, 0x0100}, "Hailo-15H"},
    {{0x1E60, 0x0101}, "Hailo-15M"},
    {{0x1E60, 0x0102}, "Hailo-15L"},
    
    // Intel
    {{0x8086, 0x6240}, "Intel Movidius Myriad X"},
    
    // Kneron
    {{0x1DB7, 0x0520}, "Kneron KL520"},
    {{0x1DB7, 0x0720}, "Kneron KL720"},
    {{0x1DB7, 0x0730}, "Kneron KL730"},
    
    // NVMe (common controllers)
    {{0x144D, 0xA808}, "Samsung NVMe SSD"},
    {{0x144D, 0xA809}, "Samsung 980 PRO"},
    {{0x1C5C, 0x174A}, "SK Hynix NVMe"},
    {{0x15B7, 0x5006}, "Sandisk NVMe"},
    {{0x1987, 0x5012}, "Phison NVMe"},
    {{0x1CC1, 0x8201}, "ADATA NVMe"},
    {{0x126F, 0x2263}, "Silicon Motion NVMe"},
    {{0x1E0F, 0x0001}, "KIOXIA NVMe"},
    {{0x8086, 0xF1A8}, "Intel NVMe"},
    {{0x1B4B, 0x1092}, "Marvell NVMe"},
    {{0x1179, 0x011A}, "Toshiba NVMe"},
    {{0x2646, 0x500F}, "Kingston NVMe"},
    {{0x1D97, 0x1160}, "Shenzhen NVMe"},
    {{0x1E4B, 0x1202}, "Maxio NVMe"},
    
    // Network
    {{0x10EC, 0x8168}, "Realtek RTL8168 Gigabit"},
    {{0x10EC, 0x8125}, "Realtek RTL8125 2.5G"},
    {{0x8086, 0x15F3}, "Intel I225-V 2.5G"},
    {{0x8086, 0x15E3}, "Intel I219-V Gigabit"},
    {{0x14E4, 0x1682}, "Broadcom BCM5762 Gigabit"},
    {{0x14C3, 0x7961}, "MediaTek MT7921 WiFi 6"},
    {{0x14C3, 0x0608}, "MediaTek MT7921E WiFi 6E"},
    {{0x8086, 0x2725}, "Intel AX210 WiFi 6E"},
    {{0x8086, 0x7AF0}, "Intel AX211 WiFi 6E"},
    {{0x10EC, 0xC852}, "Realtek RTL8852 WiFi 6"},
    {{0x14E4, 0x4433}, "Broadcom BCM4377 WiFi 6E"},
    {{0x17CB, 0x1103}, "Qualcomm WCN785x WiFi 7"},
    
    // USB Controllers
    {{0x1912, 0x0014}, "Renesas uPD720201 USB 3.0"},
    {{0x1912, 0x0015}, "Renesas uPD720202 USB 3.0"},
    {{0x1B73, 0x1100}, "Fresco Logic FL1100 USB 3.0"},
    {{0x1B21, 0x2142}, "ASMedia ASM2142 USB 3.1"},
    {{0x1B21, 0x3242}, "ASMedia ASM3242 USB 3.2"},
    {{0x8086, 0x9A1B}, "Intel USB4/Thunderbolt"},
};

// NPU Capabilities Database
static const std::map<NpuType, NpuCapabilities> kNpuCapabilities = {
    {NpuType::CORAL_TPU_PCIE, {
        4.0f, 2.0f, 0.0f, 0.0f, 8,
        false, true, false, false, false, false,
        true, true, true,
        {"Conv2D", "DepthwiseConv2D", "FullyConnected", "Pooling", "Softmax", "Add", "Mul"},
        {"TensorFlow Lite", "Edge TPU"}
    }},
    {NpuType::HAILO_8, {
        26.0f, 13.0f, 6.5f, 0.0f, 32,
        true, true, true, true, false, false,
        true, true, true,
        {"Conv2D", "DepthwiseConv2D", "FullyConnected", "Pooling", "BatchNorm", "ReLU", "Sigmoid", "Softmax", "Add", "Concat", "Split", "Reshape", "Transpose"},
        {"Hailo Model Zoo", "TensorFlow", "PyTorch", "ONNX", "TensorFlow Lite"}
    }},
    {NpuType::HAILO_8L, {
        13.0f, 6.5f, 3.25f, 0.0f, 16,
        true, true, true, true, false, false,
        true, true, true,
        {"Conv2D", "DepthwiseConv2D", "FullyConnected", "Pooling", "BatchNorm", "ReLU", "Sigmoid", "Softmax", "Add", "Concat"},
        {"Hailo Model Zoo", "TensorFlow", "PyTorch", "ONNX", "TensorFlow Lite"}
    }},
    {NpuType::INTEL_NCS2, {
        1.0f, 0.5f, 0.25f, 0.0f, 4,
        false, true, true, true, false, false,
        true, true, true,
        {"Conv2D", "DepthwiseConv2D", "FullyConnected", "Pooling", "BatchNorm", "ReLU", "PReLU", "Softmax"},
        {"OpenVINO", "TensorFlow", "Caffe", "ONNX"}
    }},
    {NpuType::KNERON_KL720, {
        1.5f, 0.75f, 0.4f, 0.0f, 2,
        false, true, true, true, false, false,
        true, true, true,
        {"Conv2D", "DepthwiseConv2D", "FullyConnected", "Pooling", "BatchNorm", "ReLU", "Softmax"},
        {"Kneron Toolchain", "ONNX", "TensorFlow Lite"}
    }},
};

}  // namespace aidl::android::hardware::neuralnetworks::rpi5
