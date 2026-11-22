
set DIST_PATH=dist\windows-amd64\bin
set BUILD_PATH=build\windows-amd64\pyinstaller

mkdir %DIST_PATH%
mkdir %BUILD_PATH%

pyinstaller .\tools\header_to_py.py -c -F -n header_gen -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2
