cmake_minimum_required(VERSION 3.16)

project(SoLoud LANGUAGES CXX)

file(GLOB
  HeaderFiles
  "include/*.hpp"
)

file(GLOB
  SourceFiles
  "src/audiosource/*.*"
  "src/core/*.*"
  "src/filter/*.*"
)

add_library(SoLoud STATIC ${HeaderFiles} ${SourceFiles})

target_compile_features(SoLoud PRIVATE cxx_std_20)

target_include_directories(SoLoud PUBLIC include)

target_include_directories(SoLoud PRIVATE
  src/audiosource
  src/backend
  src/core
  src/filter
)

target_compile_definitions(SoLoud PRIVATE
  -DSTB_VORBIS_NO_STDIO
)

target_precompile_headers(SoLoud PRIVATE
  <cassert>
  <cstdio>
  <cstring>
  <cmath>
  <optional>
  <memory>
  <array>
  <stdexcept>
  <vector>
  <string>
  <span>
  <variant>
  include/soloud.hpp
  include/soloud_audiosource.hpp
  include/soloud_filter.hpp
  include/soloud_vec3.hpp
)

if (WIN32)
  target_compile_definitions(SoLoud PRIVATE
    -DWIN32_LEAN_AND_MEAN
    -DVC_EXTRA_LEAN
    -D_USE_MATH_DEFINES
  )
  target_precompile_headers(SoLoud PRIVATE
    <Windows.h>
  )
endif ()

set(SoLoud_BACKEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/backend/")

if (MSVC)
  target_compile_options(SoLoud PRIVATE /wd4700)
endif ()

if (NOT MSVC)
  set_source_files_properties(
    src/audiosource/speech/klatt.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-unused-value"
  )
endif ()

if (EMSCRIPTEN)
  target_compile_definitions(SoLoud PRIVATE -DWITH_SDL2_STATIC)
  target_sources(SoLoud PRIVATE ${SoLoud_BACKEND_DIR}/soloud_sdl2_static.cpp)

  set(LinkFlags "SHELL:-s USE_SDL=2")
  target_compile_options(SoLoud PRIVATE ${LinkFlags})
  target_link_options(SoLoud PRIVATE ${LinkFlags})
  target_link_libraries(SoLoud PRIVATE SDL2)
elseif (WIN32)
  target_compile_definitions(SoLoud PRIVATE -DWITH_WINMM)
  target_sources(SoLoud PRIVATE ${SoLoud_BACKEND_DIR}/soloud_winmm.cpp)
elseif (APPLE)
  find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
  find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)

  target_compile_definitions(SoLoud PRIVATE -DWITH_COREAUDIO)
  target_sources(SoLoud PRIVATE ${SoLoud_BACKEND_DIR}/soloud_coreaudio.cpp)

  target_link_libraries(SoLoud PUBLIC
    ${COREAUDIO_FRAMEWORK}
    ${AUDIOTOOLBOX_FRAMEWORK}
  )
elseif (ANDROID)
  target_compile_definitions(SoLoud PRIVATE -DWITH_OPENSLES)
  target_sources(SoLoud PRIVATE ${SoLoud_BACKEND_DIR}/soloud_opensles.cpp)

  find_library(OPENSLES_LIBRARY OpenSLES)
  target_link_libraries(SoLoud PUBLIC ${OPENSLES_LIBRARY})
elseif (UNIX)
  target_compile_definitions(SoLoud PRIVATE -DWITH_ALSA)
  target_sources(SoLoud PRIVATE ${SoLoud_BACKEND_DIR}/soloud_alsa.cpp)

  find_library(ALSA_LIBRARY asound REQUIRED)

  target_link_libraries(SoLoud PUBLIC ${ALSA_LIBRARY})
else ()
  message(FATAL_ERROR "Could not detect a proper backend for SoLoud!")
endif ()


add_executable(testbed main.cpp)
target_link_libraries(testbed PRIVATE SoLoud)
target_compile_features(testbed PRIVATE cxx_std_20)
