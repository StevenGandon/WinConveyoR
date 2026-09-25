#!/bin/bash
set -e

mkdir -p build/macos-arm64
cd build/macos-arm64

cmake ../../lib/libwconr \
    -DDIST_DIR=../../dist/macos-arm64 \
    -DTARGET_ARCH="arm64" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DSANITIZE=OFF
cmake --build . -- -j$(sysctl -n hw.ncpu)