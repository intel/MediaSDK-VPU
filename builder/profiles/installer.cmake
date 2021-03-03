
#documentation
install(
    DIRECTORY ${CMAKE_HOME_DIRECTORY}/doc/pdf/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/doc/
    FILES_MATCHING 
    PATTERN "*.pdf"
)

install(FILES
        ${CMAKE_HOME_DIRECTORY}/LICENSE
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/
)

install(FILES
        ${CMAKE_HOME_DIRECTORY}/doc/LICENSE.rtf
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/
)

#sources:tutorials
foreach(dir common simple_2_decode_hddl simple_5_transcode_hddl)
install(
    DIRECTORY 
        ${CMAKE_HOME_DIRECTORY}/tutorials/${dir}
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/sources/tutorials/
)
endforeach()

install(FILES
        ${CMAKE_HOME_DIRECTORY}/tutorials/CMakeLists.txt
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/sources/tutorials/
)

#sources:samples
foreach(dir sample_common sample_decode sample_encode sample_multi_transcode)
install(
    DIRECTORY 
        ${CMAKE_HOME_DIRECTORY}/samples/${dir}
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/sources/samples/
)
endforeach()

install(FILES
        ${CMAKE_HOME_DIRECTORY}/samples/CMakeLists.txt
        ${CMAKE_HOME_DIRECTORY}/samples/AllSamples.sln
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/sources/samples/
)

if (Windows)
#sources: api, libva, builder, doc, cmake, sln
install(
    DIRECTORY 
        ${CMAKE_MFX_HOME}
        ${CMAKE_HOME_DIRECTORY}/builder
        ${CMAKE_HOME_DIRECTORY}/libva
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/sources/
)    

install(FILES
        ${CMAKE_HOME_DIRECTORY}/AllBuild.sln
        ${CMAKE_HOME_DIRECTORY}/CMakeLists.txt
        ${CMAKE_HOME_DIRECTORY}/LICENSE
        ${CMAKE_HOME_DIRECTORY}/mfxconfig.h.in
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/sources/
)
endif()

#content
install(
    DIRECTORY ${CMAKE_HOME_DIRECTORY}/tests/content
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(FILES
        $<$<PLATFORM_ID:Windows>:${CMAKE_HOME_DIRECTORY}/tests/run_examples.bat>
        $<$<PLATFORM_ID:Linux>:${CMAKE_HOME_DIRECTORY}/tests/run_examples.sh>
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/bin/
)
