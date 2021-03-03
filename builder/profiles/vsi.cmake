# Copyright (c) 2018-2019 Intel Corporation
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


set(CMAKE_VERBOSE_MAKEFILE ON)

option(MFX_HW_VSI_TARGET "Build for VSI ARM?" OFF)
option(MFX_HW_VSI "Build for VSI Host?" ON)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  set(MFX_HW_VSI_TARGET ON)
endif()

add_definitions( -DMFX_VA )
add_definitions(-DMFX_HW_KMB)
append(" -DVA_RT_FORMAT_YUV420_10=0x00000100 -DVA_ENC_SLICE_STRUCTURE_EQUAL_MULTI_ROWS=0x00000020 " CMAKE_CXX_FLAGS)

if (MFX_HW_VSI_TARGET)
  append(" -DMFX_HW_VSI_TARGET" CMAKE_CXX_FLAGS)
endif()

if ( Linux )
  # by default char is unsigned on yocto arm.
  append(" -Wno-narrowing -fsigned-char " CMAKE_CXX_FLAGS)
endif()

if ( Windows )
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( MFX_LIBNAME "libmfxhw64" )
  else()
    set( MFX_LIBNAME "libmfxhw32" )
  endif()
else()
  if (NOT MFX_HW_VSI_TARGET)
    include_directories(
      ${MFX_HOME}/_studio/shared/include
    )

    append(" -DMFX_VA_LINUX" CMAKE_CXX_FLAGS)
  endif()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( MFX_LIBNAME mfxhw64 )
  else()
    set( MFX_LIBNAME mfxhw32 )
  endif()
endif()

option(USE_SSH_FOR_BYPASS "Use SSH for bypass repo?" OFF)
option(MFX_VSI_HDDL "Build with HDDL Unite support?" ON)
option(MFX_VSI_MSDK_SHIM "Build with MSDK Shim support?" OFF)

if (MFX_ONEVPL)
  add_definitions (-DMFX_ONEVPL)
endif()

if (MFX_VSI_HDDL)
  include (${BUILDER_ROOT}/profiles/FindHddlUnite.cmake)
  FindHDDLUnite ()
endif()

set ( MFX_ENABLE_MCTF OFF )
set ( MFX_ENABLE_ASC OFF )
set ( MFX_ENABLE_KERNELS OFF )
set ( MFX_ENABLE_SW_FALLBACK OFF )
set ( MFX_ENABLE_H264_VIDEO_FEI_ENCODE OFF )
set ( MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE OFF )

set( MFX_ENABLE_USER_DECODE OFF)
set( MFX_ENABLE_USER_ENCODE OFF)
set( MFX_ENABLE_USER_ENC OFF)
set( MFX_ENABLE_USER_VPP OFF)

set( BUILD_TUTORIALS ON )
set( BUILD_TOOLS OFF )

set ( MFX_LIBVA_NAME "" )

include_directories(
    ${CMAKE_BINARY_DIR}/va_hantro
)

include (${BUILDER_ROOT}/profiles/installer.cmake)
