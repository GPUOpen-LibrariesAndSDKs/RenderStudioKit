cmake_minimum_required(VERSION 3.12)
project(RenderStudioResolver VERSION 0.2 LANGUAGES CXX)

set(USDPLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/defaults)
include(HandleBoostNamespace)

find_package(USDMonolithic REQUIRED)

add_definitions(-D_WEBSOCKETPP_CPP11_STL_)

add_subdirectory(Sources)
