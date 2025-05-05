#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONFIG_PATH="${SCRIPT_DIR}/child_image/mcuboot.conf"

if [ $# -ne 1 ]; then
    echo "This script sets up the MCUboot configuration."
    echo
    echo "Usage: $0 <path_to_private_key>"
    echo
    echo "Steps:"
    echo "  1. Provide the path to an existing private key file (in PEM format)."
    echo "  2. If you don't have one, follow the guide to generate it using OpenSSL:"
    echo "     https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/bootloaders_dfu/mcuboot_nsib/bootloader_signature_keys.html#ug-fw-update-keys-openssl"
    exit 1
fi

KEY_FILE="$1"
DEST_KEY="${SCRIPT_DIR}/priv.pem"

if [ ! -f "$KEY_FILE" ]; then
    echo "Error: File '$KEY_FILE' not found."
    exit 1
fi

# Avoid copying if source and destination are the same file
if [ "$(realpath "$KEY_FILE")" != "$(realpath "$DEST_KEY")" ]; then
    cp "$KEY_FILE" "$DEST_KEY"
    echo "Copied key to ${DEST_KEY}"
else
    echo "Key file is already at the destination (${DEST_KEY}), skipping copy."
fi

echo "Creating mcuboot config at $CONFIG_PATH"
mkdir -p "$(dirname "$CONFIG_PATH")"
echo "CONFIG_BOOT_SIGNATURE_KEY_FILE=\"${DEST_KEY}\"" > "$CONFIG_PATH"
