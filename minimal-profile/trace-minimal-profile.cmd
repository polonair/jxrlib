@echo off
setlocal

set "PROFILE_DIR=%~dp0"
set "TRACE_DIR=%PROFILE_DIR%trace"
set "ENCODER=%~dp0..\jxrencoderdecoder\Release\JXREncApp\x64\JXREncApp.exe"
set "DECODER=%~dp0..\jxrencoderdecoder\Release\JXRDecApp\x64\JXRDecApp.exe"

if not exist "%ENCODER%" exit /b 1
if not exist "%DECODER%" exit /b 1
if not exist "%TRACE_DIR%" mkdir "%TRACE_DIR%"

"%ENCODER%" -i "%PROFILE_DIR%minimal-gray-16x16.bmp" -o "%PROFILE_DIR%minimal-gray-16x16.jxr" -c 2 -d 0 -q 1 -l 0 -f -X "%TRACE_DIR%"
if errorlevel 1 exit /b 1

"%DECODER%" -i "%PROFILE_DIR%minimal-gray-16x16.jxr" -o "%PROFILE_DIR%minimal-gray-16x16-restored.bmp" -c 2 -a 0 -p 0 -X "%TRACE_DIR%"
