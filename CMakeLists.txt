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
set(KIT_TARGETS RenderStudioKit RenderStudioResolver RenderStudioNetworking RenderStudioLogger RenderStudioUtils RenderStudioNotice)
set(KIT_DEPENDENCIES fmt uriparser zip)

if(NOT HOUDINI_SUPPORT)
    list(APPEND KIT_TARGETS RenderStudioSchema)
endif()

install(TARGETS ${KIT_TARGETS} ${KIT_DEPENDENCIES}
    EXPORT RenderStudioTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} Commented out since we have duplication of dll's for now
    # INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} Commented out since each target installs own includes
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

# Build workspace
if (WITH_SHARED_WORKSPACE_SUPPORT)
    # Install build dependencies with pip
    if (WITH_PYTHON_DEPENDENCIES_INSTALL)
        find_program(PIP_EXECUTABLE REQUIRED NAMES pip3 pip)
        set(REQUIREMENTS_FILE ${CMAKE_SOURCE_DIR}/Workspace/requirements.txt)
        set(INSTALLED_REQUIREMENTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/installed_requirements.txt)

        # Custom command to check if requirements have changed
        add_custom_command(
            OUTPUT ${INSTALLED_REQUIREMENTS_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${REQUIREMENTS_FILE} ${INSTALLED_REQUIREMENTS_FILE}
            COMMAND ${PIP_EXECUTABLE} install -r ${REQUIREMENTS_FILE}
            DEPENDS ${REQUIREMENTS_FILE}
            COMMENT "Checking and installing python dependencies if required"
        )

        # Custom target for installing Python dependencies
        add_custom_target(
            InstallPythonDeps ALL
            DEPENDS ${INSTALLED_REQUIREMENTS_FILE}
        )
    else()
        find_program(NUITKA_EXECUTABLE REQUIRED NAMES nuitka.bat)
    endif()

    if (WIN32)
        set(WORKSPACE_EXE_NAME "RenderStudioWorkspace.exe")
    endif()

    if (UNIX)
        set(WORKSPACE_EXE_NAME "RenderStudioWorkspace.bin")
    endif()

    file(GLOB_RECURSE PYTHON_SOURCE_FILES ${CMAKE_SOURCE_DIR}/Workspace/*.py)

    # Define the output of the command, i.e., the path of the executable produced by Nuitka
    set(NUITKA_OUTPUT_EXE ${CMAKE_CURRENT_BINARY_DIR}/nuitka/RenderStudioWorkspace.dist/${WORKSPACE_EXE_NAME})
    if (NOT PYTHON_EXECUTABLE)
        find_program(PYTHON_EXECUTABLE REQUIRED NAMES python3 python)
    endif()

    # Define a custom command that compiles the Python script into an executable
    # This command will only run if the output file (Nuitka executable) is missing or if any of the PYTHON_SOURCE_FILES have changed.
    add_custom_command(
        OUTPUT ${NUITKA_OUTPUT_EXE}
        COMMAND ${PYTHON_EXECUTABLE} -m nuitka
            --output-dir=${CMAKE_CURRENT_BINARY_DIR}/nuitka
            --standalone
            --assume-yes-for-downloads
            --windows-icon-from-ico=${CMAKE_SOURCE_DIR}/Resources/Icon.ico
            --msvc=14.2
            ${CMAKE_SOURCE_DIR}/Workspace/RenderStudioWorkspace.py
        DEPENDS ${PYTHON_SOURCE_FILES}
        COMMENT "Building executable of RenderStudioWorkspace using nuitka"
    )

    # Define a custom target that depends on the Nuitka output executable
    # This target will run every time, but the custom command above will only re-execute if its dependencies are updated.
    add_custom_target(
        ExecuteNuitka ALL
        DEPENDS ${NUITKA_OUTPUT_EXE}
    )

    # Optionally, ensure that Python dependencies are installed before Nuitka is executed.
    if (WITH_PYTHON_DEPENDENCIES_INSTALL)
        add_dependencies(ExecuteNuitka InstallPythonDeps)
    endif()

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nuitka/RenderStudioWorkspace.dist/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/usd/RenderStudioWorkspace
        USE_SOURCE_PERMISSIONS
    )
endif()
