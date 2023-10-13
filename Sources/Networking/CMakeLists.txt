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
project(RenderStudioNetworking)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)

if (NOT WIN32)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Certificates.h)
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Certificates.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if (WIN32)
    find_package(BinaryOpenSSL REQUIRED)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    OpenSSL::SSL
    uriparser
    zip::zip
    fmt::fmt
    RenderStudioLogger
)

target_include_directories(${PROJECT_NAME} PRIVATE ..)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Crypt32)
endif()

if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

# Install headers
install(FILES WebsocketClient.h RestClient.h MaterialLibraryApi.h Url.h Certificates.h
    DESTINATION include/RenderStudio/)

install(FILES ${OPENSSL_SSL_BINARY} ${OPENSSL_CRYPTO_BINARY}
    DESTINATION lib)

SetDefaultCompileDefinitions(${PROJECT_NAME})
SetMaxWarningLevel(${PROJECT_NAME})
