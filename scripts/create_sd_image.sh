#!/bin/bash
# Create SD card image for Raspberry Pi 5 Android 16
# This script creates a flashable SD card image from AOSP build output

set -e

OUT_DIR="/root/aosp-rpi5/out/target/product/rpi5"
IMAGE_NAME="android-16-rpi5.img"
IMAGE_SIZE="8G"  # Total SD card image size

echo "=== Creating Android 16 SD Card Image for Raspberry Pi 5 ==="
echo ""

# Check required files exist
REQUIRED_FILES=(
    "boot.img"
    "system.img"
    "vendor.img"
    "product.img"
    "system_ext.img"
    "odm.img"
    "userdata.img"
    "vbmeta.img"
    "dtb.img"
)

echo "Checking required files..."
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$OUT_DIR/$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
    echo "  Found: $file"
done
echo ""

# Create the image file
echo "Creating $IMAGE_SIZE image file..."
cd "$OUT_DIR"
rm -f "$IMAGE_NAME"
fallocate -l $IMAGE_SIZE "$IMAGE_NAME" ; dd if=/dev/zero of="$IMAGE_NAME" bs=1M count=8192 status=progress

# Create partition table (GPT)
echo "Creating GPT partition table..."
parted -s "$IMAGE_NAME" mklabel gpt

# Partition layout for Raspberry Pi 5 with Android:
# 1. boot_firmware (FAT32, 256MB) - RPi bootloader, kernel, DTB
# 2. boot_a (ext4, 64MB) - Android boot image slot A
# 3. boot_b (ext4, 64MB) - Android boot image slot B (A/B updates)
# 4. misc (raw, 4MB) - Bootloader control
# 5. metadata (ext4, 16MB) - Encryption metadata
# 6. vbmeta_a (raw, 1MB) - Verified boot slot A
# 7. vbmeta_b (raw, 1MB) - Verified boot slot B
# 8. super (raw, 4.5GB) - Dynamic partitions (system, vendor, etc.)
# 9. userdata (f2fs, remaining) - User data
# 10. cache (ext4, 256MB) - Cache

echo "Creating partitions..."
parted -s "$IMAGE_NAME" mkpart boot_firmware fat32 1MiB 257MiB
parted -s "$IMAGE_NAME" mkpart boot_a ext4 257MiB 321MiB
parted -s "$IMAGE_NAME" mkpart boot_b ext4 321MiB 385MiB
parted -s "$IMAGE_NAME" mkpart misc 385MiB 389MiB
parted -s "$IMAGE_NAME" mkpart metadata ext4 389MiB 405MiB
parted -s "$IMAGE_NAME" mkpart vbmeta_a 405MiB 406MiB
parted -s "$IMAGE_NAME" mkpart vbmeta_b 406MiB 407MiB
parted -s "$IMAGE_NAME" mkpart super 407MiB 4907MiB
parted -s "$IMAGE_NAME" mkpart userdata 4907MiB 7407MiB
parted -s "$IMAGE_NAME" mkpart cache ext4 7407MiB 7663MiB

# Set partition names
parted -s "$IMAGE_NAME" name 1 boot_firmware
parted -s "$IMAGE_NAME" name 2 boot_a
parted -s "$IMAGE_NAME" name 3 boot_b
parted -s "$IMAGE_NAME" name 4 misc
parted -s "$IMAGE_NAME" name 5 metadata
parted -s "$IMAGE_NAME" name 6 vbmeta_a
parted -s "$IMAGE_NAME" name 7 vbmeta_b
parted -s "$IMAGE_NAME" name 8 super
parted -s "$IMAGE_NAME" name 9 userdata
parted -s "$IMAGE_NAME" name 10 cache

# Set up loop device
echo "Setting up loop device..."
LOOP_DEV=$(losetup --find --show --partscan "$IMAGE_NAME")
echo "Using loop device: $LOOP_DEV"

# Wait for partition devices to appear
sleep 2
partprobe "$LOOP_DEV"
sleep 2

# Format partitions
echo "Formatting partitions..."
mkfs.vfat -F 32 -n BOOT_FW "${LOOP_DEV}p1"
mkfs.ext4 -L boot_a "${LOOP_DEV}p2"
mkfs.ext4 -L boot_b "${LOOP_DEV}p3"
mkfs.ext4 -L metadata "${LOOP_DEV}p5"
mkfs.ext4 -L cache "${LOOP_DEV}p10"

# Create boot firmware partition content
echo "Populating boot_firmware partition..."
BOOT_MNT=$(mktemp -d)
mount "${LOOP_DEV}p1" "$BOOT_MNT"

# Copy RPi5 boot files
if [ -d "/root/aosp-rpi5/device/brcm/rpi5/boot" ]; then
    cp -r /root/aosp-rpi5/device/brcm/rpi5/boot/* "$BOOT_MNT/" 2>/dev/null ; true
fi

# Copy kernel and DTBs
if [ -f "$OUT_DIR/kernel" ]; then
    cp "$OUT_DIR/kernel" "$BOOT_MNT/kernel8.img"
fi

# Copy device tree files
mkdir -p "$BOOT_MNT/overlays"
if [ -d "/root/aosp-rpi5/device/brcm/rpi5/dtbs" ]; then
    cp /root/aosp-rpi5/device/brcm/rpi5/dtbs/*.dtb "$BOOT_MNT/" 2>/dev/null ; true
    cp /root/aosp-rpi5/device/brcm/rpi5/dtbs/overlays/* "$BOOT_MNT/overlays/" 2>/dev/null ; true
fi

# Create config.txt for RPi5
cat > "$BOOT_MNT/config.txt" << 'BOOTCONFIG'
# Raspberry Pi 5 Android 16 Boot Configuration

# Enable 64-bit mode
arm_64bit=1

# Kernel and initrd
kernel=kernel8.img
initramfs ramdisk.img followkernel

# Memory
gpu_mem=256
total_mem=8192

# Display
hdmi_force_hotplug=1
disable_overscan=1
max_framebuffers=2

# Enable DRM VC4 driver
dtoverlay=vc4-kms-v3d-pi5
max_framebuffer_height=2160
max_framebuffer_width=3840

# USB
dtoverlay=dwc2,dr_mode=host

# PCIe/NVMe support
dtparam=pciex1
dtparam=pciex1_gen=3

# Audio
dtparam=audio=on

# Serial console
enable_uart=1

# Camera and display support
camera_auto_detect=1
display_auto_detect=1

# Boot partition
boot_delay=1

# Android-specific
otg_mode=1
BOOTCONFIG

# Create cmdline.txt
cat > "$BOOT_MNT/cmdline.txt" << 'CMDLINE'
coherent_pool=1M 8250.nr_uarts=1 snd_bcm2835.enable_compat_alsa=0 snd_bcm2835.enable_hdmi=1 video=HDMI-A-1:1920x1080@60 smsc95xx.macaddr=DC:A6:32:XX:XX:XX vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000 console=ttyAMA0,115200 androidboot.console=ttyAMA0 androidboot.hardware=rpi5 androidboot.selinux=permissive init=/init firmware_class.path=/vendor/firmware ro rootwait
CMDLINE

# Copy ramdisk
if [ -f "$OUT_DIR/ramdisk.img" ]; then
    cp "$OUT_DIR/ramdisk.img" "$BOOT_MNT/"
fi

umount "$BOOT_MNT"
rmdir "$BOOT_MNT"

# Write boot images to A slot
echo "Writing boot_a partition..."
dd if="$OUT_DIR/boot.img" of="${LOOP_DEV}p2" bs=4M status=progress

# Write vbmeta
echo "Writing vbmeta_a partition..."
dd if="$OUT_DIR/vbmeta.img" of="${LOOP_DEV}p6" bs=1M status=progress

# Create super partition with lpmake
echo "Creating super partition with dynamic partitions..."

# Check if lpmake is available
LPMAKE="$OUT_DIR/../../../host/linux-x86/bin/lpmake"
if [ ! -f "$LPMAKE" ]; then
    LPMAKE=$(find /root/aosp-rpi5/out -name "lpmake" -type f | head -1)
fi

if [ -f "$LPMAKE" ]; then
    echo "Using lpmake: $LPMAKE"
    
    # Create super.img
    "$LPMAKE" \
        --metadata-size 65536 \
        --super-name super \
        --metadata-slots 2 \
        --device super:4718592000 \
        --group rpi5_dynamic_partitions_a:4714397696 \
        --group rpi5_dynamic_partitions_b:4714397696 \
        --partition system_a:readonly:$(stat -c%s "$OUT_DIR/system.img"):rpi5_dynamic_partitions_a \
        --image system_a="$OUT_DIR/system.img" \
        --partition vendor_a:readonly:$(stat -c%s "$OUT_DIR/vendor.img"):rpi5_dynamic_partitions_a \
        --image vendor_a="$OUT_DIR/vendor.img" \
        --partition product_a:readonly:$(stat -c%s "$OUT_DIR/product.img"):rpi5_dynamic_partitions_a \
        --image product_a="$OUT_DIR/product.img" \
        --partition system_ext_a:readonly:$(stat -c%s "$OUT_DIR/system_ext.img"):rpi5_dynamic_partitions_a \
        --image system_ext_a="$OUT_DIR/system_ext.img" \
        --partition odm_a:readonly:$(stat -c%s "$OUT_DIR/odm.img"):rpi5_dynamic_partitions_a \
        --image odm_a="$OUT_DIR/odm.img" \
        --partition system_b:readonly:0:rpi5_dynamic_partitions_b \
        --partition vendor_b:readonly:0:rpi5_dynamic_partitions_b \
        --partition product_b:readonly:0:rpi5_dynamic_partitions_b \
        --partition system_ext_b:readonly:0:rpi5_dynamic_partitions_b \
        --partition odm_b:readonly:0:rpi5_dynamic_partitions_b \
        --sparse \
        --output "$OUT_DIR/super.img"
    
    # Convert sparse to raw and write
    SIMG2IMG="$OUT_DIR/../../../host/linux-x86/bin/simg2img"
    if [ ! -f "$SIMG2IMG" ]; then
        SIMG2IMG=$(find /root/aosp-rpi5/out -name "simg2img" -type f | head -1)
    fi
    
    if [ -f "$SIMG2IMG" ]; then
        echo "Converting sparse super.img to raw..."
        "$SIMG2IMG" "$OUT_DIR/super.img" "$OUT_DIR/super_raw.img"
        dd if="$OUT_DIR/super_raw.img" of="${LOOP_DEV}p8" bs=4M status=progress
        rm -f "$OUT_DIR/super_raw.img"
    else
        echo "WARNING: simg2img not found, writing sparse image directly"
        dd if="$OUT_DIR/super.img" of="${LOOP_DEV}p8" bs=4M status=progress
    fi
else
    echo "WARNING: lpmake not found, manually copying partition images..."
    # Fallback: write individual images directly (may not work with dynamic partitions)
    echo "This may not produce a bootable image without proper dynamic partition setup"
fi

# Write userdata partition
echo "Writing userdata partition..."
dd if="$OUT_DIR/userdata.img" of="${LOOP_DEV}p9" bs=4M status=progress

# Write cache partition
echo "Writing cache partition..."
dd if="$OUT_DIR/cache.img" of="${LOOP_DEV}p10" bs=4M status=progress

# Sync and cleanup
echo "Syncing..."
sync

# Release loop device
echo "Releasing loop device..."
losetup -d "$LOOP_DEV"

# Calculate final image size
FINAL_SIZE=$(ls -lh "$OUT_DIR/$IMAGE_NAME" | awk '{print $5}')
echo ""
echo "=== SD Card Image Created Successfully ==="
echo "Output: $OUT_DIR/$IMAGE_NAME"
echo "Size: $FINAL_SIZE"
echo ""
echo "To flash to SD card:"
echo "  Linux:   sudo dd if=$OUT_DIR/$IMAGE_NAME of=/dev/sdX bs=4M status=progress"
echo "  Windows: Use Raspberry Pi Imager or balenaEtcher"
echo ""
