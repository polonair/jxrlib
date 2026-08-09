@echo off
setlocal

set "PROFILE_DIR=%~dp0"
set "ENCODER=%~dp0..\jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe"
set "DECODER=%~dp0..\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe"
set "BMP=%PROFILE_DIR%test-sign-334x330.bmp"
set "JXR=%PROFILE_DIR%test-sign-334x330.jxr"
set "RESTORED_BMP=%PROFILE_DIR%test-sign-334x330-restored.bmp"

if not exist "%ENCODER%" (
  echo Encoder not found: %ENCODER%
  exit /b 1
)
if not exist "%DECODER%" (
  echo Decoder not found: %DECODER%
  exit /b 1
)

"%ENCODER%" -i "%BMP%" -o "%JXR%" -c 0 -d 3 -q 1 -l 0 -f -p
if errorlevel 1 exit /b %errorlevel%
"%DECODER%" -i "%JXR%" -o "%RESTORED_BMP%" -c 0 -a 0 -p 0
