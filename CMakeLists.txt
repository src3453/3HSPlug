cmake_minimum_required(VERSION 3.22)

project(juce_cmake_template VERSION 0.0.1)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building in Release mode")
    add_compile_options(/O2)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/utf-8)
endif()

add_subdirectory(lib/JUCE)

juce_add_plugin(3HSPlug

    COMPANY_NAME "src3453 @ KiTsuNeKo Works"
    COMPANY_COPYRIGHT "(C) 2025 src3453 licensed under AGPLv3"
    PLUGIN_MANUFACTURER_CODE KTNK
    PLUGIN_CODE 3HSP

    FORMATS Standalone
    PRODUCT_NAME "3HSPlug")

juce_generate_juce_header(3HSPlug)

target_sources(3HSPlug
    PRIVATE
        src/PluginEditor.cpp
        src/PluginProcessor.cpp
        src/PatchBankData.cpp
        src/DrumKeymapManager.cpp
        src/DrumPcmSampleLoader.cpp
    )
#        src/OscilloscopeComponent.cpp

target_compile_definitions(3HSPlug
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(3HSPlug
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)