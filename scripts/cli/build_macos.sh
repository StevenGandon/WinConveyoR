DIST_PATH=dist/macos-arm64/bin
BUILD_PATH=build/macos-arm64/pyinstaller

mkdir -p ${DIST_PATH}
mkdir -p ${BUILD_PATH}

if [ ! -f "dist/macos-arm64/lib/libwconr.dylib" ]; then
    echo "libwconr not found in macos arm64 distributables path."
    exit 1
fi

pyinstaller ./cli/main.py \
            -c -F \
            -n wcr -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2 --add-data "./assets/cli:./assets/cli" --add-binary "dist/macos-arm64/lib/libwconr.dylib:."
