# Raspberry Pi 5 Hardware Support

This document details the hardware support status for Android on Raspberry Pi 5.

## BCM2712 SoC Overview

The Raspberry Pi 5 uses the BCM2712 SoC with the following specifications:

| Component | Specification |
|-----------|---------------|
| CPU | Quad-core ARM Cortex-A76 @ 2.4GHz |
| GPU | VideoCore VII (V3D 7.1) |
| RAM | 4GB or 8GB LPDDR4X-4267 |
| Video Output | 2x HDMI 2.0 (4Kp60), DSI |
| Video Input | 2x MIPI CSI-2 |
| USB | 2x USB 3.0, 2x USB 2.0 |
| Networking | Gigabit Ethernet, WiFi 5, BT 5.0 |
| Storage | microSD, PCIe 2.0 x1 |
| GPIO | 40-pin header |

## Display Support

### HDMI

- **Status**: ✅ Fully Working
- **Features**:
  - Dual 4K@60Hz output
  - HDMI CEC support
  - Audio over HDMI
  - Hot-plug detection

**Configuration** (config.txt):
```ini
hdmi_force_hotplug=1
hdmi_group=2
hdmi_mode=82  # 1920x1080@60Hz
```

### DSI (Official Display)

- **Status**: 🔄 Working with overlay
- **Supported Displays**:
  - Official 7" Touchscreen Display
  - Third-party DSI panels (with custom overlay)

**Enable DSI**:
```ini
dtoverlay=rpi5-dsi-display
```

### Framebuffer Console

- **Status**: ✅ Working
- **Notes**: Useful for debugging without GUI

## GPU / Graphics

### OpenGL ES

- **Status**: ✅ Working
- **Version**: OpenGL ES 3.2
- **Driver**: Mesa V3D

**Configuration**:
```properties
ro.hardware.egl=mesa
ro.opengles.version=196610
```

### Vulkan

- **Status**: ✅ Working
- **Version**: Vulkan 1.2
- **Driver**: Mesa Broadcom

**Configuration**:
```properties
ro.hardware.vulkan=broadcom
```

### Hardware Video Decode

- **Status**: 🔄 In Progress
- **Codecs**:
  - H.264: Working (V4L2 stateless)
  - H.265/HEVC: Working (V4L2 stateless)
  - VP9: In progress
  - AV1: Not supported by hardware

## Audio

### HDMI Audio

- **Status**: ✅ Working
- **Channels**: Up to 8 channels
- **Formats**: PCM, AC3, DTS passthrough

### I2S/PCM

- **Status**: 🔄 Working with HATs
- **Supported HATs**:
  - HiFiBerry DAC+
  - IQaudio DAC
  - JustBoom DAC

**Enable I2S**:
```ini
dtoverlay=hifiberry-dacplus
```

### USB Audio

- **Status**: ✅ Working
- **Notes**: Full USB Audio Class 1/2 support

## Camera

### CSI Camera (Pi Camera Module 3)

- **Status**: 🔄 In Progress
- **Sensor**: Sony IMX708 (12MP)
- **Features**:
  - Up to 4K video
  - HDR mode
  - Autofocus

**Enable Camera**:
```ini
dtoverlay=rpi5-camera-imx708
```

### USB Camera

- **Status**: ✅ Working
- **Notes**: UVC cameras fully supported

## Network

### Ethernet

- **Status**: ✅ Working
- **Speed**: Gigabit
- **Features**: WoL, VLAN, jumbo frames

### WiFi

- **Status**: ✅ Working
- **Standard**: WiFi 5 (802.11ac)
- **Bands**: 2.4GHz, 5GHz
- **Features**: AP mode, WiFi Direct

**Firmware Required**:
- `brcmfmac43455-sdio.bin`
- `brcmfmac43455-sdio.clm_blob`
- `brcmfmac43455-sdio.txt`

### Bluetooth

- **Status**: ✅ Working
- **Version**: Bluetooth 5.0 / BLE
- **Profiles**: A2DP, HFP, HID, PAN, etc.

**Firmware Required**:
- `BCM4345C0.hcd`

## USB

### USB 3.0 Ports

- **Status**: ✅ Working
- **Speed**: 5 Gbps
- **Notes**: Full xHCI support

### USB 2.0 Ports

- **Status**: ✅ Working
- **Speed**: 480 Mbps

### USB OTG

- **Status**: 🔄 Limited
- **Modes**: Host, Device (ADB)

## Storage

### microSD

- **Status**: ✅ Working
- **Speed**: UHS-I SDR104
- **Max Speed**: ~100 MB/s

### NVMe (PCIe)

- **Status**: ✅ Working
- **Interface**: PCIe 2.0 x1 (PCIe 3.0 with config)
- **Speed**: Up to 500 MB/s (PCIe 2.0) / 800 MB/s (PCIe 3.0)

**Enable PCIe 3.0**:
```ini
dtparam=pciex1_gen=3
```

⚠️ **Warning**: PCIe 3.0 is not officially certified and may cause stability issues.

## GPIO

### Digital I/O

- **Status**: ✅ Working
- **Pins**: 28 GPIO (BCM 0-27)
- **Features**: Input, Output, Pull-up/down, Interrupts

### PWM

- **Status**: ✅ Working
- **Channels**: 4 (PWM0, PWM1)
- **Pins**: GPIO12, GPIO13, GPIO18, GPIO19

### I2C

- **Status**: ✅ Working
- **Buses**: I2C0, I2C1, I2C3
- **Speed**: Standard (100kHz), Fast (400kHz)

**Enable I2C**:
```ini
dtparam=i2c_arm=on
```

### SPI

- **Status**: ✅ Working
- **Buses**: SPI0, SPI3, SPI5
- **Speed**: Up to 125 MHz

**Enable SPI**:
```ini
dtparam=spi=on
```

### UART

- **Status**: ✅ Working
- **Ports**: UART0, UART2, UART3, UART4
- **Notes**: UART0 is primary serial console

**Enable UART**:
```ini
enable_uart=1
```

## Power Management

### CPU Frequency Scaling

- **Status**: ✅ Working
- **Governors**: performance, powersave, schedutil, ondemand
- **Range**: 600 MHz - 2.4 GHz

### Thermal Management

- **Status**: ✅ Working
- **Throttle Points**: 70°C, 80°C, 85°C
- **Shutdown**: 90°C

### Fan Control

- **Status**: ✅ Working (Official Active Cooler)
- **Control**: PWM via HAL

## Sensors

### Temperature Sensor

- **Status**: ✅ Working
- **Location**: CPU die
- **Range**: -40°C to 125°C

### Other Sensors

External sensors can be added via I2C/SPI:
- Accelerometer
- Gyroscope
- Compass
- Pressure/Humidity

## RTC (Real-Time Clock)

- **Status**: ✅ Working (External)
- **Supported RTCs**:
  - DS1307
  - DS3231
  - PCF8523
  - RV3028

**Enable RTC**:
```ini
dtoverlay=i2c-rtc,ds3231
```

## Known Limitations

1. **No hardware AES acceleration** - Software encryption only
2. **Limited V4L2 codec support** - Some formats use software decode
3. **No DSP** - Audio processing is CPU-based
4. **Power consumption** - Higher than previous Pi models

## HAT Compatibility

### Verified HATs

| HAT | Status | Notes |
|-----|--------|-------|
| Sense HAT | ✅ Working | Full support |
| HiFiBerry DAC+ | ✅ Working | Audio HAT |
| Official PoE+ HAT | ✅ Working | Power + cooling |
| PiJuice | 🔄 Partial | Battery HAL needed |
| RasClock | ✅ Working | RTC |

### HAT Development

For custom HATs, create device tree overlays following the format in `overlays/`.
