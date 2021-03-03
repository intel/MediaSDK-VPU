include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../samples/sample_common/include)

set( sources
    ${CMAKE_CURRENT_SOURCE_DIR}/cmd_options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common_utils.cpp
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/common_utils_windows.cpp>
    ${CMAKE_CURRENT_SOURCE_DIR}/common_hddl.cpp
    $<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/common_utils_linux.cpp>
    ${CMAKE_CURRENT_SOURCE_DIR}/common_vaapi.cpp
)

make_library( tutorials_common universal static )
