
set DIST_PATH=dist\windows-amd64\bin
set BUILD_PATH=build\windows-amd64\pyinstaller
setlocal EnableDelayedExpansion

mkdir %DIST_PATH%
mkdir %BUILD_PATH%

if not exist "dist/windows-amd64/lib/libwconr.dll" (
    echo libwconr not found in windows amd64 distributables path.
    exit 1
)

set "BINARY_ARGS="

for %%F in ("dist\windows-amd64\lib\*.dll") do (
    set "BINARY_ARGS=!BINARY_ARGS! --add-binary "..\..\..\%%F:.""
)

pyinstaller .\daemon\main.py -c -F -n wcr-daemon -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2 !BINARY_ARGS!
endlocal
