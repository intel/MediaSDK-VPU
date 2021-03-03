# Copyright (c) 2018-2020 Intel Corporation
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

if (MFX_HW_VSI_TARGET AND (NOT MFX_VSI_MSDK_SHIM))
  return()
endif()

set (BYPASS_REPO "https://gitlab.devtools.intel.com/OWR/IoTG/GMS/Yocto/Graphics/Media/vaapi_bypass.git")
if (USE_SSH_FOR_BYPASS)
  set (BYPASS_REPO "ssh://git@gitlab.devtools.intel.com:29418/OWR/IoTG/GMS/Yocto/Graphics/Media/vaapi_bypass.git")
endif()

configure_file(CMakeLists.txt.in
               ${CMAKE_BINARY_DIR}/vaapi_bypass_windows-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/vaapi_bypass_windows-download )
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/vaapi_bypass_windows-download )

set (VA_HANTRO_H_SRC ${CMAKE_BINARY_DIR}/vaapi_bypass_windows-src)
file(COPY ${VA_HANTRO_H_SRC}/src/ext/va_hantro_kmb/va_hantro.h
        DESTINATION ${CMAKE_BINARY_DIR}/va_hantro)

if ( Windows )
  set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

  # =============================================================================

  mfx_include_dirs()

  include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
  include_directories( ${CMAKE_HOME_DIRECTORY}/contrib/xlink/include )
  include_directories( ${HDDL_ROOT}/include )

  #if (MFX_VSI_MSDK_SHIM)
 #   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vaapi_bypass_windows/cmake.win)
 # else()

    add_subdirectory(${CMAKE_BINARY_DIR}/vaapi_bypass_windows-src/cmake.win ${CMAKE_BINARY_DIR}/vaapi_bypass_windows-build)
  #endif()

  set( version_defs "" )
  set( defs "" )
  set( sources "" )
  set( sources.plus "" )
  set( LIBS "" )
  set( LIBS_PRIVATE "")

  file( GLOB srcs "${CMAKE_CURRENT_SOURCE_DIR}/va/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/va/*.cpp" )
  list( APPEND sources ${srcs})

  list( APPEND sources $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/libva-kmb/libva-kmb.def>)

  list( APPEND LIBS_PRIVATE ${CMAKE_HOME_DIRECTORY}/contrib/xlink/lib/XLink_DLL.lib vaapi_bypass )

  if (MFX_VSI_HDDL)
    list( APPEND LIBS ${HDDLUNITE_LIB} )
    set( defs "${defs} -DHDDL_UNITE -DIA " )
  endif()

  ## plugin info: name, description and version
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( libva_name libva-kmb )
  else()
    set( libva_name libva-kmb32 )
  endif()

  ## defs
  set( USE_STRICT_NAME TRUE )
  set( defs "${defs} -DWIN_HOST -DUSE_HANTRO " )

  set(MFX_LIBVA_NAME ${libva_name} PARENT_SCOPE)

  make_library( ${libva_name} none shared)
  install( TARGETS ${libva_name} LIBRARY DESTINATION ${MFX_MODULES_DIR} )

  add_dependencies(${libva_name} vaapi_bypass)
endif()
