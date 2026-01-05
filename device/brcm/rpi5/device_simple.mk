#
# Copyright (C) 2025 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0
# Device-specific packages and configurations for Raspberry Pi 5
# SIMPLIFIED version - using AOSP default HALs

LOCAL_PATH := device/brcm/rpi5

# Enable updating of APEXes
$(call inherit-product, $(SRC_TARGET_DIR)/product/updatable_apex.mk)

# Generic ramdisk configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_ramdisk.mk)

# Fastboot HAL
PRODUCT_PACKAGES += \
    fastbootd

# Health HAL
PRODUCT_PACKAGES += \
    android.hardware.health-service.example

# Gatekeeper HAL
PRODUCT_PACKAGES += \
    android.hardware.gatekeeper-service.software

# Power HAL
PRODUCT_PACKAGES += \
    android.hardware.power-service.example

# Memtrack HAL
PRODUCT_PACKAGES += \
    android.hardware.memtrack-service.example

# Graphics - minimal
PRODUCT_PACKAGES += \
    libGLES_mesa \
    gralloc.default

# Audio - minimal
PRODUCT_PACKAGES += \
    audio.primary.default \
    audio.r_submix.default

# Display settings
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.surface_flinger.max_frame_buffer_acquired_buffers=3

# System properties
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.egl=mesa \
    ro.opengles.version=196610 \
    dalvik.vm.heapsize=512m

# Inherit vendor configuration
$(call inherit-product-if-exists, vendor/brcm/rpi5/rpi5-vendor.mk)
