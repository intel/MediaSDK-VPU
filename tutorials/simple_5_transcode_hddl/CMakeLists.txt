include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../samples/sample_common/include)
list( APPEND LIBS_VARIANT tutorials_common sample_common)

set(DEPENDENCIES libva libmfx dl pthread hddl)

if ( Linux )
  set(DEPENDENCIES ${DEPENDENCIES} libva-drm)
endif()

make_executable( simple_transcode_hddl universal )
