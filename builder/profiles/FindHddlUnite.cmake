function (FindHDDLUnite)
  add_definitions (-DHDDL_UNITE)

  set (HDDL_ROOT $ENV{HDDL_ROOT})

  if ( NOT MFX_HW_VSI_TARGET )
    find_library (HDDLUNITE_LIB 
    NAMES HddlUnite
    HINTS "${HDDL_ROOT}/lib" )
  else()
    find_library (HDDLUNITE_LIB 
    NAMES HddlUnite
    HINTS "/opt/intel/hddlunite/lib")
  endif()

  if (HDDLUNITE_LIB)
    message ("HDDLUnite Library located at ${HDDLUNITE_LIB}")
  else ()
    message ("Cannot locate HDDLUnite Library")
  endif ()

  if (MFX_HW_VSI_TARGET)
    find_library (HDDLUNITE_LIB
      NAMES DeviceClient
      HINTS "/opt/intel/hddlunite/lib")

    if (HDDLUNITE_LIB)
        message ("HDDL DeviceClient Library located at ${DEVICE_CLIENT_LIB}")
    else ()
        message ("Cannot locate HDDL DeviceClient Library")
    endif ()
  endif()

  set (BYPASS_INCLUDE_DIR $ENV{BYPASS_INCLUDE_DIR})

  if (NOT MFX_HW_VSI_TARGET)
    set (HDDLUNITE_INCLUDE_DIR "${HDDL_ROOT}/include")
    include_directories (${HDDLUNITE_INCLUDE_DIR})
  else()
    set (HDDLUNITE_INCLUDE_DIR "${BYPASS_INCLUDE_DIR}/opt/intel/hddlunite/include")
    include_directories (${HDDLUNITE_INCLUDE_DIR})
  endif()

  message ("HddlUnite header directory: ${HDDLUNITE_INCLUDE_DIR}")
endfunction (FindHDDLUnite)
