cmake_minimum_required(VERSION 3.22)
project(s3hs_core_tests LANGUAGES CXX)
enable_testing()
set(core_dir "${CMAKE_CURRENT_SOURCE_DIR}/../src/s3hs_core/3hs")
add_executable(s3hs_core_tests s3hs_core_tests.cpp
    "${core_dir}/3hs816/src/channel.cpp"
    "${core_dir}/3hs816/src/engine.cpp"
    "${core_dir}/3hs816/src/envelope.cpp"
    "${core_dir}/3hs816/src/registers.cpp"
    "${core_dir}/3hs816/src/wavetables.cpp"
    "${core_dir}/3hs88pwn4/src/biquad.cpp"
    "${core_dir}/3hs88pwn4/src/engine.cpp"
    "${core_dir}/3hs88pwn4/src/master_effects.cpp"
    "${core_dir}/3hs88pwn4/src/pcm_channel.cpp"
    "${core_dir}/3hs88pwn4/src/registers.cpp"
)
target_include_directories(s3hs_core_tests PRIVATE "${core_dir}/3hs816/include" "${core_dir}/3hs88pwn4/include")
target_compile_features(s3hs_core_tests PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(s3hs_core_tests PRIVATE /utf-8)
endif()
add_test(NAME s3hs_core_tests COMMAND s3hs_core_tests)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/JUCE/modules/juce_core/juce_core.cpp")
    set(juce_modules "${CMAKE_CURRENT_SOURCE_DIR}/../lib/JUCE/modules")
    add_executable(patch_bank_tests patch_bank_tests.cpp ../src/PatchBankData.cpp
        "${juce_modules}/juce_core/juce_core.cpp"
        "${juce_modules}/juce_core/juce_core_CompilationTime.cpp")
    target_compile_features(patch_bank_tests PRIVATE cxx_std_17)
    target_include_directories(patch_bank_tests PRIVATE "${juce_modules}")
    target_compile_definitions(patch_bank_tests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_USE_CURL=0)
    if(MSVC)
        target_compile_options(patch_bank_tests PRIVATE /utf-8 /bigobj)
    endif()
    add_test(NAME patch_bank_tests COMMAND patch_bank_tests)
endif()
