#
# Copyright (C) 2024 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Product configuration for Raspberry Pi 5

# Inherit from common Android configurations
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# Inherit from device-specific configurations
$(call inherit-product, device/brcm/rpi5/device_simple.mk)

# Inherit vendor configurations if available
$(call inherit-product-if-exists, vendor/brcm/rpi5/rpi5-vendor.mk)

# Product identifiers
PRODUCT_NAME := rpi5
PRODUCT_DEVICE := rpi5
PRODUCT_BRAND := Raspberry
PRODUCT_MODEL := Raspberry Pi 5
PRODUCT_MANUFACTURER := Raspberry Pi Foundation
PRODUCT_BOARD := rpi5

# Build information
BUILD_FINGERPRINT := Raspberry/rpi5/rpi5:14/UP1A.231105.001/$(shell date +%Y%m%d):userdebug/test-keys
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=rpi5 \
    BUILD_FINGERPRINT=$(BUILD_FINGERPRINT)

# Device characteristics
PRODUCT_CHARACTERISTICS := tablet

# Screen density
PRODUCT_AAPT_CONFIG := normal large xlarge
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Shipping API level
PRODUCT_SHIPPING_API_LEVEL := 34

# Dalvik VM configuration
$(call inherit-product, frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

# Enable Google Play Store
PRODUCT_GMS_CLIENTID_BASE := android-raspberry

# Overlays
DEVICE_PACKAGE_OVERLAYS += device/brcm/rpi5/overlay

# Init scripts
PRODUCT_PACKAGES += \
    fstab.rpi5 \
    init.rpi5.rc \
    init.rpi5.usb.rc \
    ueventd.rpi5.rc

# Recovery init
PRODUCT_COPY_FILES += \
    device/brcm/rpi5/init.recovery.rpi5.rc:$(TARGET_COPY_OUT_RECOVERY)/root/init.recovery.rpi5.rc

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.audio.output.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.output.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.camera.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.external.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.external.xml \
    frameworks/native/data/etc/android.hardware.ethernet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.ethernet.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.opengles.aep.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.opengles.aep.xml \
    frameworks/native/data/etc/android.hardware.screen.landscape.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.screen.landscape.xml \
    frameworks/native/data/etc/android.hardware.screen.portrait.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.screen.portrait.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.vulkan.compute-0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.compute-0.xml \
    frameworks/native/data/etc/android.hardware.vulkan.level-0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.level-0.xml \
    frameworks/native/data/etc/android.hardware.vulkan.version-1_1.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.version-1_1.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.software.app_widgets.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.app_widgets.xml \
    frameworks/native/data/etc/android.software.backup.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.backup.xml \
    frameworks/native/data/etc/android.software.device_admin.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.device_admin.xml \
    frameworks/native/data/etc/android.software.ipsec_tunnels.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.ipsec_tunnels.xml \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml \
    frameworks/native/data/etc/android.software.print.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.print.xml \
    frameworks/native/data/etc/android.software.verified_boot.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.verified_boot.xml \
    frameworks/native/data/etc/android.software.voice_recognizers.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.voice_recognizers.xml \
    frameworks/native/data/etc/android.software.webview.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.webview.xml \
    frameworks/native/data/etc/tablet_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/tablet_core_hardware.xml

# Custom permissions for Pi-specific features
PRODUCT_COPY_FILES += \
    device/brcm/rpi5/permissions/com.raspberry.hardware.gpio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/com.raspberry.hardware.gpio.xml \
    device/brcm/rpi5/permissions/com.raspberry.hardware.spi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/com.raspberry.hardware.spi.xml \
    device/brcm/rpi5/permissions/com.raspberry.hardware.i2c.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/com.raspberry.hardware.i2c.xml \
    device/brcm/rpi5/permissions/com.raspberry.hardware.pwm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/com.raspberry.hardware.pwm.xml
