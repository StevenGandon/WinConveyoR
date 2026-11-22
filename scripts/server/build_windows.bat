
set DIST_PATH=dist\windows-amd64\bin
set BUILD_PATH=build\windows-amd64\pyinstaller

mkdir %DIST_PATH%
mkdir %BUILD_PATH%

pyinstaller .\hosting\mirror-server\wcrmgr.py -c -F -n wcrmgr -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2
pyinstaller .\hosting\mirror-server\wcrmirror.py -c -F -n wcrmirror -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2
pyinstaller .\hosting\mirror-server\wcrpackager.py -c -F -n wcrpackager -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2
pyinstaller .\hosting\mirror-server\wcrgenkey.py -c -F -n wcrgenkey -y --distpath %DIST_PATH% --workpath %BUILD_PATH% --specpath %BUILD_PATH% --optimize 2
