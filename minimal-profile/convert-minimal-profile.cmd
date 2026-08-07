@echo off
setlocal

set "PROFILE_DIR=%~dp0"
set "ENCODER=%~dp0..\jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe"
set "DECODER=%~dp0..\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe"
set "BMP=%PROFILE_DIR%minimal-gray-16x16.bmp"
set "JXR=%PROFILE_DIR%minimal-gray-16x16.jxr"
set "RESTORED_BMP=%PROFILE_DIR%minimal-gray-16x16-restored.bmp"

if not exist "%ENCODER%" (
    echo Encoder not found: %ENCODER%
    exit /b 1
)

if not exist "%DECODER%" (
    echo Decoder not found: %DECODER%
    exit /b 1
)

"%ENCODER%" -i "%BMP%" -o "%JXR%" -c 2 -d 0 -q 1 -l 0 -f
if errorlevel 1 exit /b 1

"%DECODER%" -i "%JXR%" -o "%RESTORED_BMP%" -c 2 -a 0 -p 0
if errorlevel 1 exit /b 1

echo Created %JXR%
echo Created %RESTORED_BMP%
