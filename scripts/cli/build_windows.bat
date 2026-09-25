
set DIST_PATH=dist\windows-amd64\bin
set BUILD_PATH=build\windows-amd64\pyinstaller

mkdir %DIST_PATH%
mkdir %BUILD_PATH%

pyinstaller .\cli\main.py -c -F -n wcr -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2 --add-data "./assets/cli:./assets/cli" --add-binary "dist/windows-amd64/lib/libwconr.dll:.
