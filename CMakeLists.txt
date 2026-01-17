cmake_minimum_required(VERSION 3.16)
project(PS5ControllerScripts VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# ============================================================================
# LUA LIBRARY (built from source)
# ============================================================================
set(LUA_SRC_DIR ${CMAKE_SOURCE_DIR}/libs/lua-src/src)

set(LUA_SOURCES
    ${LUA_SRC_DIR}/lapi.c
    ${LUA_SRC_DIR}/lauxlib.c
    ${LUA_SRC_DIR}/lbaselib.c
    ${LUA_SRC_DIR}/lcode.c
    ${LUA_SRC_DIR}/lcorolib.c
    ${LUA_SRC_DIR}/lctype.c
    ${LUA_SRC_DIR}/ldblib.c
    ${LUA_SRC_DIR}/ldebug.c
    ${LUA_SRC_DIR}/ldo.c
    ${LUA_SRC_DIR}/ldump.c
    ${LUA_SRC_DIR}/lfunc.c
    ${LUA_SRC_DIR}/lgc.c
    ${LUA_SRC_DIR}/linit.c
    ${LUA_SRC_DIR}/liolib.c
    ${LUA_SRC_DIR}/llex.c
    ${LUA_SRC_DIR}/lmathlib.c
    ${LUA_SRC_DIR}/lmem.c
    ${LUA_SRC_DIR}/loadlib.c
    ${LUA_SRC_DIR}/lobject.c
    ${LUA_SRC_DIR}/lopcodes.c
    ${LUA_SRC_DIR}/loslib.c
    ${LUA_SRC_DIR}/lparser.c
    ${LUA_SRC_DIR}/lstate.c
    ${LUA_SRC_DIR}/lstring.c
    ${LUA_SRC_DIR}/lstrlib.c
    ${LUA_SRC_DIR}/ltable.c
    ${LUA_SRC_DIR}/ltablib.c
    ${LUA_SRC_DIR}/ltm.c
    ${LUA_SRC_DIR}/lundump.c
    ${LUA_SRC_DIR}/lutf8lib.c
    ${LUA_SRC_DIR}/lvm.c
    ${LUA_SRC_DIR}/lzio.c
)

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${LUA_SRC_DIR})

# ============================================================================
# VIGEMCLIENT LIBRARY (built from source)
# ============================================================================
set(VIGEM_SRC_DIR ${CMAKE_SOURCE_DIR}/libs/ViGEmClient)

add_library(ViGEmClient STATIC
    ${VIGEM_SRC_DIR}/src/ViGEmClient.cpp
)
target_include_directories(ViGEmClient PUBLIC
    ${VIGEM_SRC_DIR}/include
    ${VIGEM_SRC_DIR}/src
)
target_link_libraries(ViGEmClient setupapi)

# ============================================================================
# IMGUI LIBRARY
# ============================================================================
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# ============================================================================
# MAIN APPLICATION
# ============================================================================
set(APP_SOURCES
    src/main.cpp
    src/Application.cpp
    src/DualSenseController.cpp
    src/VirtualController.cpp
    src/ScriptEngine.cpp
    src/ScriptManager.cpp
    src/InputProcessor.cpp
    src/GUI.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/hidapi/include
)

target_link_libraries(${PROJECT_NAME}
    lua
    ViGEmClient
    imgui
    d3d11
    dxgi
    d3dcompiler
    setupapi
    hid
    ${CMAKE_SOURCE_DIR}/libs/hidapi/lib/hidapi.lib
)

# Copy hidapi DLL to output
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/libs/hidapi/lib/hidapi.dll"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Copy scripts folder to output
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/scripts"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/scripts"
)

# ============================================================================
# INSTALL / PACKAGING
# ============================================================================
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/libs/hidapi/lib/hidapi.dll DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/SETUP.txt DESTINATION . RENAME README.txt)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "PS5ControllerScripts")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
