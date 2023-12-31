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

find_package(OpenSSL REQUIRED)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})


# Find packages
if(MAYA_SUPPORT)
    find_package(MayaUsd REQUIRED)

    # perform find package after we listed all necessary directoris in Find mayausd package
    find_package(Boost REQUIRED COMPONENTS python log)
else()
    find_package(Boost REQUIRED COMPONENTS python log)
    if(HOUDINI_SUPPORT)
        find_package(HoudiniUSD REQUIRED)
    else()
        if (PXR_BUILD_MONOLITHIC)
            find_package(USDMonolithic REQUIRED)
        else()
            find_package(pxr REQUIRED CONFIG)
        endif()
    endif()
endif()

# Add dependencies
add_subdirectory(External)
add_subdirectory(Logger)
add_subdirectory(Networking)
add_subdirectory(Serialization)
add_subdirectory(Server)
add_subdirectory(Resolver)
add_subdirectory(Utils)
add_subdirectory(Notice)

if(NOT HOUDINI_SUPPORT)
    add_subdirectory(Schema)
    target_link_libraries(${PROJECT_NAME} PRIVATE RenderStudioSchema)
endif()

if (PXR_ENABLE_PYTHON_SUPPORT)
    add_subdirectory(Bindings)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:Boost::python>
    $<BUILD_INTERFACE:Boost::boost>
    $<BUILD_INTERFACE:Boost::log>
    $<BUILD_INTERFACE:usd>
    RenderStudioResolver
    RenderStudioNetworking
    RenderStudioLogger
)

if (TBB_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIR})
endif()

# Set compile options
SetDefaultCompileDefinitions(${PROJECT_NAME})
SetMaxWarningLevel(${PROJECT_NAME})
SetVisualStudioDebuggerOptions(${PROJECT_NAME})

install(FILES Kit.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RenderStudio)

# Export target
set_target_properties(${PROJECT_NAME}
    PROPERTIES EXPORT_NAME Kit
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

# Downlaod syncthing
include(FetchContent)

if (WIN32)
    FetchContent_Declare(
        syncthing
        URL https://github.com/syncthing/syncthing/releases/download/v1.25.0/syncthing-windows-amd64-v1.25.0.zip
        URL_HASH SHA256=fbd89a9e2893f778b8379f151b50c922c72ac73e8af03317480f3a2ad9eb7aeb
    )
endif()

if (UNIX)
    FetchContent_Declare(
        syncthing
        URL https://github.com/syncthing/syncthing/releases/download/v1.25.0/syncthing-linux-amd64-v1.25.0.tar.gz
        URL_HASH SHA256=c16d62bc54659c27728b3501786240a06e29dd553204fd01b2968cad0bb03e4e
    )
endif()

FetchContent_GetProperties(syncthing)

if(NOT syncthing_POPULATED)
    FetchContent_Populate(syncthing)
endif()

if (WIN32)
    install(FILES ${syncthing_SOURCE_DIR}/syncthing.exe
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/usd/RenderStudioWorkspace)
endif()

if (UNIX)
    install(FILES ${syncthing_SOURCE_DIR}/syncthing
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/usd/RenderStudioWorkspace
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
endif()
