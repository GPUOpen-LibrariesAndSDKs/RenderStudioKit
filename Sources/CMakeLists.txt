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
add_library(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::python
    Boost::boost
    RenderStudioResolver
    RenderStudioNetworking
    RenderStudioLogger
    RenderStudioExternal
    RenderStudioSchema
    usd
)

# Set compile options
SetDefaultCompileDefinitions(${PROJECT_NAME})
SetMaxWarningLevel(${PROJECT_NAME})
SetVisualStudioDebuggerOptions(${PROJECT_NAME})

# Export target
include(CMakePackageConfigHelpers)

set_target_properties(${PROJECT_NAME}
    PROPERTIES EXPORT_NAME Kit
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
)

install(TARGETS ${PROJECT_NAME} RenderStudioResolver RenderStudioNetworking RenderStudioLogger RenderStudioExternal RenderStudioSchema usd fmt uriparser zip
    EXPORT RenderStudioTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES Kit.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RenderStudio)

install(EXPORT RenderStudioTargets
    FILE RenderStudioTargets.cmake
    NAMESPACE RenderStudio::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)

# Deal with version
file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../VERSION RENDER_STUDIO_KIT_VERSION)

set_property(TARGET ${PROJECT_NAME} 
    PROPERTY VERSION ${RENDER_STUDIO_KIT_VERSION}
)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY SOVERSION ${RENDER_STUDIO_KIT_VERSION}
)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${RENDER_STUDIO_KIT_VERSION}
)

set_property(TARGET ${PROJECT_NAME} 
    APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/RenderStudioConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/RenderStudioConfigVersion.cmake"
  VERSION ${RENDER_STUDIO_KIT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RenderStudioConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RenderStudioConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)
