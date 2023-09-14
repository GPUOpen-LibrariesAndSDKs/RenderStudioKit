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
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set (CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(RenderStudioSchema SHARED ${SOURCES})

# Link libraries

target_link_libraries(RenderStudioSchema PRIVATE
    Boost::boost
    usdGeom
    usd
    RenderStudioLogger
    RenderStudioNetworking
)

if(MAYA_SUPPORT)
    target_link_libraries(RenderStudioSchema PRIVATE
        ${PYTHON_LIBRARIES}
    )
else()
    target_link_libraries(RenderStudioSchema PRIVATE
        Boost::python
    )
endif()

# Set compile options
target_compile_definitions(RenderStudioSchema PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN 
    NOMINMAX
    _WIN32_WINNT=0x0601
    RENDERSTUDIOSCHEMA_EXPORTS
    VS_PLATFORM_TOOLSET=${VS_NUMERIC_PLATFORM_TOOLSET}
)

# Install library
install(TARGETS RenderStudioSchema
    DESTINATION plugin/usd)

# Install plugInfo
if(WIN32)
    set(PLUGIN_LIBRARY_NAME RenderStudioSchema.dll)
elseif(UNIX)
    set(PLUGIN_LIBRARY_NAME libRenderStudioSchema.so)
endif()

configure_file(plugInfo.json plugInfo.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION plugin/usd/RenderStudioSchema/resources)

# Install headers
file(GLOB HEADERS *.h)
install(FILES ${HEADERS} DESTINATION include/RenderStudioResolver)

# Export CMake target
set_target_properties(RenderStudioSchema PROPERTIES
    EXPORT_NAME Schema
)

export(TARGETS RenderStudioSchema
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioSchemaConfig.cmake
)
