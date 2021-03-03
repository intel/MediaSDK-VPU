##******************************************************************************
## Copyright (C) 2016-2019 Intel Corporation.  All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
## - Redistributions of source code must retain the above copyright notice,
## this list of conditions and the following disclaimer.
## - Redistributions in binary form must reproduce the above copyright notice,
## this list of conditions and the following disclaimer in the documentation
## and/or other materials provided with the distribution.
## - Neither the name of Intel Corporation nor the names of its contributors
## may be used to endorse or promote products derived from this software
## without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
## IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
## OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
## INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
## NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
## THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##******************************************************************************

cmake_minimum_required( VERSION 3.6 FATAL_ERROR )
project(dispatcher LANGUAGES CXX)

if( NOT MFX_API_FOLDER )
  set( MFX_API_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../../include )
endif()

# While equal to get_mfx_version in samples/builder, this function should remain separate to make this file self-sufficient
function( get_api_version mfx_version_major mfx_version_minor )
  file(STRINGS ${MFX_API_FOLDER}/mfxdefs.h major REGEX "#define MFX_VERSION_MAJOR" LIMIT_COUNT 1)
  file(STRINGS ${MFX_API_FOLDER}/mfxdefs.h minor REGEX "#define MFX_VERSION_MINOR" LIMIT_COUNT 1)
  string(REPLACE "#define MFX_VERSION_MAJOR " "" major ${major})
  string(REPLACE "#define MFX_VERSION_MINOR " "" minor ${minor})
  set(${mfx_version_major} ${major} PARENT_SCOPE)
  set(${mfx_version_minor} ${minor} PARENT_SCOPE)
endfunction()

set( CMAKE_LIB_DIR ${CMAKE_BINARY_DIR}/__bin )

set(no_warnings "-Wno-unknown-pragmas -Wno-unused")
set(warnings "-Wall -Wformat -Wformat-security")

list(APPEND sources
  include/mfxaudio_exposed_functions_list.h
  include/mfx_critical_section.h
  include/mfx_dispatcher_defs.h
  include/mfx_dispatcher.h
  include/mfx_dispatcher_log.h
  include/mfx_driver_store_loader.h
  include/mfx_dxva2_device.h
  include/mfx_exposed_functions_list.h
  include/mfx_library_iterator.h
  include/mfx_load_dll.h
  include/mfx_load_plugin.h
  include/mfx_plugin_hive.h
  include/mfx_vector.h
  include/mfx_win_reg_key.h
  
  src/main.cpp
  src/mfx_critical_section.cpp
  src/mfx_dispatcher.cpp
  src/mfx_dispatcher_log.cpp
  src/mfx_driver_store_loader.cpp
  src/mfx_dxva2_device.cpp
  src/mfx_function_table.cpp
  src/mfx_library_iterator.cpp
  src/mfx_load_dll.cpp
  src/mfx_load_plugin.cpp
  src/mfx_plugin_hive.cpp
  src/mfx_win_reg_key.cpp
)

add_library(mfx STATIC ${sources})
add_library(MFX::mfx ALIAS mfx)

set_property(TARGET mfx PROPERTY FOLDER "dispatcher")

target_link_libraries(mfx PRIVATE ${CMAKE_DL_LIBS})

target_compile_features(mfx PRIVATE cxx_std_11)

target_compile_options(mfx
  PRIVATE
   -W4
  )

target_include_directories(mfx 
  PUBLIC
    $<BUILD_INTERFACE:${MFX_API_FOLDER}>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    include
  )

get_api_version(MFX_VERSION_MAJOR MFX_VERSION_MINOR)

if (CMAKE_SYSTEM_NAME MATCHES Windows)
  # force correct name on Windows
  set_target_properties(mfx PROPERTIES PREFIX "lib")
  set_target_properties(mfx PROPERTIES IMPORT_PREFIX "lib")
endif()

set_target_properties( mfx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER mfx )
set_target_properties( mfx PROPERTIES   VERSION ${MFX_VERSION_MAJOR}.${MFX_VERSION_MINOR})
set_target_properties( mfx PROPERTIES SOVERSION ${MFX_VERSION_MAJOR})

export(TARGETS mfx NAMESPACE MFX:: FILE MfxTargets.cmake)

install(TARGETS mfx EXPORT MfxTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )

install(EXPORT MfxTargets
  FILE MfxTargets.cmake
  NAMESPACE MFX::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mfx
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/mfx/MfxConfigVersion.cmake"
  VERSION ${MFX_VERSION_MAJOR}.${MFX_VERSION_MINOR}
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT MfxTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/mfx/MfxTargets.cmake"
  NAMESPACE MFX::
)
configure_file(${MSDK_BUILDER_ROOT}/MfxConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/mfx/MfxConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/mfx)
install(EXPORT MfxTargets
  FILE
    MfxTargets.cmake
  NAMESPACE
    MFX::
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
    ${MSDK_BUILDER_ROOT}/MfxConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/mfx/MfxConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)

