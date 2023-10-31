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
set(KIT_TARGET_NAME "RenderStudioResolver")

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${KIT_TARGET_NAME} SHARED ${SOURCES})

# Link libraries
target_link_libraries(${KIT_TARGET_NAME} PRIVATE
    Boost::boost
    Boost::python
    ar
    sdf
    RenderStudioNetworking
    RenderStudioLogger
    RenderStudioSerialization
    RenderStudioUtils
    RenderStudioNotice
)

if (TBB_INCLUDE_DIR)
    target_link_libraries(${KIT_TARGET_NAME} PRIVATE ${TBB_LIBRARY})
    target_include_directories(${KIT_TARGET_NAME} PRIVATE ${TBB_INCLUDE_DIR})
endif()

if (MAYA_SUPPORT)
    target_link_libraries(${KIT_TARGET_NAME} PRIVATE
       ${PYTHON_LIBRARIES}
       usd
    )
endif()

target_include_directories(${KIT_TARGET_NAME} PRIVATE ..)

if(WIN32)
    target_link_libraries(${KIT_TARGET_NAME} PRIVATE
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
SetDefaultCompileDefinitions(${KIT_TARGET_NAME})
target_compile_definitions(${KIT_TARGET_NAME} PRIVATE
    AR_EXPORTS
    VS_PLATFORM_TOOLSET=${VS_NUMERIC_PLATFORM_TOOLSET}
)

SetMaxWarningLevel(${KIT_TARGET_NAME})

# Install library
install(TARGETS ${KIT_TARGET_NAME}
    DESTINATION plugin/usd)

# Install plugInfo
if(WIN32)
    set(PLUGIN_LIBRARY_NAME ${KIT_TARGET_NAME}.dll)
elseif(UNIX)
    set(PLUGIN_LIBRARY_NAME lib${KIT_TARGET_NAME}.so)
endif()

configure_file(plugInfo.json plugInfo.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION plugin/usd/${KIT_TARGET_NAME}/resources)
