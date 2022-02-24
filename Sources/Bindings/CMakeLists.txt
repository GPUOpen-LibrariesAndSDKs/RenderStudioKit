cmake_minimum_required(VERSION 3.12)

set(USDPLUGIN_PYTHON_NAME _${USDPLUGIN_NAME})

set_boost_namespace(${USDPLUGIN_PYTHON_NAME})

add_library(${USDPLUGIN_PYTHON_NAME} SHARED
    module.cpp
    moduleDeps.cpp
    wrapWebUsdAssetResolver.cpp
    BoostIncludeWrapper.h
)

set_property(TARGET ${USDPLUGIN_PYTHON_NAME} PROPERTY PREFIX "")
target_link_libraries(${USDPLUGIN_PYTHON_NAME} ${USDPLUGIN_NAME})

target_compile_definitions(${USDPLUGIN_PYTHON_NAME}
PRIVATE
    MFB_PACKAGE_NAME=${USDPLUGIN_NAME}
    MFB_ALT_PACKAGE_NAME=${USDPLUGIN_NAME}
    MFB_PACKAGE_MODULE=webusd.${USDPLUGIN_NAME}
)

# Install python wrapper
install(TARGETS ${USDPLUGIN_PYTHON_NAME}
    DESTINATION lib/python/webusd/${USDPLUGIN_NAME})

install(FILES __init__.py
    DESTINATION lib/python/webusd/${USDPLUGIN_NAME})
