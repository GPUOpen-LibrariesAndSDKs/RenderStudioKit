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
project(RenderStudioKit VERSION 0.3 LANGUAGES CXX)

# Options
option(WITH_SHARED_WORKSPACE_SUPPORT "Enables shared workspace feature. Requires pyinstaller to be installed" ON)
option(WITH_PYTHON_DEPENDENCIES_INSTALL "Enables automatic install for all build requirements for Python" ON)

# Config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/defaults)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/macros)

include(${CMAKE_SOURCE_DIR}/CMake/macros/macros.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/macros/msvc.cmake)

add_subdirectory(Sources)

# Install license
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_SOURCE_DIR}/NOTICE DESTINATION share/doc/${CMAKE_PROJECT_NAME})

# Install RenderStudioTargets.cmake
include(CMakePackageConfigHelpers)
set(KIT_TARGETS RenderStudioKit RenderStudioResolver RenderStudioNetworking RenderStudioLogger RenderStudioUtils)
set(KIT_DEPENDENCIES fmt uriparser zip)

if(NOT HOUDINI_SUPPORT)
    list(APPEND KIT_TARGETS RenderStudioSchema)
endif()

install(TARGETS ${KIT_TARGETS} ${KIT_DEPENDENCIES}
    EXPORT RenderStudioTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} Commented out since we have duplication of dll's for now
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT RenderStudioTargets
    FILE RenderStudioTargets.cmake
    NAMESPACE RenderStudio::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake
)

# Install RenderStudioConfig.cmake
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/CMake/templates/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/RenderStudioConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake/
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RenderStudioConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake/
)

# Build watchdog
if (WITH_SHARED_WORKSPACE_SUPPORT)
    # Install build dependencies with pip
    if (WITH_PYTHON_DEPENDENCIES_INSTALL)
        find_program(PIP_EXECUTABLE REQUIRED NAMES pip3 pip)
        add_custom_target(
            InstallPythonDeps ALL
            COMMAND ${PIP_EXECUTABLE} install -r ${CMAKE_SOURCE_DIR}/Watchdog/requirements.txt
            COMMENT "Installing python dependencies with pip"
        )
    else()
        find_program(PYINSTALLER_EXECUTABLE REQUIRED NAMES pyinstaller)
    endif()

    # Build executable with pyinstaller
    add_custom_target(
        ExecutePyinstaller ALL
        COMMAND
            pyinstaller
            --distpath ${CMAKE_CURRENT_BINARY_DIR}/pyinstaller/dist
            --workpath ${CMAKE_CURRENT_BINARY_DIR}/pyinstaller/build
            ${CMAKE_SOURCE_DIR}/Watchdog/watchdog.spec
        COMMENT "Building executable of RenderStudioWatchdog using pyinstaller"
        DEPENDS InstallPythonDeps
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyinstaller/dist/RenderStudioWatchdog.exe
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/usd/
    )
endif()
