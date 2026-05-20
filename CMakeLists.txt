cmake_minimum_required(VERSION 3.15)
project(SDKForWindows2026 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

# ============================================================
# DemoSDK (C++ class version - links Controller.lib)
# ============================================================
add_executable(DemoSDK WIN32
    demo/DemoForSDK.cpp
    demo/stdafx.cpp
    demo/DemoForSDK.rc
)

target_include_directories(DemoSDK PRIVATE
    demo
    demo/lib
)

target_compile_definitions(DemoSDK PRIVATE
    WIN32
    _CONSOLE
    UNICODE
    _UNICODE
)

target_link_directories(DemoSDK PRIVATE
    demo/lib
)

target_link_libraries(DemoSDK PRIVATE
    Controller
    shlwapi
)

# Request admin elevation (UACExecutionLevel=2 in original vcproj)
set_target_properties(DemoSDK PROPERTIES
    LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
)

# ============================================================
# DemoSDK_C (C version - loads Controller_C.dll at runtime)
# ============================================================
add_executable(DemoSDK_C WIN32
    DemoSDK_C/DemoSDK_C.cpp
    DemoSDK_C/stdafx.cpp
)

target_include_directories(DemoSDK_C PRIVATE
    DemoSDK_C
)

target_compile_definitions(DemoSDK_C PRIVATE
    WIN32
    _CONSOLE
    UNICODE
    _UNICODE
)

set_target_properties(DemoSDK_C PROPERTIES
    LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
)
