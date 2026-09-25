
DIST_PATH=dist/linux-amd64/bin
BUILD_PATH=build/linux-amd64/pyinstaller

mkdir -p ${DIST_PATH}
mkdir -p ${BUILD_PATH}

pyinstaller ./tools/header_to_py.py \
            -c -F \
            -n header_gen -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2
