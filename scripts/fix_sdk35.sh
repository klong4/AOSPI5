#!/bin/bash
# Fix SDK version 35 references to 34 in AOSP

echo "Finding and fixing SDK version 35 references..."

find ~/aosp-rpi5/packages/modules -name "Android.bp" 2>/dev/null | while read f; do
    if grep -q ": \"35\"" "$f" 2>/dev/null; then
        sed -i 's/: "35"/: "34"/g' "$f"
        echo "Fixed: $f"
    fi
done

echo "Done!"
