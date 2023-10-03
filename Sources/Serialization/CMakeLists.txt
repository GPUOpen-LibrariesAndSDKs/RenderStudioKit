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
project(RenderStudioSerialization)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::python
    sdf
)

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE
    HAVE_SNPRINTF
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0601
)

SetMaxWarningLevel(${PROJECT_NAME})

# Install headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Serialization.h ${CMAKE_CURRENT_SOURCE_DIR}/Api.h
    DESTINATION include/RenderStudioResolver/Serialization)
