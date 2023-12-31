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
project(RenderStudioServer)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::python
    RenderStudioNetworking
    RenderStudioLogger
    RenderStudioSerialization
    RenderStudioKit
    tf
)

if (TBB_INCLUDE_DIR)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${TBB_LIBRARY})
    target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIR})
endif()

if (MAYA_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${PYTHON_LIBRARIES}
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

SetMaxWarningLevel(${PROJECT_NAME})
SetDefaultCompileDefinitions(${PROJECT_NAME})

# Install headers
install(TARGETS ${PROJECT_NAME} DESTINATION RenderStudioServer)

if (WIN32)
    install(PROGRAMS Run.bat DESTINATION RenderStudioServer)
elseif(UNIX)
    install(PROGRAMS Run.sh DESTINATION RenderStudioServer)
endif()
