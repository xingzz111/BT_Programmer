cmake_minimum_required(VERSION 3.15)
project(SDKForWindows2026 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

# ============================================================
# DemoSDK (C++ class version - links MSVC Controller.lib)
# Only buildable with MSVC: Controller.dll uses MSVC C++ ABI.
# ============================================================
if(MSVC)
    add_executable(DemoSDK WIN32
        demo/DemoForSDK.cpp
        demo/stdafx.cpp
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

    set_target_properties(DemoSDK PROPERTIES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()

# ============================================================
# DemoSDK_C (C version - loads Controller_C.dll at runtime)
# Cross-compiles with MinGW; no static link to MSVC libs.
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

if(MINGW)
    target_link_options(DemoSDK_C PRIVATE
        -municode
    )
else()
    set_target_properties(DemoSDK_C PROPERTIES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()
