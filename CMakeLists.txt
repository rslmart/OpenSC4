cmake_minimum_required(VERSION 3.20)

if(APPLE AND NOT CMAKE_OSX_SYSROOT)
    if(EXISTS "/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk")
        set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk" CACHE PATH "macOS SDK" FORCE)
    elseif(EXISTS "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
        set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" CACHE PATH "macOS SDK" FORCE)
    endif()
endif()

project(OpenSC4 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPENSC4_BUILD_AUTHENTIC "Build Sub-Project 1: OpenSC4 Authentic (100% 1:1 SimCity 4 Deluxe Decompilation)" ON)
option(OPENSC4_BUILD_ENHANCED  "Build Sub-Project 2: OpenSC4 Enhanced (Modernized C++20, Performance & Toggleable Mods)" ON)
option(OPENSC4_BUILD_TESTS     "Build verification suites for enabled sub-projects" ON)

enable_testing()

if(OPENSC4_BUILD_AUTHENTIC)
    add_subdirectory(authentic)
endif()

if(OPENSC4_BUILD_ENHANCED)
    add_subdirectory(enhanced)
endif()

if(OPENSC4_BUILD_TESTS AND OPENSC4_BUILD_AUTHENTIC AND OPENSC4_BUILD_ENHANCED AND NOT EMSCRIPTEN)
    add_executable(opensc4_parity_digest_authentic tests/parity_digest.cpp)
    target_link_libraries(opensc4_parity_digest_authentic PRIVATE opensc4_authentic_core)

    add_executable(opensc4_parity_digest_enhanced tests/parity_digest.cpp)
    target_link_libraries(opensc4_parity_digest_enhanced PRIVATE opensc4_enhanced_core)
    target_compile_definitions(opensc4_parity_digest_enhanced PRIVATE OPENSC4_PARITY_ENHANCED=1)

    add_test(NAME OpenSC4AuthenticEditionParity
        COMMAND ${CMAKE_COMMAND}
            -DAUTHENTIC_DIGEST=$<TARGET_FILE:opensc4_parity_digest_authentic>
            -DENHANCED_DIGEST=$<TARGET_FILE:opensc4_parity_digest_enhanced>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_parity.cmake)
endif()
