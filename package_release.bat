@echo off
echo Packaging PS5 Controller Scripts for release...

set RELEASE_DIR=release
set BUILD_DIR=build\bin\Release

:: Clean and create release folder
if exist %RELEASE_DIR% rmdir /s /q %RELEASE_DIR%
mkdir %RELEASE_DIR%
mkdir %RELEASE_DIR%\scripts

:: Copy executable
copy %BUILD_DIR%\PS5ControllerScripts.exe %RELEASE_DIR%\

:: Copy required DLLs
copy %BUILD_DIR%\lua54.dll %RELEASE_DIR%\
copy %BUILD_DIR%\ViGEmClient.dll %RELEASE_DIR%\
copy libs\hidapi\lib\hidapi.dll %RELEASE_DIR%\

:: Copy scripts
copy scripts\*.lua %RELEASE_DIR%\scripts\

:: Copy readme
copy SETUP.txt %RELEASE_DIR%\README.txt

echo.
echo Release packaged to: %RELEASE_DIR%\
echo.
echo Contents:
dir /b %RELEASE_DIR%
echo.
echo Users only need to install ViGEmBus driver before running.
echo Download link: https://github.com/ViGEm/ViGEmBus/releases
pause
