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

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set( MFX_LIBNAME "libmfxhw64" )
else()
  set( MFX_LIBNAME "libmfxhw32" )
endif()

set( BUILD_RUNTIME ON )
#set( BUILD_DISPATCHER OFF )

add_definitions( -DMFX_VA )
add_definitions( -DMFX_VA_WIN )
add_definitions( -DMFX_D3D11_ENABLED )
add_definitions( -D_UNICODE )
add_definitions( -DUNICODE )
add_definitions( -DNOMINMAX )

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")

add_compile_options("$<$<CONFIG:Release>:/Qspectre>" "$<$<CONFIG:Release>:/guard:cf>")
add_link_options("$<$<CONFIG:Release>:/guard:cf>")
add_link_options("/DYNAMICBASE")

set( MFX_BUNDLED_IPP OFF )

set( ENABLE_OPENCL OFF )

set( MFX_ENABLE_KERNELS ON )
set( MFX_ENABLE_MCTF ON )
set( MFX_ENABLE_ASC ON )
set( MFX_ENABLE_MFE OFF )
set( MFX_ENABLE_VPP ON )
set( MFX_ENABLE_SW_FALLBACK ON )

set( MFX_ENABLE_USER_DECODE OFF)
set( MFX_ENABLE_USER_ENCODE OFF)
set( MFX_ENABLE_USER_ENC OFF)
set( MFX_ENABLE_USER_VPP OFF)

set( MFX_ENABLE_H264_VIDEO_ENCODE ON )
set( MFX_ENABLE_H264_VIDEO_FEI_ENCODE OFF )
set( MFX_ENABLE_H265_VIDEO_ENCODE ON )
set( MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE OFF )
set( MFX_ENABLE_VP9_VIDEO_ENCODE ON )
set( MFX_ENABLE_VP8_VIDEO_DECODE ON )
set( MFX_ENABLE_VP9_VIDEO_DECODE ON )
set( MFX_ENABLE_H264_VIDEO_DECODE ON )
set( MFX_ENABLE_H265_VIDEO_DECODE ON )
set( MFX_ENABLE_MPEG2_VIDEO_DECODE ON )
set( MFX_ENABLE_MPEG2_VIDEO_ENCODE ON )
set( MFX_ENABLE_MJPEG_VIDEO_DECODE ON )
set( MFX_ENABLE_MJPEG_VIDEO_ENCODE ON )
set( MFX_ENABLE_VC1_VIDEO_DECODE ON )
