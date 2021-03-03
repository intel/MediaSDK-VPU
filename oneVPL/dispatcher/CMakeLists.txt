# ##############################################################################
# Copyright (C) 2020 Intel Corporation
#
# SPDX-License-Identifier: MIT
# ##############################################################################
cmake_minimum_required(VERSION 3.10.2)

include_directories( ${MFX_API_HOME} )

# While equal to get_mfx_version in samples/builder, this function should remain separate to make this file self-sufficient
function( get_api_version mfx_version_major mfx_version_minor )
  file(STRINGS ${MFX_INCLUDE}/mfxdefs.h major REGEX "#define MFX_VERSION_MAJOR" LIMIT_COUNT 1)
  file(STRINGS ${MFX_INCLUDE}/mfxdefs.h minor REGEX "#define MFX_VERSION_MINOR" LIMIT_COUNT 1)
  string(REPLACE "#define MFX_VERSION_MAJOR " "" major ${major})
  string(REPLACE "#define MFX_VERSION_MINOR " "" minor ${minor})
  set(${mfx_version_major} ${major} PARENT_SCOPE)
  set(${mfx_version_minor} ${minor} PARENT_SCOPE)
endfunction()

set(TARGET mfx)

if (CMAKE_SYSTEM_NAME MATCHES Windows)
  set(SOURCES
      windows/main.cpp
      windows/mfx_critical_section.cpp
      windows/mfx_dispatcher.cpp
      windows/mfx_dispatcher_log.cpp
      windows/mfx_dxva2_device.cpp
      windows/mfx_function_table.cpp
      windows/mfx_library_iterator.cpp
      windows/mfx_load_dll.cpp
      windows/mfx_win_reg_key.cpp
      windows/libmfx.def)
endif()

if (CMAKE_SYSTEM_NAME MATCHES Linux)
  set(SOURCES linux/mfxloader.cpp)

  if(NOT DEFINED MFX_MODULES_DIR)
    set(MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
  endif()
  add_definitions(-DMFX_MODULES_DIR="${MFX_MODULES_DIR}")
  message(STATUS "MFX_MODULES_DIR=${MFX_MODULES_DIR}")
endif()

list(APPEND SOURCES vpl/mfx_dispatcher_vpl.cpp
     vpl/mfx_dispatcher_vpl_loader.cpp vpl/mfx_dispatcher_vpl_config.cpp)

add_library(${TARGET} SHARED "")

get_api_version(API_VERSION_MAJOR API_VERSION_MINOR)

if (CMAKE_SYSTEM_NAME MATCHES Windows)
  # force correct name on Windows
  set_target_properties(${TARGET} PROPERTIES PREFIX "lib")
  set_target_properties(${TARGET} PROPERTIES IMPORT_PREFIX "lib")
endif()
set_target_properties(${TARGET} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${TARGET} PROPERTIES SOVERSION ${API_VERSION_MAJOR})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} )

if (CMAKE_SYSTEM_NAME MATCHES Linux)
  set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} )
endif()

target_sources(${TARGET} PRIVATE ${SOURCES})
target_link_libraries(
  ${TARGET}
 PRIVATE ${CMAKE_DL_LIBS})

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                             ${CMAKE_CURRENT_BINARY_DIR})