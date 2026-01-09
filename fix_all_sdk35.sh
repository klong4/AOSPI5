#!/bin/bash
# Fix all SDK version 35 references to 34
export PATH=/usr/bin:/bin:/usr/sbin:/sbin:$PATH

echo "Fixing SDK 35 references in AOSP..."

# Find and fix all Android.bp files with SDK version 35
for dir in /root/aosp-rpi5/packages /root/aosp-rpi5/hardware /root/aosp-rpi5/system /root/aosp-rpi5/vendor /root/aosp-rpi5/frameworks /root/aosp-rpi5/external /root/aosp-rpi5/device /root/aosp-rpi5/cts /root/aosp-rpi5/tools /root/aosp-rpi5/bionic /root/aosp-rpi5/build; do
    if [ -d "$dir" ]; then
        echo "Scanning $dir..."
        find "$dir" -name "Android.bp" -type f 2>/dev/null | while read f; do
            if grep -q ': "35"' "$f" 2>/dev/null; then
                sed -i 's/: "35"/: "34"/g' "$f"
                echo "Fixed: $f"
            fi
        done
    fi
done

echo "Done fixing SDK 35 references!"
