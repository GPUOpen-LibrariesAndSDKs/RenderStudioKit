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
project(RenderStudioSchema)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(RenderStudioSchema SHARED ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    usdGeom
    usd
    RenderStudioLogger
    RenderStudioNetworking
)

if (TBB_INCLUDE_DIR)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${TBB_LIBRARY})
    target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIR})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ..)

if(MAYA_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${PYTHON_LIBRARIES}
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Boost::python
    )
endif()

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    RENDERSTUDIOSCHEMA_EXPORTS
)

# Install library
install(TARGETS ${PROJECT_NAME}
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

SetDefaultCompileDefinitions(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME})
