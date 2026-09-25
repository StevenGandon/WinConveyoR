mkdir build\windows-amd64
pushd build\windows-amd64

cmake -G "MinGW Makefiles" ..\..\lib\libwconr -DCMAKE_TOOLCHAIN_FILE=..\..\toolchain\mingw_toolchain-amd64.cmake -DDIST_DIR=..\..\dist\windows-amd64 -DCMAKE_C_FLAGS="-m64" -DCMAKE_CXX_FLAGS="-m64" -DTARGET_ARCH="amd64" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DSANITIZE=OFF -DCMAKE_FIND_ROOT_PATH="C:/msys64/mingw64"

cmake --build . -- -j8
