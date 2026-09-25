
DIST_PATH=dist/linux-amd64/bin
BUILD_PATH=build/linux-amd64/pyinstaller

mkdir -p ${DIST_PATH}
mkdir -p ${BUILD_PATH}

pyinstaller ./hosting/mirror-server/wcrmgr.py \
            -c -F \
            -n wcrmgr -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2
pyinstaller ./hosting/mirror-server/wcrmirror.py \
            -c -F \
            -n wcrmirror -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2
pyinstaller ./hosting/mirror-server/wcrpackager.py \
            -c -F \
            -n wcrpackager -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2
pyinstaller ./hosting/mirror-server/wcrgenkey.py \
            -c -F \
            -n wcrgenkey -y --distpath ${DIST_PATH} \
            --workpath ${BUILD_PATH} \
            --specpath ${BUILD_PATH} --optimize 2
