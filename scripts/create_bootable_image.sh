#!/bin/bash
# Create bootable SD card image for Raspberry Pi 5 running Android 16

set -e

PRODUCT_OUT="/root/aosp-rpi5/out/target/product/rpi5"
DEVICE_DIR="/root/aosp-rpi5/device/brcm/rpi5"
IMG_NAME="android-16-rpi5.img"

echo "=== Creating Bootable Android 16 SD Card Image for RPi 5 ==="

cd $PRODUCT_OUT

# Remove old image if exists
rm -f $IMG_NAME

# Create empty image file (8GB)
echo "[1/8] Creating 8GB image file..."
dd if=/dev/zero of=$IMG_NAME bs=1M count=8192 status=progress

# Create partition table
echo "[2/8] Creating GPT partition table..."
parted -s $IMG_NAME mklabel gpt

# Create partitions
# 1: boot_firmware (FAT32 for RPi bootloader) - 256MB
# 2: boot_a (Android boot) - 64MB  
# 3: misc - 4MB
# 4: metadata - 16MB
# 5: super (dynamic partitions) - 5GB
# 6: userdata - remaining

echo "[3/8] Creating partitions..."
parted -s $IMG_NAME \
    mkpart boot_firmware fat32 1MiB 257MiB \
    mkpart boot_a ext4 257MiB 321MiB \
    mkpart misc 321MiB 325MiB \
    mkpart metadata ext4 325MiB 341MiB \
    mkpart super 341MiB 5461MiB \
    mkpart userdata ext4 5461MiB 100%

# Set boot flag on first partition
parted -s $IMG_NAME set 1 boot on

# Setup loop device
echo "[4/8] Setting up loop device..."
LOOP=$(losetup -f --show -P $IMG_NAME)
echo "Using loop device: $LOOP"

# Wait for partitions to appear
sleep 2
partprobe $LOOP 2>/dev/null || true

# Format partitions
echo "[5/8] Formatting partitions..."
mkfs.vfat -F 32 -n BOOT_FW ${LOOP}p1
mkfs.ext4 -F -L boot_a ${LOOP}p2
mkfs.ext4 -F -L metadata ${LOOP}p4
mkfs.ext4 -F -L userdata ${LOOP}p6

# Mount boot_firmware and populate it
echo "[6/8] Populating boot_firmware partition with RPi bootloader..."
mkdir -p /mnt/boot_fw
mount ${LOOP}p1 /mnt/boot_fw

# Copy RPi firmware files
cp $DEVICE_DIR/firmware/start4.elf /mnt/boot_fw/
cp $DEVICE_DIR/firmware/fixup4.dat /mnt/boot_fw/
cp $DEVICE_DIR/firmware/config.txt /mnt/boot_fw/
cp $DEVICE_DIR/firmware/cmdline.txt /mnt/boot_fw/
cp $DEVICE_DIR/firmware/bcm2712-rpi-5-b.dtb /mnt/boot_fw/

# Copy kernel as Image
cp $PRODUCT_OUT/kernel /mnt/boot_fw/Image

# Copy ramdisk
cp $PRODUCT_OUT/ramdisk.img /mnt/boot_fw/

# Copy DTB from build if exists
if [ -f "$PRODUCT_OUT/dtb.img" ]; then
    cp $PRODUCT_OUT/dtb.img /mnt/boot_fw/
fi

# List boot partition contents
echo "Boot partition contents:"
ls -la /mnt/boot_fw/

sync
umount /mnt/boot_fw

# Write Android boot.img to boot_a partition
echo "[7/8] Writing Android boot image..."
dd if=$PRODUCT_OUT/boot.img of=${LOOP}p2 bs=4M status=progress

# Create and write super partition with dynamic partitions
echo "[8/8] Creating super partition with dynamic partitions..."

# Check for tools
SIMG2IMG="/root/aosp-rpi5/out/host/linux-x86/bin/simg2img"
LPMAKE_CMD="/root/aosp-rpi5/out/host/linux-x86/bin/lpmake"

if [ ! -f "$LPMAKE_CMD" ]; then
    echo "Error: lpmake not found at $LPMAKE_CMD"
    losetup -d $LOOP
    exit 1
fi

if [ ! -f "$SIMG2IMG" ]; then
    echo "Error: simg2img not found at $SIMG2IMG"
    losetup -d $LOOP
    exit 1
fi

# Convert sparse images to raw to get actual sizes
echo "Converting sparse images to raw format..."
mkdir -p /tmp/raw_images
$SIMG2IMG $PRODUCT_OUT/system.img /tmp/raw_images/system.img
$SIMG2IMG $PRODUCT_OUT/vendor.img /tmp/raw_images/vendor.img
$SIMG2IMG $PRODUCT_OUT/product.img /tmp/raw_images/product.img
$SIMG2IMG $PRODUCT_OUT/system_ext.img /tmp/raw_images/system_ext.img
$SIMG2IMG $PRODUCT_OUT/odm.img /tmp/raw_images/odm.img

# Get raw image sizes
SYSTEM_SIZE=$(stat -c%s /tmp/raw_images/system.img)
VENDOR_SIZE=$(stat -c%s /tmp/raw_images/vendor.img)
PRODUCT_SIZE=$(stat -c%s /tmp/raw_images/product.img)
SYSTEM_EXT_SIZE=$(stat -c%s /tmp/raw_images/system_ext.img)
ODM_SIZE=$(stat -c%s /tmp/raw_images/odm.img)

SUPER_SIZE=$((5120 * 1024 * 1024))  # 5GB

echo "Raw partition sizes:"
echo "  system: $((SYSTEM_SIZE / 1024 / 1024)) MB"
echo "  vendor: $((VENDOR_SIZE / 1024 / 1024)) MB"
echo "  product: $((PRODUCT_SIZE / 1024 / 1024)) MB"
echo "  system_ext: $((SYSTEM_EXT_SIZE / 1024 / 1024)) MB"
echo "  odm: $((ODM_SIZE / 1024 / 1024)) MB"
echo "  super: $((SUPER_SIZE / 1024 / 1024)) MB"

# Create super.img with lpmake using raw images
echo "Building super.img with lpmake..."
$LPMAKE_CMD \
    --device-size=$SUPER_SIZE \
    --metadata-size=65536 \
    --metadata-slots=2 \
    --group=rpi5_dynamic_partitions:$SUPER_SIZE \
    --partition=system:readonly:$SYSTEM_SIZE:rpi5_dynamic_partitions \
    --image=system=/tmp/raw_images/system.img \
    --partition=vendor:readonly:$VENDOR_SIZE:rpi5_dynamic_partitions \
    --image=vendor=/tmp/raw_images/vendor.img \
    --partition=product:readonly:$PRODUCT_SIZE:rpi5_dynamic_partitions \
    --image=product=/tmp/raw_images/product.img \
    --partition=system_ext:readonly:$SYSTEM_EXT_SIZE:rpi5_dynamic_partitions \
    --image=system_ext=/tmp/raw_images/system_ext.img \
    --partition=odm:readonly:$ODM_SIZE:rpi5_dynamic_partitions \
    --image=odm=/tmp/raw_images/odm.img \
    --output=$PRODUCT_OUT/super_raw.img

echo "Writing super partition to image..."
dd if=$PRODUCT_OUT/super_raw.img of=${LOOP}p5 bs=4M status=progress

# Cleanup temp files
rm -rf /tmp/raw_images
rm -f $PRODUCT_OUT/super_raw.img

# Cleanup
echo "Syncing and cleaning up..."
sync
losetup -d $LOOP

echo ""
echo "=========================================="
echo "  Image creation complete!"
echo "=========================================="
echo ""
echo "Output: $PRODUCT_OUT/$IMG_NAME"
echo "Size: $(ls -lh $PRODUCT_OUT/$IMG_NAME | awk '{print $5}')"
echo ""
echo "To flash to SD card (Linux):"
echo "  sudo dd if=$IMG_NAME of=/dev/sdX bs=4M status=progress conv=fsync"
echo ""
echo "To flash on Windows, use:"
echo "  - Raspberry Pi Imager (select custom image)"
echo "  - balenaEtcher"
echo "  - Win32DiskImager"
