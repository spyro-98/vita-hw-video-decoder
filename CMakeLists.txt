cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
      set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
          CACHE PATH "VitaSDK toolchain")
    else()
      message(FATAL_ERROR "VITASDK is required")
    endif()
  endif()
  project(vita_hw_video_decoder VERSION 3.0.0 LANGUAGES C)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(VITA_HW_VIDEO_DECODER_FFMPEG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/ffmpeg-vita-hw"
    CACHE PATH "Pinned FFmpeg h264_vita installation")
option(VITA_HW_VIDEO_DECODER_BUILD_EXAMPLE "Compile the local-file example" OFF)

if(NOT EXISTS "${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
  message(FATAL_ERROR
    "FFmpeg h264_vita was not found at ${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}. "
    "Run tools/build-ffmpeg.sh or set VITA_HW_VIDEO_DECODER_FFMPEG_ROOT.")
endif()

if(NOT TARGET VitaAudioDecoder::Interface)
  set(VITA_AUDIO_DECODER_INTERFACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../vita-hw-audio-decoder" CACHE PATH "Shared audio contracts")
  add_subdirectory("${VITA_AUDIO_DECODER_INTERFACE_ROOT}/interface" "${CMAKE_CURRENT_BINARY_DIR}/audio-interface")
endif()

add_library(vita_hw_video_decoder STATIC
  src/vita_hw_video_decoder.c
  src/file_stream.c
  src/module_log.c
  src/decoder_runtime.c
  src/internal/software_video.c)
add_library(VitaHwVideoDecoder::VitaHwVideoDecoder ALIAS vita_hw_video_decoder)
set_target_properties(vita_hw_video_decoder PROPERTIES
  EXPORT_NAME VitaHwVideoDecoder
  VERSION 3.0.0
  SOVERSION 3)

target_include_directories(vita_hw_video_decoder
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/include)
target_link_directories(vita_hw_video_decoder PUBLIC
  $<BUILD_INTERFACE:${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/lib>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}/vita-hw-video-decoder>)
target_link_libraries(vita_hw_video_decoder PUBLIC
  VitaAudioDecoder::Interface avformat avcodec avutil vita2d
  SceVideodec_stub SceCodecEngine_stub SceGxm_stub
  SceAppMgr_stub SceCommonDialog_stub SceDisplay_stub
  pthread m)
target_compile_features(vita_hw_video_decoder PUBLIC c_std_11)
target_compile_definitions(vita_hw_video_decoder PRIVATE
  VITA_HW_VIDEO_DECODER_HAS_H264_VITA=1
  VITA_HW_VIDEO_DECODER_ALLOW_SOFTWARE_FALLBACK=0)
set(VITA_HW_VIDEO_DECODER_CPU_THREADS "3" CACHE STRING "Experimental seekable H.264 CPU workers (1..3)")
if(NOT VITA_HW_VIDEO_DECODER_CPU_THREADS MATCHES "^[123]$")
  message(FATAL_ERROR "VITA_HW_VIDEO_DECODER_CPU_THREADS must be 1, 2 or 3")
endif()
target_compile_definitions(vita_hw_video_decoder PRIVATE SOFTWARE_VIDEO_DECODER_THREADS=${VITA_HW_VIDEO_DECODER_CPU_THREADS})
target_compile_options(vita_hw_video_decoder PRIVATE
  -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard
  -O3 -ffunction-sections -fdata-sections)

if(VITA_HW_VIDEO_DECODER_BUILD_EXAMPLE)
  add_library(vita_hw_video_decoder_local_example OBJECT examples/local_file.c)
  target_link_libraries(vita_hw_video_decoder_local_example PRIVATE VitaHwVideoDecoder::VitaHwVideoDecoder)
endif()

install(TARGETS vita_hw_video_decoder EXPORT VitaHwVideoDecoderTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/vita_hw_video_decoder.h include/vita_hw_video_decoder_live.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES
  "${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/lib/libavformat.a"
  "${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/lib/libavcodec.a"
  "${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/lib/libavutil.a"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/vita-hw-video-decoder)
install(EXPORT VitaHwVideoDecoderTargets NAMESPACE VitaHwVideoDecoder::
  FILE VitaHwVideoDecoderTargets.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VitaHwVideoDecoder)
configure_package_config_file(cmake/VitaHwVideoDecoderConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/VitaHwVideoDecoderConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VitaHwVideoDecoder)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/VitaHwVideoDecoderConfigVersion.cmake
  VERSION 3.0.0 COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/VitaHwVideoDecoderConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/VitaHwVideoDecoderConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VitaHwVideoDecoder)
install(FILES LICENSE THIRD_PARTY_NOTICES.md DEPENDENCIES.lock
  DESTINATION share/licenses/vita-hw-video-decoder)
install(DIRECTORY "${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/share/licenses/"
  DESTINATION share/licenses/vita-hw-video-decoder/third-party)
install(DIRECTORY "${VITA_HW_VIDEO_DECODER_FFMPEG_ROOT}/share/sources/"
  DESTINATION share/sources/vita-hw-video-decoder/third-party)
