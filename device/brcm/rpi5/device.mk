#
# Copyright (C) 2025 The Android Open Source Project
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

# Device-specific packages and configurations for Raspberry Pi 5

LOCAL_PATH := device/brcm/rpi5

# Enable updating of APEXes
$(call inherit-product, $(SRC_TARGET_DIR)/product/updatable_apex.mk)

# Virtual A/B support
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota/launch_with_vendor_ramdisk.mk)

# Generic ramdisk configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_ramdisk.mk)

# Fastboot HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.fastboot-service.example \
    fastbootd

# Boot control HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.boot-service.default \
    android.hardware.boot-service.default_recovery

# Health HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.health-service.example \
    android.hardware.health-service.example_recovery

# Update engine
PRODUCT_PACKAGES += \
    update_engine \
    update_engine_client \
    update_verifier \
    update_engine_sideload

# USB HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.usb-service.rpi5 \
    android.hardware.usb.gadget-service.rpi5

# Audio HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.audio.core-service.rpi5 \
    android.hardware.audio.effect-service.example \
    audio.primary.rpi5 \
    audio.usb.default \
    audio.r_submix.default \
    audio.bluetooth.default \
    tinyplay \
    tinymix \
    tinycap \
    tinypcminfo

# Audio configuration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(LOCAL_PATH)/audio/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    $(LOCAL_PATH)/audio/mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml \
    frameworks/av/services/audiopolicy/config/a2dp_in_audio_policy_configuration_7_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_in_audio_policy_configuration_7_0.xml \
    frameworks/av/services/audiopolicy/config/bluetooth_audio_policy_configuration_7_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_audio_policy_configuration_7_0.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml

# Bluetooth HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.bluetooth-service.default \
    android.hardware.bluetooth.audio-service.default \
    libbt-vendor

# WiFi HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.wifi-service \
    wpa_supplicant \
    wpa_supplicant.conf \
    hostapd \
    hostapd_cli \
    libwpa_client

# WiFi configurations
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/wpa_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant_overlay.conf \
    $(LOCAL_PATH)/wifi/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf

# Graphics HAL - Mesa/Drm (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator-service.minigbm \
    android.hardware.graphics.composer3-service.ranchu \
    mapper.minigbm \
    hwcomposer.drm \
    gralloc.minigbm \
    libGLES_mesa \
    libEGL_mesa \
    libGLESv1_CM_mesa \
    libGLESv2_mesa \
    vulkan.broadcom

# DRM HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.drm-service.clearkey

# Gatekeeper HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.gatekeeper-service.software

# KeyMint HAL (replaces Keymaster in Android 16)
PRODUCT_PACKAGES += \
    android.hardware.security.keymint-service.rust \
    android.hardware.security.sharedsecret-service.nonsecure \
    android.hardware.security.secureclock-service.nonsecure

# Power HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.power-service.rpi5

# Thermal HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.thermal-service.rpi5

# Light HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.light-service.rpi5

# Memtrack HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.memtrack-service.example

# Camera HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.camera.provider-service.rpi5 \
    camera.rpi5 \
    libcamera \
    libcamera-hal

# Camera configurations
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/camera/external_camera_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/external_camera_config.xml

# Sensors HAL (AIDL)
PRODUCT_PACKAGES += \
    android.hardware.sensors-service.multihal \
    android.hardware.sensors-service.example

# GPIO HAL (AIDL - Raspberry Pi specific)
# Disabled until custom AIDL interface is created
# PRODUCT_PACKAGES += \
#     android.hardware.gpio-service.rpi5 \
#     gpio_service \
#     libgpiod

# Display settings (removed deprecated configstore)
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.surface_flinger.max_frame_buffer_acquired_buffers=3 \
    ro.surface_flinger.present_time_offset_from_vsync_ns=0 \
    ro.surface_flinger.vsync_event_phase_offset_ns=2000000 \
    ro.surface_flinger.vsync_sf_event_phase_offset_ns=6000000

# Treble/VNDK
PRODUCT_PACKAGES += \
    vndk_package

# Overlays
PRODUCT_PACKAGES += \
    Rpi5FrameworksOverlay \
    Rpi5SystemUIOverlay \
    Rpi5SettingsOverlay \
    Rpi5SettingsProviderOverlay

# Input device configurations
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/input/gpio_keys.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/gpio_keys.kl \
    $(LOCAL_PATH)/input/gpio_keys.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/gpio_keys.idc

# Media codecs
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/media/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    $(LOCAL_PATH)/media/media_codecs_performance.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_performance.xml \
    $(LOCAL_PATH)/media/media_profiles_V1_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_c2.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_c2.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_c2_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_c2_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_c2_video.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_c2_video.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_video.xml

# V4L2 codec support for hardware video decode
PRODUCT_PACKAGES += \
    android.hardware.media.c2@1.2-service-v4l2 \
    libv4l2_codec2_common \
    libv4l2_codec2_components \
    libv4l2_codec2_store

# Seccomp policy
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/seccomp/mediacodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediacodec.policy \
    $(LOCAL_PATH)/seccomp/mediaswcodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediaswcodec.policy

# Firmware
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/firmware/brcm/brcmfmac43455-sdio.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/brcmfmac43455-sdio.bin \
    $(LOCAL_PATH)/firmware/brcm/brcmfmac43455-sdio.clm_blob:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/brcmfmac43455-sdio.clm_blob \
    $(LOCAL_PATH)/firmware/brcm/brcmfmac43455-sdio.txt:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/brcmfmac43455-sdio.txt \
    $(LOCAL_PATH)/firmware/brcm/BCM4345C0.hcd:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/BCM4345C0.hcd

# System properties
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=15 \
    ro.hardware.egl=mesa \
    ro.hardware.gralloc=minigbm \
    ro.hardware.hwcomposer=drm \
    ro.hardware.vulkan=broadcom \
    ro.opengles.version=196610 \
    debug.hwui.renderer=opengl \
    persist.sys.dalvik.vm.lib.2=libart.so \
    dalvik.vm.heapsize=512m \
    dalvik.vm.heapstartsize=16m \
    dalvik.vm.heapgrowthlimit=256m \
    dalvik.vm.heaptargetutilization=0.75 \
    dalvik.vm.heapminfree=512k \
    dalvik.vm.heapmaxfree=8m

# Raspberry Pi specific apps
PRODUCT_PACKAGES += \
    RaspberryPiSettings \
    GpioController \
    PiCamera \
    PiTerminal

# Inherit vendor configuration
$(call inherit-product-if-exists, vendor/brcm/rpi5/rpi5-vendor.mk)
