/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ThermalHAL_RPi5"

#include <android-base/logging.h>
#include <android/hardware/thermal/2.0/IThermal.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <fstream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;

// Thermal zones for Raspberry Pi 5
constexpr char THERMAL_ZONE_CPU[] = "/sys/class/thermal/thermal_zone0/temp";
constexpr char THERMAL_ZONE_GPU[] = "/sys/class/thermal/thermal_zone1/temp";  // If available

// Temperature thresholds in millidegrees Celsius
constexpr float TEMP_THROTTLE_LIGHT = 70.0f;    // 70°C - light throttling
constexpr float TEMP_THROTTLE_MODERATE = 80.0f; // 80°C - moderate throttling  
constexpr float TEMP_THROTTLE_SEVERE = 85.0f;   // 85°C - severe throttling
constexpr float TEMP_SHUTDOWN = 90.0f;          // 90°C - shutdown threshold

// Cooling device paths
constexpr char FAN_PWM[] = "/sys/class/hwmon/hwmon0/pwm1";  // Official Pi 5 cooler
constexpr char FAN_ENABLE[] = "/sys/class/hwmon/hwmon0/pwm1_enable";

class Thermal : public IThermal {
public:
    Thermal();
    ~Thermal();

    // V1_0
    Return<void> getTemperatures(getTemperatures_cb _hidl_cb) override;
    Return<void> getCpuUsages(getCpuUsages_cb _hidl_cb) override;
    Return<void> getCoolingDevices(getCoolingDevices_cb _hidl_cb) override;

    // V2_0
    Return<void> getCurrentTemperatures(bool filterType, TemperatureType type,
                                        getCurrentTemperatures_cb _hidl_cb) override;
    Return<void> getTemperatureThresholds(bool filterType, TemperatureType type,
                                          getTemperatureThresholds_cb _hidl_cb) override;
    Return<void> registerThermalChangedCallback(const sp<IThermalChangedCallback>& callback,
                                                bool filterType, TemperatureType type,
                                                registerThermalChangedCallback_cb _hidl_cb) override;
    Return<void> unregisterThermalChangedCallback(const sp<IThermalChangedCallback>& callback,
                                                  unregisterThermalChangedCallback_cb _hidl_cb) override;
    Return<void> getCurrentCoolingDevices(bool filterType, CoolingType type,
                                          getCurrentCoolingDevices_cb _hidl_cb) override;

private:
    float readTemperature(const char* path);
    void setFanSpeed(int speed);
    void startThermalMonitor();
    void stopThermalMonitor();
    void thermalMonitorLoop();
    ThrottlingSeverity getSeverity(float temp);

    std::mutex mMutex;
    std::mutex mCallbackMutex;
    std::vector<sp<IThermalChangedCallback>> mCallbacks;
    std::thread mMonitorThread;
    bool mMonitorRunning;
    ThrottlingSeverity mCurrentSeverity;
    float mLastTemperature;
};

Thermal::Thermal() : mMonitorRunning(false), mCurrentSeverity(ThrottlingSeverity::NONE), 
                     mLastTemperature(0.0f) {
    LOG(INFO) << "Thermal HAL initialized for Raspberry Pi 5";
    startThermalMonitor();
}

Thermal::~Thermal() {
    stopThermalMonitor();
}

float Thermal::readTemperature(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return 0.0f;
    }
    
    int tempMilliC;
    file >> tempMilliC;
    return tempMilliC / 1000.0f;  // Convert from millidegrees to degrees
}

void Thermal::setFanSpeed(int speed) {
    // Enable manual fan control
    std::ofstream enableFile(FAN_ENABLE);
    if (enableFile.is_open()) {
        enableFile << "1";  // Manual mode
        enableFile.close();
    }
    
    // Set fan PWM (0-255)
    std::ofstream pwmFile(FAN_PWM);
    if (pwmFile.is_open()) {
        pwmFile << std::to_string(speed);
    }
}

ThrottlingSeverity Thermal::getSeverity(float temp) {
    if (temp >= TEMP_SHUTDOWN) {
        return ThrottlingSeverity::SHUTDOWN;
    } else if (temp >= TEMP_THROTTLE_SEVERE) {
        return ThrottlingSeverity::SEVERE;
    } else if (temp >= TEMP_THROTTLE_MODERATE) {
        return ThrottlingSeverity::MODERATE;
    } else if (temp >= TEMP_THROTTLE_LIGHT) {
        return ThrottlingSeverity::LIGHT;
    }
    return ThrottlingSeverity::NONE;
}

void Thermal::startThermalMonitor() {
    mMonitorRunning = true;
    mMonitorThread = std::thread(&Thermal::thermalMonitorLoop, this);
}

void Thermal::stopThermalMonitor() {
    mMonitorRunning = false;
    if (mMonitorThread.joinable()) {
        mMonitorThread.join();
    }
}

void Thermal::thermalMonitorLoop() {
    while (mMonitorRunning) {
        float temp = readTemperature(THERMAL_ZONE_CPU);
        ThrottlingSeverity severity = getSeverity(temp);
        
        // Adjust fan speed based on temperature
        int fanSpeed = 0;
        if (temp >= TEMP_THROTTLE_SEVERE) {
            fanSpeed = 255;  // Full speed
        } else if (temp >= TEMP_THROTTLE_MODERATE) {
            fanSpeed = 192;  // 75%
        } else if (temp >= TEMP_THROTTLE_LIGHT) {
            fanSpeed = 128;  // 50%
        } else if (temp >= 50.0f) {
            fanSpeed = 64;   // 25%
        }
        setFanSpeed(fanSpeed);
        
        // Notify callbacks if severity changed
        if (severity != mCurrentSeverity || std::abs(temp - mLastTemperature) > 2.0f) {
            mCurrentSeverity = severity;
            mLastTemperature = temp;
            
            Temperature temperature;
            temperature.type = TemperatureType::CPU;
            temperature.name = "CPU";
            temperature.value = temp;
            temperature.throttlingStatus = severity;
            
            std::lock_guard<std::mutex> lock(mCallbackMutex);
            for (const auto& callback : mCallbacks) {
                if (callback != nullptr) {
                    callback->notifyThrottling(temperature);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

Return<void> Thermal::getTemperatures(getTemperatures_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    
    hidl_vec<V1_0::Temperature> temperatures;
    temperatures.resize(2);
    
    // CPU temperature
    temperatures[0].type = V1_0::TemperatureType::CPU;
    temperatures[0].name = "CPU";
    temperatures[0].currentValue = readTemperature(THERMAL_ZONE_CPU);
    temperatures[0].throttlingThreshold = TEMP_THROTTLE_LIGHT;
    temperatures[0].shutdownThreshold = TEMP_SHUTDOWN;
    temperatures[0].vrThrottlingThreshold = TEMP_THROTTLE_MODERATE;
    
    // GPU temperature (may be same sensor on Pi 5)
    temperatures[1].type = V1_0::TemperatureType::GPU;
    temperatures[1].name = "GPU";
    temperatures[1].currentValue = readTemperature(THERMAL_ZONE_GPU);
    if (temperatures[1].currentValue == 0.0f) {
        temperatures[1].currentValue = temperatures[0].currentValue;  // Use CPU temp as fallback
    }
    temperatures[1].throttlingThreshold = TEMP_THROTTLE_LIGHT;
    temperatures[1].shutdownThreshold = TEMP_SHUTDOWN;
    temperatures[1].vrThrottlingThreshold = TEMP_THROTTLE_MODERATE;
    
    _hidl_cb(status, temperatures);
    return Void();
}

Return<void> Thermal::getCpuUsages(getCpuUsages_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    
    hidl_vec<CpuUsage> cpuUsages;
    cpuUsages.resize(4);  // Raspberry Pi 5 has 4 cores
    
    std::ifstream statFile("/proc/stat");
    std::string line;
    int cpuIndex = 0;
    
    while (std::getline(statFile, line) && cpuIndex < 4) {
        if (line.substr(0, 3) == "cpu" && line[3] != ' ') {
            long user, nice, system, idle, iowait, irq, softirq;
            sscanf(line.c_str(), "cpu%*d %ld %ld %ld %ld %ld %ld %ld",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq);
            
            cpuUsages[cpuIndex].name = "cpu" + std::to_string(cpuIndex);
            cpuUsages[cpuIndex].active = user + nice + system + irq + softirq;
            cpuUsages[cpuIndex].total = cpuUsages[cpuIndex].active + idle + iowait;
            cpuUsages[cpuIndex].isOnline = true;
            
            cpuIndex++;
        }
    }
    
    _hidl_cb(status, cpuUsages);
    return Void();
}

Return<void> Thermal::getCoolingDevices(getCoolingDevices_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    
    hidl_vec<V1_0::CoolingDevice> coolingDevices;
    coolingDevices.resize(1);
    
    coolingDevices[0].type = V1_0::CoolingType::FAN;
    coolingDevices[0].name = "Pi 5 Cooler";
    
    // Read current fan speed
    std::ifstream pwmFile(FAN_PWM);
    int pwmValue = 0;
    if (pwmFile.is_open()) {
        pwmFile >> pwmValue;
    }
    coolingDevices[0].currentValue = (pwmValue * 100.0f) / 255.0f;  // Convert to percentage
    
    _hidl_cb(status, coolingDevices);
    return Void();
}

Return<void> Thermal::getCurrentTemperatures(bool filterType, TemperatureType type,
                                             getCurrentTemperatures_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    
    std::vector<Temperature> temps;
    
    if (!filterType || type == TemperatureType::CPU) {
        Temperature temp;
        temp.type = TemperatureType::CPU;
        temp.name = "CPU";
        temp.value = readTemperature(THERMAL_ZONE_CPU);
        temp.throttlingStatus = getSeverity(temp.value);
        temps.push_back(temp);
    }
    
    if (!filterType || type == TemperatureType::GPU) {
        Temperature temp;
        temp.type = TemperatureType::GPU;
        temp.name = "GPU";
        temp.value = readTemperature(THERMAL_ZONE_GPU);
        if (temp.value == 0.0f) {
            temp.value = readTemperature(THERMAL_ZONE_CPU);
        }
        temp.throttlingStatus = getSeverity(temp.value);
        temps.push_back(temp);
    }
    
    hidl_vec<Temperature> temperatures(temps);
    _hidl_cb(status, temperatures);
    return Void();
}

Return<void> Thermal::getTemperatureThresholds(bool filterType, TemperatureType type,
                                               getTemperatureThresholds_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    
    std::vector<TemperatureThreshold> thresholds;
    
    auto addThreshold = [&](TemperatureType t, const std::string& name) {
        TemperatureThreshold threshold;
        threshold.type = t;
        threshold.name = name;
        threshold.hotThrottlingThresholds = {
            0.0f,                    // NONE
            TEMP_THROTTLE_LIGHT,     // LIGHT
            TEMP_THROTTLE_MODERATE,  // MODERATE
            TEMP_THROTTLE_SEVERE,    // SEVERE
            TEMP_THROTTLE_SEVERE,    // CRITICAL
            TEMP_THROTTLE_SEVERE,    // EMERGENCY
            TEMP_SHUTDOWN            // SHUTDOWN
        };
        threshold.coldThrottlingThresholds = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        threshold.vrThrottlingThreshold = TEMP_THROTTLE_MODERATE;
        thresholds.push_back(threshold);
    };
    
    if (!filterType || type == TemperatureType::CPU) {
        addThreshold(TemperatureType::CPU, "CPU");
    }
    if (!filterType || type == TemperatureType::GPU) {
        addThreshold(TemperatureType::GPU, "GPU");
    }
    
    hidl_vec<TemperatureThreshold> result(thresholds);
    _hidl_cb(status, result);
    return Void();
}

Return<void> Thermal::registerThermalChangedCallback(const sp<IThermalChangedCallback>& callback,
                                                     bool filterType, TemperatureType type,
                                                     registerThermalChangedCallback_cb _hidl_cb) {
    ThermalStatus status;
    
    if (callback == nullptr) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "Invalid callback";
        _hidl_cb(status);
        return Void();
    }
    
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    mCallbacks.push_back(callback);
    
    status.code = ThermalStatusCode::SUCCESS;
    _hidl_cb(status);
    return Void();
}

Return<void> Thermal::unregisterThermalChangedCallback(const sp<IThermalChangedCallback>& callback,
                                                       unregisterThermalChangedCallback_cb _hidl_cb) {
    ThermalStatus status;
    
    std::lock_guard<std::mutex> lock(mCallbackMutex);
    auto it = std::find(mCallbacks.begin(), mCallbacks.end(), callback);
    if (it != mCallbacks.end()) {
        mCallbacks.erase(it);
        status.code = ThermalStatusCode::SUCCESS;
    } else {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "Callback not found";
    }
    
    _hidl_cb(status);
    return Void();
}

Return<void> Thermal::getCurrentCoolingDevices(bool filterType, CoolingType type,
                                               getCurrentCoolingDevices_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    
    std::vector<CoolingDevice> devices;
    
    if (!filterType || type == CoolingType::FAN) {
        CoolingDevice device;
        device.type = CoolingType::FAN;
        device.name = "Pi 5 Cooler";
        
        std::ifstream pwmFile(FAN_PWM);
        int pwmValue = 0;
        if (pwmFile.is_open()) {
            pwmFile >> pwmValue;
        }
        device.value = pwmValue;
        
        devices.push_back(device);
    }
    
    hidl_vec<CoolingDevice> result(devices);
    _hidl_cb(status, result);
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
