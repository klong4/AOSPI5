# AOSPI5 - Creating Flashable Images

This guide explains how to create a flashable Android image for Raspberry Pi 5 that can be written using **Raspberry Pi Imager** on Windows, macOS, or Linux.

## Prerequisites

### Build Requirements
- Completed AOSP build (`./scripts/build.sh`)
- At least 20GB free disk space for image creation
- Linux environment (native, WSL2, or VM)

### Tools Required

**Linux (including WSL2):**
```bash
sudo apt-get install parted dosfstools e2fsprogs rsync xz-utils zip
```

**Windows:**
- WSL2 with Ubuntu installed
- Raspberry Pi Imager

## Quick Start

### On Linux/WSL2:
```bash
cd /path/to/AOSPI5
sudo ./scripts/create_flashable_image.sh
```

### On Windows (using WSL):
```powershell
cd D:\AOSPI5\scripts
.\build_image_windows.ps1 -UseWSL
```

## Output Files

After successful image creation, you'll find these files in the `release/` directory:

| File | Description | Use With |
|------|-------------|----------|
| `AOSPI5_Android16_YYYYMMDD.img` | Raw disk image | Linux dd, balenaEtcher |
| `AOSPI5_Android16_YYYYMMDD.img.xz` | Compressed image | **Raspberry Pi Imager** |
| `AOSPI5_Android16_YYYYMMDD.zip` | ZIP archive | Win32DiskImager |
| `*.sha256` | Checksums | Verification |

## Flashing Instructions

### Using Raspberry Pi Imager (Recommended)

1. **Download** Raspberry Pi Imager from:
   - https://www.raspberrypi.com/software/

2. **Open** Raspberry Pi Imager

3. **Choose OS:**
   - Click "Choose OS"
   - Scroll down and select "Use custom"
   - Navigate to and select `AOSPI5_Android16_YYYYMMDD.img.xz`

4. **Choose Storage:**
   - Click "Choose Storage"
   - Select your SD card (minimum 16GB, 32GB+ recommended)

5. **Write:**
   - Click "Write"
   - Wait for the write and verification to complete

### Using balenaEtcher (Alternative)

1. Download balenaEtcher from https://www.balena.io/etcher/
2. Select the `.img` or `.img.xz` file
3. Select your SD card
4. Click "Flash!"

### Using dd (Linux)

```bash
# Identify your SD card (BE CAREFUL!)
lsblk

# Flash the image (replace /dev/sdX with your SD card)
xzcat AOSPI5_Android16_YYYYMMDD.img.xz | sudo dd of=/dev/sdX bs=4M status=progress
sync
```

## Post-Flash Configuration

After flashing, the boot partition is accessible on Windows/macOS. You can edit `config.txt` to configure hardware before first boot.

### Enabling Displays

Edit `config.txt` and uncomment the appropriate overlay:

**MIPI DSI Displays:**
```ini
# Official Raspberry Pi 7" Display
dtoverlay=vc4-kms-dsi-7inch

# Waveshare DSI Displays
dtoverlay=vc4-kms-dsi-waveshare-panel,4_0_inch
```

**SPI Displays (3.5"):**
```ini
# Waveshare 3.5" Type A/B/C
dtoverlay=waveshare-35-a
dtoverlay=waveshare-35-b
dtoverlay=waveshare-35-c

# Adafruit PiTFT 3.5"
dtoverlay=adafruit-pitft35

# Elecrow 3.5"
dtoverlay=elecrow-35

# Generic ILI9488 3.5"
dtoverlay=spi-ili9488-35
```

**SPI Displays (2.8" / 3.2"):**
```ini
# Waveshare 2.8" Touch
dtoverlay=waveshare-28-tft-touch

# Waveshare 3.2" Touch
dtoverlay=waveshare-32-spi-touch
```

### Enabling Cameras

Camera auto-detection is enabled by default. To manually specify:

```ini
# Pi Camera v1 (OV5647)
dtoverlay=ov5647

# Pi Camera v2 (IMX219)
dtoverlay=imx219

# Pi Camera v3 (IMX708 with autofocus)
dtoverlay=imx708

# Pi HQ Camera (IMX477)
dtoverlay=imx477

# Pi Global Shutter Camera (IMX296)
dtoverlay=imx296

# Arducam 64MP
dtoverlay=arducam-64mp
```

### NPU/AI Accelerators

PCIe and USB accelerators are auto-detected. Supported devices:
- Google Coral Edge TPU (USB/PCIe)
- Hailo-8 / Hailo-8L / Hailo-15
- Intel Neural Compute Stick 2
- Kneron KL520/720/730

No configuration needed - devices are detected at boot.

## Partition Layout

The flashable image contains:

| Partition | Size | Filesystem | Contents |
|-----------|------|------------|----------|
| boot | 512MB | FAT32 | Kernel, DTBs, config.txt |
| system | 4GB | ext4 | Android system |
| vendor | 1GB | ext4 | Vendor HALs |
| product | 512MB | ext4 | Product apps |
| userdata | 4GB | ext4 | User data (expands on first boot) |

## Troubleshooting

### Image won't boot

1. Re-flash the image
2. Check power supply (5V 5A USB-C recommended)
3. Try a different SD card
4. Check serial console output (enable `enable_uart=1`)

### Display not working

1. Ensure correct overlay is uncommented in config.txt
2. Check display connections
3. For SPI displays, verify SPI is enabled (`dtparam=spi=on`)
4. Check power supply to display

### Camera not detected

1. Check ribbon cable connection
2. Ensure camera is enabled (`camera_auto_detect=1`)
3. Try manual overlay (e.g., `dtoverlay=imx219`)
4. Check CSI connector lock is engaged

### NPU not detected

1. Check PCIe connection
2. For USB devices, try different port
3. Check `lspci` output for device detection
4. Verify kernel module is loaded

## Building Custom Images

### Customize Partition Sizes

Edit `create_flashable_image.sh` and modify:

```bash
BOOT_SIZE=512      # Boot partition (MB)
SYSTEM_SIZE=4096   # System partition (MB)
VENDOR_SIZE=1024   # Vendor partition (MB)
PRODUCT_SIZE=512   # Product partition (MB)
USERDATA_SIZE=4096 # Userdata partition (MB)
```

### Include Custom Overlays

Place compiled `.dtbo` files in:
- `overlays/display/` - Display overlays
- `overlays/camera/` - Camera overlays
- `overlays/touch/` - Touch controller overlays

### Build with Custom Name

```bash
sudo ./scripts/create_flashable_image.sh MyCustomAndroid
```

## Support

- GitHub: https://github.com/klong4/AOSPI5
- Issues: https://github.com/klong4/AOSPI5/issues

## License

Apache License 2.0 - See LICENSE file for details.
