cmake_minimum_required(VERSION 3.22)

project(juce_cmake_template VERSION 1.1.0)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    message(STATUS "Building in Debug mode")
#elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
#    message(STATUS "Building in Release mode")
#    add_compile_options(/O2)
#endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/utf-8)
endif()

add_subdirectory(lib/JUCE)

set (BUILD_DATE "$<IF:$<CONFIG:Debug>,$<TIMESTAMP:%Y-%m-%d %H:%M:%S>,$<DATE:%Y-%m-%d %H:%M:%S>>>")
configure_file(src/config.h.in config.h @ONLY)

# Version shown in the title bar: project(VERSION) + short git hash (if available).
set(THREEHS_GIT_HASH "")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE THREEHS_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()

juce_add_plugin(3HSPlug

    COMPANY_NAME "src3453 @ KiTsuNeKo Works"
    PLUGIN_MANUFACTURER_CODE Ktnk
    PLUGIN_CODE 3Hsp
    BUNDLE_ID com.src3453.3hsplug
    VST3_CATEGORIES Instrument
    FORMATS Standalone VST3 AU
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PRODUCT_NAME "3HSPlug")

juce_generate_juce_header(3HSPlug)

# UIで選べる同梱フォント（どちらも作者の自作フォント）
juce_add_binary_data(3HSPlugUiFonts
    HEADER_NAME UiFontsData.h
    NAMESPACE UiFontsData
    SOURCES assets/fonts/azurzulu.ttf assets/fonts/f68.ttf)

target_sources(3HSPlug
    PRIVATE
        src/PluginEditor.cpp
        src/ui/Panels.cpp
        src/ui/ChannelStatusPanel.cpp
        src/ui/ScopePanel.cpp
        src/ui/PatchEditorPanel.cpp
        src/ui/DrumKitPanel.cpp
        src/ui/AlgorithmDiagram.cpp
        src/ui/Fonts.cpp
        src/ui/Themes.cpp
        src/PluginProcessor.cpp
        src/PatchBankData.cpp
        src/DrumKeymapManager.cpp
        src/DrumKit.cpp
        src/DrumPcmSampleLoader.cpp
        src/CommandLineArgs.cpp
        # integer 3HS core (vendored from CPT32, see src/s3hs_core/3hs/README.md)
        src/s3hs_core/3hs/3hs816/src/channel.cpp
        src/s3hs_core/3hs/3hs816/src/engine.cpp
        src/s3hs_core/3hs/3hs816/src/envelope.cpp
        src/s3hs_core/3hs/3hs816/src/registers.cpp
        src/s3hs_core/3hs/3hs816/src/wavetables.cpp
        src/s3hs_core/3hs/3hs88pwn4/src/biquad.cpp
        src/s3hs_core/3hs/3hs88pwn4/src/engine.cpp
        src/s3hs_core/3hs/3hs88pwn4/src/master_effects.cpp
        src/s3hs_core/3hs/3hs88pwn4/src/pcm_channel.cpp
        src/s3hs_core/3hs/3hs88pwn4/src/registers.cpp
    )

target_include_directories(3HSPlug
    PRIVATE
        src/s3hs_core/3hs/3hs816/include
        src/s3hs_core/3hs/3hs88pwn4/include)
#        src/OscilloscopeComponent.cpp

target_compile_definitions(3HSPlug
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        THREEHS_VERSION="${PROJECT_VERSION}"
        THREEHS_GIT_HASH="${THREEHS_GIT_HASH}")

target_link_libraries(3HSPlug
    PRIVATE
        3HSPlugUiFonts
        juce::juce_audio_utils
    PUBLIC
        juce::juce_audio_plugin_client
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

option(THREEHS_BUILD_UI_TESTS "Build MIDI monitor and editor smoke tests" OFF)
if(THREEHS_BUILD_UI_TESTS)
    enable_testing()
    add_executable(channel_status_tests test/channel_status_tests.cpp)
    target_include_directories(channel_status_tests PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/3HSPlug_artefacts/JuceLibraryCode")
    target_include_directories(channel_status_tests PRIVATE src/s3hs_core/3hs/3hs816/include src/s3hs_core/3hs/3hs88pwn4/include)
    target_link_libraries(channel_status_tests PRIVATE 3HSPlug)
    add_test(NAME channel_status_tests COMMAND channel_status_tests)
    set_tests_properties(channel_status_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()


