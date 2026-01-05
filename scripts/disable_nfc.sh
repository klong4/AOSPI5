#!/bin/bash
# Disable NfcNci to avoid apex platform availability issue

cat > ~/aosp-rpi5/build/release/flag_values/ap2a/RELEASE_PACKAGE_NFC_STACK.textproto << 'EOF'
name: "RELEASE_PACKAGE_NFC_STACK"
value: {
  string_value: ""
}
EOF

echo "Created NFC stack override:"
cat ~/aosp-rpi5/build/release/flag_values/ap2a/RELEASE_PACKAGE_NFC_STACK.textproto
