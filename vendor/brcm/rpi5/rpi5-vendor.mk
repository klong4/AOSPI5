# Copyright (C) 2025 The Android Open Source Project
# Raspberry Pi 5 Vendor Product Configuration
# Android 16 - AIDL HALs

PRODUCT_SOONG_NAMESPACES += \
    vendor/brcm/rpi5

# Prebuilt proprietary binaries and libraries

# Firmware files
PRODUCT_COPY_FILES += \
    vendor/brcm/rpi5/proprietary/firmware/brcm/brcmfmac43455-sdio.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/brcmfmac43455-sdio.bin \
    vendor/brcm/rpi5/proprietary/firmware/brcm/brcmfmac43455-sdio.clm_blob:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/brcmfmac43455-sdio.clm_blob \
    vendor/brcm/rpi5/proprietary/firmware/brcm/brcmfmac43455-sdio.txt:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/brcmfmac43455-sdio.txt \
    vendor/brcm/rpi5/proprietary/firmware/brcm/BCM4345C0.hcd:$(TARGET_COPY_OUT_VENDOR)/firmware/brcm/BCM4345C0.hcd

# VideoCore firmware
PRODUCT_COPY_FILES += \
    vendor/brcm/rpi5/proprietary/firmware/start4.elf:boot/start4.elf \
    vendor/brcm/rpi5/proprietary/firmware/fixup4.dat:boot/fixup4.dat \
    vendor/brcm/rpi5/proprietary/firmware/bcm2712-rpi-5-b.dtb:boot/bcm2712-rpi-5-b.dtb

# AIDL HAL Implementations (Android 16)
PRODUCT_PACKAGES += \
    android.hardware.gpio-service.rpi5 \
    android.hardware.power-service.rpi5 \
    android.hardware.thermal-service.rpi5 \
    android.hardware.light-service.rpi5 \
    android.hardware.usb-service.rpi5 \
    android.hardware.camera.provider-service.rpi5 \
    android.hardware.audio.core-service.rpi5 \
    audio.primary.rpi5 \
    camera.rpi5

# Mesa graphics
PRODUCT_PACKAGES += \
    libGLES_mesa \
    libEGL_mesa \
    libvulkan_broadcom

# Libraries
PRODUCT_PACKAGES += \
    libgpiod

# Vendor property overrides
PRODUCT_PROPERTY_OVERRIDES += \
    ro.vendor.build.fingerprint=brcm/rpi5/rpi5:16/AP1A.241205.001/1:userdebug/dev-keys
