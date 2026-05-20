cmake_minimum_required(VERSION 3.15)
project(SDKForWindows2026 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

# ============================================================
# DemoSDK (C++ class version - links Controller import lib)
# ============================================================
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

if(MINGW)
    # MinGW: generate import lib from Controller.dll using gendef + dlltool
    set(CONTROLLER_DLL "${CMAKE_SOURCE_DIR}/demo/lib/Controller.dll")
    set(CONTROLLER_DEF "${CMAKE_BINARY_DIR}/Controller.def")
    set(CONTROLLER_IMPLIB "${CMAKE_BINARY_DIR}/libController.a")

    add_custom_command(
        OUTPUT ${CONTROLLER_IMPLIB}
        COMMAND gendef ${CONTROLLER_DLL}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/Controller.def" ${CONTROLLER_DEF}
        COMMAND dlltool -d ${CONTROLLER_DEF} -l ${CONTROLLER_IMPLIB}
        DEPENDS ${CONTROLLER_DLL}
        COMMENT "Generating MinGW import library from Controller.dll"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    add_custom_target(gen_controller_lib DEPENDS ${CONTROLLER_IMPLIB})
    add_dependencies(DemoSDK gen_controller_lib)

    target_link_directories(DemoSDK PRIVATE
        ${CMAKE_BINARY_DIR}
    )

    target_link_libraries(DemoSDK PRIVATE
        Controller
        shlwapi
    )

    target_link_options(DemoSDK PRIVATE
        -municode
    )
else()
    # MSVC: use the existing .lib files
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
