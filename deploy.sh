#!/bin/bash

# Usage:
#   ./deploy.sh {device} {path/to/uf2}
# 

pico_path=/Volumes/RPI-RP2

if [[ -z ${2} || ${2##*.} != "uf2" ]]; then
    echo "[ERROR] No .uf2 file specified"
    exit 1
fi

if [ -z ${1} ]; then
    echo "[ERROR] No device specified"
    exit 1
fi

# Put the Pico onto BOOTSEL mode
stty -f ${1} 1200
echo "Waiting for Pico to mount..."
while [ ! -d ${pico_path} ]; do
    sleep 0.1
done
sleep 0.5

# Copy the target file
echo "Copying ${2} to ${1}..."
cp ${2} ${pico_path}
echo Done