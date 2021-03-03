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

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

mfx_include_dirs()

set(MFX_ORIG_LDFLAGS "${MFX_LDFLAGS}" )

### CM
include_directories( ${MSDK_LIB_ROOT}/cmrt_cross_platform/include )
set( SRC_DIR "${MSDK_LIB_ROOT}/cmrt_cross_platform/src" )
set( defs "" )
set( sources "" )
set( sources.plus "" )
file( GLOB_RECURSE srcs "${SRC_DIR}/*.c" "${SRC_DIR}/*.cpp" )
list( APPEND sources ${srcs})
make_library( cmrt_cross_platform hw static )
set( defs "" )

### MFX common
set( sources "" )
set( sources.plus "" )
foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_brc_common.cpp
    ${prefix}/mfx_common_decode_int.cpp
    ${prefix}/mfx_common_int.cpp
    ${prefix}/mfx_critical_error_handler.cpp
    ${prefix}/mfx_vpx_dec_common.cpp
  )
endforeach()

set( defs "" )
make_library( mfx_common none static )
append_property( mfx_common COMPILE_FLAGS " -DMFX_RT" )

foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
  list( APPEND sources
    ${prefix}/mfx_enc_common.cpp
    ${prefix}/mfx_ddi_enc_dump.cpp
    ${prefix}/mfx_h264_enc_common_hw.cpp
    ${prefix}/mfx_h264_encode_vaapi.cpp
    ${prefix}/mfx_h264_encode_factory.cpp
    ${prefix}/mfx_mpeg2_enc_common_hw.cpp
    ${prefix}/mfx_mpeg2_encode_vaapi.cpp
    ${prefix}/mfx_mpeg2_encode_factory.cpp
  )
endforeach()

set( defs "" )
make_library( mfx_common hw static )
append_property( mfx_common_hw COMPILE_FLAGS " -DMFX_RT" )

### libmfxhw
list( APPEND mdirs
  scheduler vpp decode/vc1
  decode/mjpeg decode/h264 decode/mpeg2 cmrt_cross_platform
  encode_hw/vp9 encode_hw/h265 encode_hw/h264 encode_hw/mpeg2 encode_hw/mjpeg
  encode/mjpeg
  decode/h265 decode/vp8 decode/vp9 mctf_package/mctf
  )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_common )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_preenc )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_pak )
include_directories( ${MSDK_LIB_ROOT}/fei/h264_enc )
include_directories( ${MSDK_LIB_ROOT}/encode_hw/hevc )
foreach( dir ${mdirs} )
  include_directories( ${MSDK_LIB_ROOT}/${dir}/include )
endforeach()

include_directories( ${MSDK_LIB_ROOT}/genx/field_copy/isa )
include_directories( ${MSDK_LIB_ROOT}/genx/copy_kernels/isa )
include_directories( ${MSDK_STUDIO_ROOT}/shared/asc/include )

list( APPEND cdirs
  brc h264_enc mpeg2_dec vc1_dec vp9_dec vc1_common jpeg_enc
  )
foreach( dir ${cdirs} )
  include_directories( ${MSDK_UMC_ROOT}/codec/${dir}/include )
endforeach()

# =============================================================================

set( sources "" )
set( sources.plus "" )
file( GLOB_RECURSE srcs "${MSDK_LIB_ROOT}/scheduler/src/*.cpp" )
list( APPEND sources ${srcs})
# =============================================================================

foreach( prefix ${MSDK_LIB_ROOT}/shared/src )
if (NOT MFX_ONEVPL)
    set (API_1_X_CORE_FILES 
        ${prefix}/libmfxsw_enc.cpp
        ${prefix}/libmfxsw_pak.cpp
        ${prefix}/libmfxsw_plugin.cpp
        ${prefix}/mfx_user_plugin.cpp
        $<$<PLATFORM_ID:Windows>:libmfx.def>
    )
else()
    set (API_1_X_CORE_FILES 
        $<$<PLATFORM_ID:Windows>:libmfx-onevpl.def>
    )
endif()
  list( APPEND sources
    ${prefix}/libmfxsw.cpp
    ${prefix}/libmfxsw_async.cpp
    ${prefix}/libmfxsw_decode.cpp
    ${prefix}/libmfxsw_encode.cpp
    ${prefix}/libmfxsw_query.cpp
    ${prefix}/libmfxsw_session.cpp
    ${prefix}/libmfxsw_vpp.cpp
    ${prefix}/libmfxsw_vpl.cpp
    ${prefix}/mfx_session.cpp
    ${API_1_X_CORE_FILES}
    #=================
    ${prefix}/mfx_brc_common.cpp
    ${prefix}/mfx_vc1_dec_common.cpp
    ${prefix}/mfx_vpx_dec_common.cpp
    #=================
  )
endforeach()
foreach( prefix ${MSDK_STUDIO_ROOT}/shared/src )

  if( NOT MFX_HW_VSI_TARGET )
    add_library(fast_copy_sse4 OBJECT ${prefix}/fast_copy_sse4_impl.cpp)
    target_compile_options(fast_copy_sse4 PRIVATE -msse4.1)
    configure_build_variant(fast_copy_sse4 none)
  endif()

  list( APPEND sources
    ${prefix}/cm_mem_copy.cpp
    ${prefix}/fast_copy_c_impl.cpp
    ${prefix}/fast_copy.cpp
    ${prefix}/mfx_vpp_vaapi.cpp
    ${prefix}/libmfx_allocator.cpp
    ${prefix}/libmfx_allocator_vaapi.cpp
    ${prefix}/libmfx_allocator_hddl.cpp
    ${prefix}/libmfx_core.cpp
    ${prefix}/libmfx_core_hw.cpp
    ${prefix}/libmfx_core_factory.cpp
    ${prefix}/libmfx_core_vaapi.cpp
    ${prefix}/libmfx_core_hddl.cpp
    ${prefix}/mfx_umc_alloc_wrapper.cpp
    ${prefix}/mfx_umc_mjpeg_vpp.cpp
    ${prefix}/mfx_umc_mjpeg_vpp_sw.cpp
    ${prefix}/mfx_static_assert_structs.cpp
    ${prefix}/mfx_mfe_adapter.cpp
  )

  if( NOT MFX_HW_VSI_TARGET )
    list( APPEND sources
      $<TARGET_OBJECTS:fast_copy_sse4>
    )
  endif()

endforeach()

set( USE_STRICT_NAME TRUE )

if (NOT MFX_ONEVPL)
  set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_LIB_ROOT}/libmfxhw.map" )
else()
  set( MFX_LDFLAGS "${MFX_ORIG_LDFLAGS} -Wl,--version-script=${MSDK_LIB_ROOT}/libmfxhw-onevpl.map" )
endif()

if( DEFINED MFX_LIBNAME )
  set( mfxlibname "${MFX_LIBNAME}")
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( mfxlibname mfxhw64 )
  else()
    set( mfxlibname mfxhw32 )
  endif()
endif()

if (MFX_SHIM_WRAPPER)
  set( mfxlibname "${mfxlibname}_wrp" )
endif()

if( MFX_ENABLE_MCTF )
  set(MCTF_LIB "mctf_hw")
else()
  set(MCTF_LIB "")
endif()

if( MFX_ENABLE_ASC )
  set(ASC_LIB "asc")
else()
  set(ASC_LIB "")
endif()

if( NOT MFX_HW_VSI )
  set(H264FEI_LIB "h264_la h264_common h264_preenc h264_enc h264_pak")
else()
  set(H264FEI_LIB "")
endif()

if( MFX_ENABLE_KERNELS )
  set(GENX_LIB "genx")
else()
  set(GENX_LIB "")
endif()

if( MFX_ENABLE_SW_FALLBACK )
  set(SW_FALLBACK_LIBS "encode_sw;ipp;encode")
else()
  set(SW_FALLBACK_LIBS "")
endif()

set( LIBS "" )
list( APPEND LIBS
  bitrate_control
  ${SW_FALLBACK_LIBS}
  umc
  vm
  vm_plus
  umc_va_hw
  cmrt_cross_platform_hw
  decode_hw
  encode_hw
  ${H264FEI_LIB}
  ${GENX_LIB}
  mfx_common
  mfx_common_hw
  ${ASC_LIB}
  ${MCTF_LIB}
  vpp_hw
  mfx_trace
  ${ITT_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
  ${MFX_LIBVA_NAME}
)

if (MFX_VSI_HDDL)
  list( APPEND LIBS ${HDDLUNITE_LIB} )

if ( Linux )
  set(DEPENDENCIES ${DEPENDENCIES} libva-drm)
endif()

endif()

if ( Linux )
  set(DEPENDENCIES ${DEPENDENCIES} libva-drm)
endif()

make_library( ${mfxlibname} hw shared )

if (MFX_SHIM_WRAPPER)
  set_target_properties(${mfxlibname} PROPERTIES SUFFIX ".dla")
endif()

get_mfx_version(mfx_version_major mfx_version_minor)

set_target_properties(${mfxlibname} PROPERTIES   VERSION ${mfx_version_major}.${mfx_version_minor})
set_target_properties(${mfxlibname} PROPERTIES SOVERSION ${mfx_version_major})

install(TARGETS ${mfxlibname} LIBRARY DESTINATION ${MFX_MODULES_DIR})

set( PKG_CONFIG_FNAME "${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE}/lib${mfxlibname}.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake" ${PKG_CONFIG_FNAME} @ONLY)
install( FILES ${PKG_CONFIG_FNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )
