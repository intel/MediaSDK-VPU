# Copyright (c) 2017 Intel Corporation
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

if(NOT DEFINED BUILDER_ROOT)
  error("Please set BUILDER_ROOT variable.")
endif()

get_filename_component( MSDK_BUILD_ROOT_MINUS_ONE "${BUILDER_ROOT}/.." ABSOLUTE )

set( MSDK_STUDIO_ROOT  ${MSDK_BUILD_ROOT_MINUS_ONE}/_studio )
set( MSDK_TSUITE_ROOT  ${MSDK_BUILD_ROOT_MINUS_ONE}/_testsuite )
set( MSDK_LIB_ROOT     ${MSDK_STUDIO_ROOT}/mfx_lib )
set( MSDK_UMC_ROOT     ${MSDK_STUDIO_ROOT}/shared/umc )
set( MSDK_SAMPLES_ROOT ${MSDK_BUILD_ROOT_MINUS_ONE}/samples )
set( MSDK_TOOLS_ROOT   ${MSDK_BUILD_ROOT_MINUS_ONE}/tools )
set( MSDK_BUILDER_ROOT ${BUILDER_ROOT} )

function( mfx_include_dirs )
  include_directories (
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/include
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/umc/core/vm/include
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/umc/core/vm_plus/include
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/umc/core/umc/include
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/umc/io/umc_io/include
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/umc/io/umc_va/include
    ${CMAKE_HOME_DIRECTORY}/_studio/shared/umc/io/media_buffers/include
    ${CMAKE_HOME_DIRECTORY}/_studio/mfx_lib/shared/include
    ${CMAKE_HOME_DIRECTORY}/_studio/mfx_lib/optimization/h265/include
    ${CMAKE_HOME_DIRECTORY}/_studio/mfx_lib/optimization/h264/include
    ${CMAKE_HOME_DIRECTORY}/_studio/mfx_lib/shared/include
    ${CMAKE_HOME_DIRECTORY}/_studio/mfx_lib/fei/include
    ${CMAKE_HOME_DIRECTORY}/_studio/mfx_lib/fei/h264_la
    ${CMAKE_HOME_DIRECTORY}/contrib/ipp/include
    ${CMAKE_HOME_DIRECTORY}/contrib/cm/include
  )

endfunction()
