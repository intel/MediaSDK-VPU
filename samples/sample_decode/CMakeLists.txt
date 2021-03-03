include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_common/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../sample_misc/wayland/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

list( APPEND LIBS_VARIANT sample_common )

set(DEPENDENCIES libmfx dl pthread hddl)
make_executable( shortname universal )