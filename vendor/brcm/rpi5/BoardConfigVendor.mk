# Copyright (C) 2024 The Android Open Source Project
# Raspberry Pi 5 Vendor Configuration

# Mark as board vendor
BOARD_VENDOR := brcm

# Product configuration
$(call inherit-product, vendor/brcm/rpi5/rpi5-vendor.mk)
