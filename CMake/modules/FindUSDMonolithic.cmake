# USD 20.05 does not generate cmake config for the monolithic library as it does for the default mode
# So we have to handle monolithic USD in a special way

if (UNIX)
    set (PXR_LIB_PREFIX "lib")
endif ()

find_path(USD_INCLUDE_DIR pxr/pxr.h
    PATHS ${USD_LOCATION}/include
          $ENV{USD_LOCATION}/include
    DOC "USD Include directory"
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH)

find_path(USD_LIBRARY_DIR
    NAMES "${PXR_LIB_PREFIX}usd_usd_ms${CMAKE_SHARED_LIBRARY_SUFFIX}"
    PATHS ${USD_LOCATION}/lib
          $ENV{USD_LOCATION}/lib
    DOC "USD Libraries directory"
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH)

find_library(USD_MONOLITHIC_LIBRARY
    NAMES
        usd_usd_ms # Windows requires raw library name to find
        ${PXR_LIB_PREFIX}usd_usd_ms${CMAKE_SHARED_LIBRARY_SUFFIX} # Linux requires prefix and suffix
    PATHS ${USD_LIBRARY_DIR}
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(USDMonolithic
    USD_INCLUDE_DIR
    USD_LIBRARY_DIR
    USD_MONOLITHIC_LIBRARY)

if(USDMonolithic_FOUND)
    list(APPEND CMAKE_PREFIX_PATH ${USD_LOCATION})

    # --Python.
    find_package(PythonInterp 3 REQUIRED)
    find_package(PythonLibs 3 REQUIRED)
    find_package(Boost REQUIRED)

    add_library(usd_usd_ms SHARED IMPORTED)
    set_property(TARGET usd_usd_ms APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

    target_compile_definitions(usd_usd_ms INTERFACE
        -DPXR_PYTHON_ENABLED=1)
    target_link_libraries(usd_usd_ms INTERFACE
        ${USD_MONOLITHIC_LIBRARY}
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES})

    target_link_directories(usd_usd_ms INTERFACE
        ${USD_LIBRARY_DIR})
    target_include_directories(usd_usd_ms INTERFACE
        ${USD_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS})
    set_target_properties(usd_usd_ms PROPERTIES
      IMPORTED_IMPLIB_RELEASE "${USD_MONOLITHIC_LIBRARY}"
      IMPORTED_LOCATION_RELEASE "${USD_LIBRARY_DIR}/${PXR_LIB_PREFIX}usd_usd_ms${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )

    foreach(targetName
        arch tf gf js trace work plug vt ar kind sdf ndr sdr pcp usd usdGeom
        usdVol usdLux usdMedia usdShade usdRender usdHydra usdRi usdSkel usdUI
        usdUtils garch hf hio cameraUtil pxOsd glf hgi hgiGL hd hdSt hdx
        usdImaging usdImagingGL usdRiImaging usdSkelImaging usdVolImaging
        usdAppUtils usdviewq)
        add_library(${targetName} INTERFACE)
        target_link_libraries(${targetName} INTERFACE usd_usd_ms)
    endforeach()
endif()
