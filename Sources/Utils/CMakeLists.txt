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
set(KIT_TARGET_NAME "RenderStudioUtils")

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${KIT_TARGET_NAME} STATIC ${SOURCES})

# Link libraries
target_link_libraries(${KIT_TARGET_NAME} PRIVATE
    RenderStudioLogger
    zip::zip
)

target_include_directories(${KIT_TARGET_NAME} PRIVATE ..)

SetDefaultCompileDefinitions(${KIT_TARGET_NAME})
SetMaxWarningLevel(${KIT_TARGET_NAME})
