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

#define LOG_TAG "GpioHAL"

#include <android-base/logging.h>
#include <android/hardware/gpio/1.0/IGpio.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <fstream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gpiod.h>

namespace android {
namespace hardware {
namespace gpio {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

// Raspberry Pi 5 GPIO configuration
constexpr char GPIO_CHIP_NAME[] = "gpiochip0";
constexpr int GPIO_PIN_COUNT = 28;  // GPIO0-27 on 40-pin header
constexpr int GPIO_PIN_OFFSET = 0;

// GPIO pin capabilities on Raspberry Pi 5
enum class PinFunction : uint8_t {
    INPUT = 0,
    OUTPUT = 1,
    ALT0 = 4,
    ALT1 = 5,
    ALT2 = 6,
    ALT3 = 7,
    ALT4 = 3,
    ALT5 = 2
};

enum class PullMode : uint8_t {
    NONE = 0,
    PULL_DOWN = 1,
    PULL_UP = 2
};

struct GpioPin {
    int pin;
    PinFunction function;
    PullMode pull;
    bool exported;
    struct gpiod_line* line;
};

class Gpio : public IGpio {
public:
    Gpio();
    ~Gpio();

    // IGpio interface methods
    Return<Status> exportPin(int32_t pin) override;
    Return<Status> unexportPin(int32_t pin) override;
    Return<Status> setDirection(int32_t pin, Direction direction) override;
    Return<void> getDirection(int32_t pin, getDirection_cb _hidl_cb) override;
    Return<Status> setValue(int32_t pin, bool value) override;
    Return<void> getValue(int32_t pin, getValue_cb _hidl_cb) override;
    Return<Status> setPullMode(int32_t pin, PullMode mode) override;
    Return<void> getPullMode(int32_t pin, getPullMode_cb _hidl_cb) override;
    Return<Status> setEdgeTrigger(int32_t pin, EdgeTrigger trigger) override;
    Return<void> waitForEdge(int32_t pin, int64_t timeoutMs, waitForEdge_cb _hidl_cb) override;
    Return<void> getPinInfo(int32_t pin, getPinInfo_cb _hidl_cb) override;
    Return<void> listPins(listPins_cb _hidl_cb) override;

private:
    bool validatePin(int32_t pin);
    bool initGpioChip();
    void closeGpioChip();

    struct gpiod_chip* mChip;
    std::unordered_map<int, GpioPin> mPins;
    std::mutex mMutex;
    bool mInitialized;
};

Gpio::Gpio() : mChip(nullptr), mInitialized(false) {
    mInitialized = initGpioChip();
    if (mInitialized) {
        LOG(INFO) << "GPIO HAL initialized for Raspberry Pi 5";
    } else {
        LOG(ERROR) << "Failed to initialize GPIO HAL";
    }
}

Gpio::~Gpio() {
    closeGpioChip();
}

bool Gpio::initGpioChip() {
    mChip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!mChip) {
        LOG(ERROR) << "Failed to open GPIO chip: " << GPIO_CHIP_NAME;
        return false;
    }
    
    LOG(INFO) << "Opened GPIO chip: " << gpiod_chip_name(mChip)
              << " with " << gpiod_chip_num_lines(mChip) << " lines";
    return true;
}

void Gpio::closeGpioChip() {
    std::lock_guard<std::mutex> lock(mMutex);
    
    for (auto& pair : mPins) {
        if (pair.second.line) {
            gpiod_line_release(pair.second.line);
        }
    }
    mPins.clear();
    
    if (mChip) {
        gpiod_chip_close(mChip);
        mChip = nullptr;
    }
}

bool Gpio::validatePin(int32_t pin) {
    if (pin < GPIO_PIN_OFFSET || pin >= GPIO_PIN_OFFSET + GPIO_PIN_COUNT) {
        LOG(ERROR) << "Invalid GPIO pin: " << pin;
        return false;
    }
    return true;
}

Return<Status> Gpio::exportPin(int32_t pin) {
    if (!mInitialized) return Status::NOT_INITIALIZED;
    if (!validatePin(pin)) return Status::INVALID_PIN;
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    if (mPins.find(pin) != mPins.end()) {
        return Status::ALREADY_EXISTS;
    }
    
    struct gpiod_line* line = gpiod_chip_get_line(mChip, pin);
    if (!line) {
        LOG(ERROR) << "Failed to get GPIO line: " << pin;
        return Status::ERROR;
    }
    
    GpioPin gpioPin = {
        .pin = pin,
        .function = PinFunction::INPUT,
        .pull = PullMode::NONE,
        .exported = true,
        .line = line
    };
    
    mPins[pin] = gpioPin;
    LOG(INFO) << "Exported GPIO pin: " << pin;
    return Status::OK;
}

Return<Status> Gpio::unexportPin(int32_t pin) {
    if (!mInitialized) return Status::NOT_INITIALIZED;
    if (!validatePin(pin)) return Status::INVALID_PIN;
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        return Status::NOT_FOUND;
    }
    
    if (it->second.line) {
        gpiod_line_release(it->second.line);
    }
    mPins.erase(it);
    
    LOG(INFO) << "Unexported GPIO pin: " << pin;
    return Status::OK;
}

Return<Status> Gpio::setDirection(int32_t pin, Direction direction) {
    if (!mInitialized) return Status::NOT_INITIALIZED;
    if (!validatePin(pin)) return Status::INVALID_PIN;
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        return Status::NOT_FOUND;
    }
    
    int ret;
    if (direction == Direction::INPUT) {
        ret = gpiod_line_request_input(it->second.line, "android-gpio-hal");
        it->second.function = PinFunction::INPUT;
    } else {
        ret = gpiod_line_request_output(it->second.line, "android-gpio-hal", 0);
        it->second.function = PinFunction::OUTPUT;
    }
    
    if (ret < 0) {
        LOG(ERROR) << "Failed to set direction for pin " << pin;
        return Status::ERROR;
    }
    
    return Status::OK;
}

Return<void> Gpio::getDirection(int32_t pin, getDirection_cb _hidl_cb) {
    if (!mInitialized) {
        _hidl_cb(Status::NOT_INITIALIZED, Direction::INPUT);
        return Void();
    }
    
    if (!validatePin(pin)) {
        _hidl_cb(Status::INVALID_PIN, Direction::INPUT);
        return Void();
    }
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        _hidl_cb(Status::NOT_FOUND, Direction::INPUT);
        return Void();
    }
    
    Direction dir = (it->second.function == PinFunction::OUTPUT) ? 
                    Direction::OUTPUT : Direction::INPUT;
    _hidl_cb(Status::OK, dir);
    return Void();
}

Return<Status> Gpio::setValue(int32_t pin, bool value) {
    if (!mInitialized) return Status::NOT_INITIALIZED;
    if (!validatePin(pin)) return Status::INVALID_PIN;
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        return Status::NOT_FOUND;
    }
    
    if (it->second.function != PinFunction::OUTPUT) {
        return Status::INVALID_OPERATION;
    }
    
    int ret = gpiod_line_set_value(it->second.line, value ? 1 : 0);
    if (ret < 0) {
        LOG(ERROR) << "Failed to set value for pin " << pin;
        return Status::ERROR;
    }
    
    return Status::OK;
}

Return<void> Gpio::getValue(int32_t pin, getValue_cb _hidl_cb) {
    if (!mInitialized) {
        _hidl_cb(Status::NOT_INITIALIZED, false);
        return Void();
    }
    
    if (!validatePin(pin)) {
        _hidl_cb(Status::INVALID_PIN, false);
        return Void();
    }
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        _hidl_cb(Status::NOT_FOUND, false);
        return Void();
    }
    
    int value = gpiod_line_get_value(it->second.line);
    if (value < 0) {
        _hidl_cb(Status::ERROR, false);
        return Void();
    }
    
    _hidl_cb(Status::OK, value != 0);
    return Void();
}

Return<Status> Gpio::setPullMode(int32_t pin, PullMode mode) {
    if (!mInitialized) return Status::NOT_INITIALIZED;
    if (!validatePin(pin)) return Status::INVALID_PIN;
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        return Status::NOT_FOUND;
    }
    
    // Note: Pull mode requires kernel support via gpio-line-bias
    // This may need to be implemented via sysfs or device tree
    int flags = GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
    
    switch (static_cast<uint8_t>(mode)) {
        case static_cast<uint8_t>(PullMode::PULL_UP):
            flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
            break;
        case static_cast<uint8_t>(PullMode::PULL_DOWN):
            flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
            break;
        case static_cast<uint8_t>(PullMode::NONE):
        default:
            flags = GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
            break;
    }
    
    it->second.pull = mode;
    LOG(INFO) << "Set pull mode for pin " << pin << " to " << static_cast<int>(mode);
    return Status::OK;
}

Return<void> Gpio::getPullMode(int32_t pin, getPullMode_cb _hidl_cb) {
    if (!mInitialized) {
        _hidl_cb(Status::NOT_INITIALIZED, PullMode::NONE);
        return Void();
    }
    
    if (!validatePin(pin)) {
        _hidl_cb(Status::INVALID_PIN, PullMode::NONE);
        return Void();
    }
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        _hidl_cb(Status::NOT_FOUND, PullMode::NONE);
        return Void();
    }
    
    _hidl_cb(Status::OK, it->second.pull);
    return Void();
}

Return<Status> Gpio::setEdgeTrigger(int32_t pin, EdgeTrigger trigger) {
    if (!mInitialized) return Status::NOT_INITIALIZED;
    if (!validatePin(pin)) return Status::INVALID_PIN;
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        return Status::NOT_FOUND;
    }
    
    int eventType;
    switch (trigger) {
        case EdgeTrigger::RISING:
            eventType = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
            break;
        case EdgeTrigger::FALLING:
            eventType = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
            break;
        case EdgeTrigger::BOTH:
            eventType = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
            break;
        case EdgeTrigger::NONE:
        default:
            return Status::OK;  // Nothing to configure
    }
    
    int ret = gpiod_line_request_both_edges_events(it->second.line, "android-gpio-hal");
    if (ret < 0) {
        LOG(ERROR) << "Failed to set edge trigger for pin " << pin;
        return Status::ERROR;
    }
    
    return Status::OK;
}

Return<void> Gpio::waitForEdge(int32_t pin, int64_t timeoutMs, waitForEdge_cb _hidl_cb) {
    if (!mInitialized) {
        _hidl_cb(Status::NOT_INITIALIZED, EdgeTrigger::NONE);
        return Void();
    }
    
    if (!validatePin(pin)) {
        _hidl_cb(Status::INVALID_PIN, EdgeTrigger::NONE);
        return Void();
    }
    
    std::lock_guard<std::mutex> lock(mMutex);
    
    auto it = mPins.find(pin);
    if (it == mPins.end()) {
        _hidl_cb(Status::NOT_FOUND, EdgeTrigger::NONE);
        return Void();
    }
    
    struct timespec timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_nsec = (timeoutMs % 1000) * 1000000;
    
    int ret = gpiod_line_event_wait(it->second.line, &timeout);
    if (ret < 0) {
        _hidl_cb(Status::ERROR, EdgeTrigger::NONE);
        return Void();
    }
    
    if (ret == 0) {
        _hidl_cb(Status::TIMEOUT, EdgeTrigger::NONE);
        return Void();
    }
    
    struct gpiod_line_event event;
    ret = gpiod_line_event_read(it->second.line, &event);
    if (ret < 0) {
        _hidl_cb(Status::ERROR, EdgeTrigger::NONE);
        return Void();
    }
    
    EdgeTrigger trigger = (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) ?
                          EdgeTrigger::RISING : EdgeTrigger::FALLING;
    
    _hidl_cb(Status::OK, trigger);
    return Void();
}

Return<void> Gpio::getPinInfo(int32_t pin, getPinInfo_cb _hidl_cb) {
    PinInfo info = {};
    
    if (!mInitialized) {
        _hidl_cb(Status::NOT_INITIALIZED, info);
        return Void();
    }
    
    if (!validatePin(pin)) {
        _hidl_cb(Status::INVALID_PIN, info);
        return Void();
    }
    
    info.pin = pin;
    info.name = "GPIO" + std::to_string(pin);
    info.capabilities = PinCapability::INPUT | PinCapability::OUTPUT | 
                       PinCapability::PWM | PinCapability::INTERRUPT;
    
    // Map physical pins to GPIO numbers for Raspberry Pi 5
    // This follows the BCM numbering scheme
    static const std::unordered_map<int, std::string> pinNames = {
        {2, "SDA1"}, {3, "SCL1"}, {4, "GPIO_GCLK"},
        {17, "GPIO_GEN0"}, {27, "GPIO_GEN2"}, {22, "GPIO_GEN3"},
        {10, "SPI_MOSI"}, {9, "SPI_MISO"}, {11, "SPI_CLK"},
        {14, "TXD0"}, {15, "RXD0"},
        {18, "GPIO_GEN1/PWM0"}, {23, "GPIO_GEN4"}, {24, "GPIO_GEN5"},
        {25, "GPIO_GEN6"}, {8, "SPI_CE0_N"}, {7, "SPI_CE1_N"},
        {12, "PWM0"}, {13, "PWM1"}, {19, "PWM1"}, {16, "GPIO16"},
        {26, "GPIO26"}, {20, "GPIO20"}, {21, "GPIO21"}
    };
    
    auto nameIt = pinNames.find(pin);
    if (nameIt != pinNames.end()) {
        info.name = nameIt->second;
    }
    
    _hidl_cb(Status::OK, info);
    return Void();
}

Return<void> Gpio::listPins(listPins_cb _hidl_cb) {
    hidl_vec<int32_t> pins;
    
    if (!mInitialized) {
        _hidl_cb(Status::NOT_INITIALIZED, pins);
        return Void();
    }
    
    pins.resize(GPIO_PIN_COUNT);
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        pins[i] = GPIO_PIN_OFFSET + i;
    }
    
    _hidl_cb(Status::OK, pins);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gpio
}  // namespace hardware
}  // namespace android
