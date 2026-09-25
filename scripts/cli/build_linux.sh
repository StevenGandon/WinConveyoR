
DIST_PATH=dist/linux-amd64/bin
BUILD_PATH=build/linux-amd64/pyinstaller

mkdir -p ${DIST_PATH}
mkdir -p ${BUILD_PATH}

if [ ! -f "dist/linux-amd64/lib/libwconr.so" ]; then
    echo "libwconr not found in linux amd64 distributables path."
    exit 1
fi

BINARY_ARGS=()
for f in dist/linux-amd64/lib/*.so; do
    BINARY_ARGS+=(--add-binary "../../../$f:.")
done

pyinstaller ./cli/main.py \
            -c -F \
            -n wcr -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2 --add-data "../../../assets/cli:./assets/cli" \
            "${BINARY_ARGS[@]}"
