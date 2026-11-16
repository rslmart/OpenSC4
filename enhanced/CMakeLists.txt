cmake_minimum_required(VERSION 3.20)

if(APPLE AND NOT CMAKE_OSX_SYSROOT)
    if(EXISTS "/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk")
        set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk" CACHE PATH "macOS SDK" FORCE)
    elseif(EXISTS "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
        set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" CACHE PATH "macOS SDK" FORCE)
    endif()
endif()

project(OpenSC4_Enhanced VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPENSC4_BUILD_TESTS "Build OpenSC4 Enhanced verification suite" ON)

# Sub-Project 2: OpenSC4 Enhanced (Modernized C++20 Architecture, Performance & Toggleable Mods)
add_library(opensc4_enhanced_core STATIC
    src/gzcom.cpp
    src/dbpf.cpp
    src/formats.cpp
    src/city.cpp
    src/tract_developer.cpp
    src/budget_simulator.cpp
    src/civic_simulators.cpp
    src/sc4_subfiles.cpp
    src/sc4_subfiles_batch2.cpp
    src/sc4_subfiles_batch3.cpp
    src/sc4_subfiles_automata.cpp
    src/sc4_save_registry.cpp
    src/decompilation_registry.cpp
    src/enhanced/enhanced_engine.cpp
    src/renderer/gl_city_renderer.cpp
)

target_include_directories(opensc4_enhanced_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(MSVC)
    target_compile_options(opensc4_enhanced_core PRIVATE /W4 /fp:precise)
else()
    target_compile_options(opensc4_enhanced_core PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()

if(EMSCRIPTEN)
    add_executable(opensc4_enhanced_wasm
        src/platform/wasm_main.cpp
    )
    target_link_libraries(opensc4_enhanced_wasm PRIVATE opensc4_enhanced_core)
    target_link_options(opensc4_enhanced_wasm PRIVATE
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free','_opensc4_wasm_load_sc4','_opensc4_wasm_step_sim','_opensc4_wasm_set_tool','_opensc4_wasm_set_overlay']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS']"
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/wasm_shell.html"
    )
    set_property(TARGET opensc4_enhanced_wasm APPEND PROPERTY
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/wasm_shell.html")
    set_target_properties(opensc4_enhanced_wasm PROPERTIES SUFFIX ".html")
elseif(APPLE)
    enable_language(OBJCXX)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)

    add_executable(opensc4_enhanced
        src/platform/main.cpp
        src/platform/http_server.cpp
        src/platform/macos_gl_app.mm
    )
    target_compile_definitions(opensc4_enhanced PRIVATE GL_SILENCE_DEPRECATION)
    target_compile_definitions(opensc4_enhanced_core PUBLIC GL_SILENCE_DEPRECATION)
    target_link_libraries(opensc4_enhanced_core PUBLIC ${OPENGL_LIBRARY})
    target_link_libraries(opensc4_enhanced PRIVATE
        opensc4_enhanced_core
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
    )

    add_executable(opensc4_render_snapshot tools/render_snapshot.mm)
    target_link_libraries(opensc4_render_snapshot PRIVATE opensc4_enhanced_core ${OPENGL_LIBRARY})
else()
    add_executable(opensc4_enhanced
        src/platform/main.cpp
        src/platform/http_server.cpp
    )
    target_link_libraries(opensc4_enhanced PRIVATE opensc4_enhanced_core)
endif()

if(OPENSC4_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    add_executable(opensc4_enhanced_verify
        tests/verify_sot.cpp
    )
    target_link_libraries(opensc4_enhanced_verify PRIVATE opensc4_enhanced_core)
    add_test(NAME OpenSC4EnhancedVerification COMMAND opensc4_enhanced_verify)
endif()
