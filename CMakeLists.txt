cmake_minimum_required(VERSION 3.12)
project(RenderStudioResolver VERSION 0.2 LANGUAGES CXX)

set(USDPLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

option(PXR_ENABLE_PYTHON_SUPPORT "Build Python wrapper and Python based tests" ON)
option(BUILD_MONOLITHIC "Build against monolithic USD" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/defaults)
include(HandleBoostNamespace)

if(BUILD_MONOLITHIC)
    find_package(USDMonolithic REQUIRED)
else()
    find_package(USD REQUIRED)
endif()

if (UNIX)
add_compile_options(-Wno-deprecated)
endif()

add_definitions(-D_WEBSOCKETPP_CPP11_STL_)

add_subdirectory(Sources)
