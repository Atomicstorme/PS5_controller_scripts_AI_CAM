@echo off
echo PS5 Controller Scripts - Dependency Setup
echo ==========================================
echo.

:: Create libs directory structure
if not exist "libs" mkdir libs
if not exist "libs\imgui" mkdir libs\imgui
if not exist "libs\imgui\backends" mkdir libs\imgui\backends
if not exist "libs\hidapi" mkdir libs\hidapi
if not exist "libs\hidapi\include" mkdir libs\hidapi\include
if not exist "libs\hidapi\lib" mkdir libs\hidapi\lib
if not exist "libs\ViGEmClient" mkdir libs\ViGEmClient
if not exist "libs\ViGEmClient\include" mkdir libs\ViGEmClient\include
if not exist "libs\ViGEmClient\include\ViGEm" mkdir libs\ViGEmClient\include\ViGEm
if not exist "libs\ViGEmClient\lib" mkdir libs\ViGEmClient\lib
if not exist "libs\lua" mkdir libs\lua
if not exist "libs\lua\include" mkdir libs\lua\include

echo Directories created.
echo.
echo Please download the following dependencies manually:
echo.
echo 1. DEAR IMGUI (v1.90+)
echo    Download from: https://github.com/ocornut/imgui/releases
echo    Extract to: libs\imgui\
echo    Required files:
echo      - imgui.cpp, imgui.h, imgui_internal.h
echo      - imgui_demo.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp
echo      - imconfig.h, imstb_rectpack.h, imstb_textedit.h, imstb_truetype.h
echo      - backends\imgui_impl_win32.cpp, backends\imgui_impl_win32.h
echo      - backends\imgui_impl_dx11.cpp, backends\imgui_impl_dx11.h
echo.
echo 2. HIDAPI
echo    Download from: https://github.com/libusb/hidapi/releases
echo    Get the Windows release (hidapi-win.zip)
echo    Extract:
echo      - hidapi.h to libs\hidapi\include\
echo      - x64\hidapi.lib to libs\hidapi\lib\
echo      - x64\hidapi.dll to libs\hidapi\lib\ (will be copied to output)
echo.
echo 3. VIGEMCLIENT
echo    First install ViGEmBus driver: https://github.com/ViGEm/ViGEmBus/releases
echo    Download ViGEmClient: https://github.com/ViGEm/ViGEmClient/releases
echo    Extract:
echo      - include\ViGEm\Client.h to libs\ViGEmClient\include\ViGEm\
echo      - include\ViGEm\Common.h to libs\ViGEmClient\include\ViGEm\
echo      - lib\release\x64\ViGEmClient.lib to libs\ViGEmClient\lib\
echo      - bin\release\x64\ViGEmClient.dll to libs\ViGEmClient\lib\
echo.
echo 4. LUA 5.4
echo    Download from: https://luabinaries.sourceforge.net/download.html
echo    Get lua-5.4.x_Win64_dll17_lib.zip
echo    Extract:
echo      - lua54.dll to libs\lua\
echo      - lua54.lib to libs\lua\
echo      - include\*.h to libs\lua\include\
echo.
echo After downloading all dependencies, build with:
echo   mkdir build
echo   cd build
echo   cmake ..
echo   cmake --build . --config Release
echo.
pause
