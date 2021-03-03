# Copyright (c) 2020 Intel Corporation. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(CMAKE_VERBOSE_MAKEFILE on)

if (NOT MFX_VSI_MSDK_SHIM)
  return()
endif()

option(MFX_SHIM_TARGET "Build for VSI ARM?" ${MFX_HW_VSI_TARGET})

set (COMM_FOLDER ${CMAKE_BINARY_DIR}/vaapi_bypass_windows-src/src)

include_directories(
    common
    ${COMM_FOLDER}
    ${COMM_FOLDER}/host
    ${COMM_FOLDER}/target
    ${COMM_FOLDER}/util
    ${COMM_FOLDER}/common
    ${COMM_FOLDER}/common/xlink
    ${COMM_FOLDER}/common/tcp
    ${COMM_FOLDER}/common/unite
    ${COMM_FOLDER}/common/va
    ${CMAKE_CURRENT_SOURCE_DIR}/../api/vpl/
    ${CMAKE_CURRENT_SOURCE_DIR}/../api/
    ${HDDLUNITE_INCLUDE_DIR}
)

if (Windows)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../api/dispatcher/windows)
else()
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../api/dispatcher)
endif()

set( defs "" )
set( sources "" )
set( sources.plus "" )
set( LIBS "" )

set( defs "${defs} -DMFX_HW_KMB -DLIBVA_SUPPORT -DUSE_HANTRO " )

if (MFX_SHIM_TARGET)
  set( defs "${defs} -DMFX_HW_KMB_TARGET " )  
else ()
  if (Windows)
    set( defs "${defs} -DWIN_HOST " )
  endif()

  if (MFX_SHIM_WRAPPER)
    set( defs "${defs} -DMFX_SHIM_WRAPPER " )
  endif()
endif ()

set( defs "${defs} -DMFX_MSDK_SHIM " )


if( NOT DEFINED MFX_PLUGINS_CONF_DIR )
  set( MFX_PLUGINS_CONF_DIR ${CMAKE_INSTALL_FULL_DATADIR}/mfx )
endif( )
add_definitions( -DMFX_PLUGINS_CONF_DIR="${MFX_PLUGINS_CONF_DIR}" )
message( STATUS "MFX_PLUGINS_CONF_DIR=${MFX_PLUGINS_CONF_DIR}" )

if( NOT DEFINED MFX_MODULES_DIR )
  set( MFX_MODULES_DIR ${CMAKE_INSTALL_FULL_LIBDIR} )
endif( )
add_definitions( -DMFX_MODULES_DIR="${MFX_MODULES_DIR}" )
message( STATUS "MFX_MODULES_DIR=${MFX_MODULES_DIR}" )

list( APPEND sources 
  common/comm_helper.cpp
  common/hddl_mfx_common.cpp
)

if (NOT MFX_SHIM_TARGET)
  include_directories(${MSDK_STUDIO_ROOT}/shared/include)

  list( APPEND sources 
      host/host_mfx_shim.cpp
      common/hddl_mfx_common.h
      common/comm_helper.h
      target/mfx_payload.h
      ${MSDK_STUDIO_ROOT}/shared/src/libmfx_allocator_hddl.cpp
  )

  if (NOT MFX_ONEVPL)
    list( APPEND sources 
        msdk_bypass.def
    )
  else()
    list( APPEND sources 
        msdk_bypass_onevpl.def
    )
  endif()

  if (MFX_SHIM_WRAPPER)
    include_directories(target)
    list( APPEND sources target/mfx_payload.cpp)
  endif()
else()
  list( APPEND sources
      ${COMM_FOLDER}/common/xlink/xlink_pcie.c
      ${COMM_FOLDER}/common/tcp/tcp_placeholders.c
      ${COMM_FOLDER}/common/unite/unite.c
      ${COMM_FOLDER}/common/gen_comm.c
      ${COMM_FOLDER}/common/thread_manager.c
      ${COMM_FOLDER}/common/memory_manager.c

      ${COMM_FOLDER}/target/target_va_shim.c
      ${COMM_FOLDER}/target/payload.c
      ${COMM_FOLDER}/common/va/va_display_drm.c
      ${COMM_FOLDER}/common/va/va_display.c
      target/target_mfx_shim.cpp
      target/mfx_payload.cpp
  )

  set( defs "${defs} -DACCEL " )
endif()

set(DEPENDENCIES libva dl pthread hddl)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif ()

include_directories (${COMM_FOLDER}/ext/va_hantro_kmb)

if (Windows)
  list( APPEND LIBS ${CMAKE_HOME_DIRECTORY}/contrib/xlink/lib/XLink_DLL.lib )
  include_directories( ${CMAKE_HOME_DIRECTORY}/contrib/xlink/include )
  include_directories( ${CMAKE_HOME_DIRECTORY}/contrib/hddl/include )
  make_library( ${MFX_LIBNAME} none shared)
else()
  include (${CMAKE_BINARY_DIR}/vaapi_bypass_windows-src/cmake/xlink_include.cmake)
  include (${CMAKE_BINARY_DIR}/vaapi_bypass_windows-src/cmake/safestringlib_include.cmake)
  FindXLink ()                   	
  FindSafeStringLib ()

  list( APPEND LIBS ${XLINK_LIB} libsafestring)
  set(DEPENDENCIES ${DEPENDENCIES} libva-drm)

  if (MFX_SHIM_TARGET)    
    list( APPEND LIBS mfxhw64 )
    make_executable( "hddl_msdk_bypass" universal )
  else()
    list( APPEND LIBS mfx )
    make_library( ${MFX_LIBNAME} none shared)
  endif()
endif ()
