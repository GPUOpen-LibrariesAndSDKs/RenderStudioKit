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
project(RenderStudioExternal)

# Set policies
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Add targets
set(URIPARSER_BUILD_TESTS OFF)
set(URIPARSER_BUILD_TOOLS OFF)
set(URIPARSER_BUILD_DOCS OFF)
set(BUILD_SHARED_LIBS OFF)

set(_saved_CMAKE_MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)
add_subdirectory(fmt)
add_subdirectory(uriparser)
add_subdirectory(zip)
set(CMAKE_MESSAGE_LOG_LEVEL ${_saved_CMAKE_MESSAGE_LOG_LEVEL})

# Create static library
add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE fmt uriparser zip)

set_target_properties(${PROJECT_NAME} fmt uriparser zip uninstall PROPERTIES FOLDER HiddenTargets)

if (NOT MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET uriparser PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET zip PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

# Export targets
set_target_properties(${PROJECT_NAME} PROPERTIES
    EXPORT_NAME External
)

export(TARGETS ${PROJECT_NAME} fmt uriparser zip
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioExternalConfig.cmake
)
