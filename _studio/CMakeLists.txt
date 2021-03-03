# Copyright (c) 2018 Intel Corporation
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

if (MFX_VSI_MSDK_SHIM AND (NOT MFX_SHIM_WRAPPER) AND (NOT MFX_HW_VSI_TARGET))
  return()
endif()

add_subdirectory(shared/mfx_trace)
add_subdirectory(shared/umc)

if (MFX_ENABLE_ASC)
  add_subdirectory(shared/asc)
endif()

add_subdirectory(mfx_lib/decode)
add_subdirectory(mfx_lib/encode)
add_subdirectory(mfx_lib/encode_hw)

if (NOT MFX_HW_VSI)
  add_subdirectory(mfx_lib/fei)
endif()

if( MFX_ENABLE_KERNELS )
  add_subdirectory(mfx_lib/genx)
endif()

add_subdirectory(mfx_lib/vpp)

if (MFX_ENABLE_MCTF)
  add_subdirectory(mfx_lib/mctf_package)
endif()

add_subdirectory(mfx_lib)

if(MFX_ENABLE_USER_ENCODE)
  add_subdirectory(hevce_hw)
  if (NOT MFX_HW_VSI)
    add_subdirectory(hevc_fei)
    add_subdirectory(vp9e_hw)
  endif()
endif()

if (NOT MFX_HW_VSI)
  add_subdirectory(mfx_lib/plugin)
endif()

