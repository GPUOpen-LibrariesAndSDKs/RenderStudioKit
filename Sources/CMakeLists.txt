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

# Find packages
find_package(Boost REQUIRED COMPONENTS python log)
find_package(OpenSSL REQUIRED)

if(MAYA_SUPPORT)
    find_package(MayaUsd REQUIRED)
else() 
    find_package(USDMonolithic REQUIRED)
endif()

# Add dependencies
add_subdirectory(External)
add_subdirectory(Logger)
add_subdirectory(Networking)
add_subdirectory(Serialization)
add_subdirectory(Server)
add_subdirectory(Schema)
add_subdirectory(Resolver)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${CMAKE_PROJECT_NAME} ${SOURCES})

# Link libraries
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE .)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    usd
    Boost::python
    Boost::boost
    RenderStudioResolver
)

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    WIN32_LEAN_AND_MEAN 
    NOMINMAX
    _WIN32_WINNT=0x0601
    VS_PLATFORM_TOOLSET=${VS_NUMERIC_PLATFORM_TOOLSET}
)

SetMaxWarningLevel(${PROJECT_NAME})
SetVisualStudioDebuggerOptions(${PROJECT_NAME})