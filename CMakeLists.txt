cmake_minimum_required(VERSION 3.12)
project(RenderStudioResolver VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/defaults)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/macros)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/)

include(${CMAKE_SOURCE_DIR}/CMake/macros.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/macros/msvc.cmake)

add_subdirectory(Sources)
