cmake_minimum_required(VERSION 3.12)

add_subdirectory(External)

add_library(${USDPLUGIN_NAME} SHARED
    Asset.cpp
    Asset.h
    Resolver.cpp
    Resolver.h)

set_boost_namespace(${USDPLUGIN_NAME})

target_link_libraries(${USDPLUGIN_NAME}
    ${Boost_PYTHON_LIBRARY}
    ar 
    sdf
)

if(WIN32)
  target_link_libraries(${USDPLUGIN_NAME} wsock32 ws2_32)
  target_compile_definitions(${USDPLUGIN_NAME} PUBLIC HAVE_SNPRINTF WIN32_LEAN_AND_MEAN BOOST_ALL_DYN_LINK)
endif()

# Install library
install(TARGETS ${USDPLUGIN_NAME}
    DESTINATION plugin/usd)

# Install plugInfo
if(WIN32)
    set(PLUGIN_LIBRARY_NAME ${USDPLUGIN_NAME}.dll)
elseif(UNIX)
    set(PLUGIN_LIBRARY_NAME lib${USDPLUGIN_NAME}.so)
endif()

configure_file(Plugin/plugInfo.json Plugin/plugInfo.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Plugin/plugInfo.json
    DESTINATION plugin/usd/${USDPLUGIN_NAME}/resources)

# Install python bindings
if (PXR_ENABLE_PYTHON_SUPPORT)
    add_subdirectory(Bindings)
endif()
