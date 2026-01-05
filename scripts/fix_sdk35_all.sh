#!/bin/bash
# Fix SDK version 35 references to 34 across entire AOSP tree

echo "Finding and fixing SDK version 35 references..."

# Search in key directories
for dir in hardware/interfaces frameworks system packages; do
    if [ -d ~/aosp-rpi5/$dir ]; then
        find ~/aosp-rpi5/$dir -name "Android.bp" 2>/dev/null | while read f; do
            if grep -q ": \"35\"" "$f" 2>/dev/null; then
                sed -i 's/: "35"/: "34"/g' "$f"
                echo "Fixed: $f"
            fi
        done
    fi
done

echo "Done!"
