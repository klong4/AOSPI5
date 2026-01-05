#!/usr/bin/env python3
"""Fix virtualization module by replacing missing kernel prebuilt references with empty_file"""

filepath = "/home/klong/aosp-rpi5/packages/modules/Virtualization/build/microdroid/Android.bp"

with open(filepath, "r") as f:
    content = f.read()

# Replace all kernel prebuilt references with empty_file
replacements = [
    ('":microdroid_kernel_prebuilt-arm64"', '":empty_file"'),
    ('":microdroid_kernel_prebuilt-x86_64"', '":empty_file"'),
    ('":microdroid_gki_kernel_prebuilts-android15-6.6-arm64"', '":empty_file"'),
    ('":microdroid_gki_kernel_prebuilts-android15-6.6-x86_64"', '":empty_file"'),
    ('":microdroid_gki_kernel_prebuilts-android16-6.12-arm64"', '":empty_file"'),
    ('":microdroid_gki_kernel_prebuilts-android16-6.12-x86_64"', '":empty_file"'),
]

for old, new in replacements:
    if old in content:
        content = content.replace(old, new)
        print(f"Replaced: {old} -> {new}")

with open(filepath, "w") as f:
    f.write(content)

print("Done!")
