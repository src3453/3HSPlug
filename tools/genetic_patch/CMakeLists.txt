cmake_minimum_required(VERSION 3.22)
project(s3hs_patch_renderer LANGUAGES CXX)
get_filename_component(S3HS_CORE_DIR "../../src/s3hs_core" ABSOLUTE)
# The integer 3HS core vendored in src/s3hs_core/3hs, wrapped by sound.cpp.
file(GLOB S3HS_CORE_SOURCES
    "${S3HS_CORE_DIR}/3hs/3hs816/src/*.cpp"
    "${S3HS_CORE_DIR}/3hs/3hs88pwn4/src/*.cpp")
file(GLOB S3HS_CORE_HEADERS
    "${S3HS_CORE_DIR}/3hs/3hs816/include/hs816/*.hpp"
    "${S3HS_CORE_DIR}/3hs/3hs816/src/*.hpp"
    "${S3HS_CORE_DIR}/3hs/3hs88pwn4/include/hs88/*.hpp"
    "${S3HS_CORE_DIR}/3hs/3hs88pwn4/src/*.hpp")
list(SORT S3HS_CORE_SOURCES)
list(SORT S3HS_CORE_HEADERS)
set(S3HS_RENDERER_SOURCES
    "${S3HS_CORE_DIR}/sound.cpp"
    "${S3HS_CORE_DIR}/ram.cpp"
    "${S3HS_CORE_DIR}/header/spec.hpp"
    ${S3HS_CORE_SOURCES}
    ${S3HS_CORE_HEADERS}
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${S3HS_RENDERER_SOURCES})
set(source_hashes "")
foreach(source IN LISTS S3HS_RENDERER_SOURCES)
    file(SHA256 "${source}" source_hash)
    string(APPEND source_hashes "${source_hash}")
endforeach()
string(SHA256 S3HS_RENDERER_SOURCE_ID "${source_hashes}")
configure_file(renderer_build.h.in renderer_build.h @ONLY)
add_library(s3hs_patch_renderer SHARED renderer.cpp ${S3HS_CORE_SOURCES})
# Keep the ABI 2 import library separate from existing ABI 1 build artifacts.
set_target_properties(s3hs_patch_renderer PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/abi2-imports")
target_include_directories(s3hs_patch_renderer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    "${S3HS_CORE_DIR}/3hs/3hs816/include" "${S3HS_CORE_DIR}/3hs/3hs88pwn4/include")
target_compile_features(s3hs_patch_renderer PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(s3hs_patch_renderer PRIVATE /O2 /utf-8)
else()
    target_compile_options(s3hs_patch_renderer PRIVATE -O3)
endif()
