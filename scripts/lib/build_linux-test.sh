#!/bin/bash
set -e

mkdir -p build/linux-amd64-test
cd build/linux-amd64-test

cmake ../../lib/libwconr \
    -DDIST_DIR=../../dist/linux-amd64-test \
    -DCMAKE_C_FLAGS="-m64" \
    -DCMAKE_CXX_FLAGS="-m64" \
    -DTARGET_ARCH="amd64" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DSANITIZE=OFF \
    -DBUILD_TESTS=ON

cmake --build . -- -j$(nproc)

ctest --test-dir . --output-on-failure

cd ../../