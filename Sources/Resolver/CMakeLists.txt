# Copyright 2023 Advanced Micro Devices, Inc
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.12)
project(RenderStudioResolver)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::python
    ar
    sdf
    RenderStudioNetworking
    RenderStudioLogger
    RenderStudioSerialization
)

target_include_directories(${PROJECT_NAME} PRIVATE ..)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        wsock32
        ws2_32
        bcrypt
    )
endif()

if (WIN32)
    string (REGEX REPLACE "v([0-9]+)" "\\1" VS_NUMERIC_PLATFORM_TOOLSET ${CMAKE_VS_PLATFORM_TOOLSET})
else()
    set(VS_NUMERIC_PLATFORM_TOOLSET 0)
endif()

# Set compile options
SetDefaultCompileDefinitions(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    AR_EXPORTS
    VS_PLATFORM_TOOLSET=${VS_NUMERIC_PLATFORM_TOOLSET}
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_UNIX)
endif()

SetMaxWarningLevel(${PROJECT_NAME})

# Install library
install(TARGETS ${PROJECT_NAME}
    DESTINATION plugin/usd)

# Install plugInfo
if(WIN32)
    set(PLUGIN_LIBRARY_NAME ${PROJECT_NAME}.dll)
elseif(UNIX)
    set(PLUGIN_LIBRARY_NAME lib${PROJECT_NAME}.so)
endif()

configure_file(plugInfo.json plugInfo.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION plugin/usd/${PROJECT_NAME}/resources)

# Install headers
install(FILES Notice.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RenderStudio)

ExportTargetConfig(${PROJECT_NAME} Resolver)
