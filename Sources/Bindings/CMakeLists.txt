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

set(USDPLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(USDPLUGIN_PYTHON_NAME _${USDPLUGIN_NAME})

add_library(${USDPLUGIN_PYTHON_NAME} SHARED
    module.cpp
    moduleDeps.cpp
    wrapRenderStudioKit.cpp
)

target_link_libraries(${USDPLUGIN_PYTHON_NAME} PRIVATE
    Boost::boost
    Boost::python
    Boost::log
    RenderStudioKit
    RenderStudioResolver
    RenderStudioNetworking
    usd
)

if (TBB_LIBRARY)
    target_link_libraries(${USDPLUGIN_PYTHON_NAME} PRIVATE ${TBB_LIBRARY})
endif()

target_compile_definitions(${USDPLUGIN_PYTHON_NAME}
PRIVATE
    MFB_PACKAGE_NAME=${USDPLUGIN_NAME}
    MFB_ALT_PACKAGE_NAME=${USDPLUGIN_NAME}
    MFB_PACKAGE_MODULE=rs.${USDPLUGIN_NAME}
)

# target_include_directories(${USDPLUGIN_PYTHON_NAME} PRIVATE ${USD_INCLUDE_DIRS})
# target_include_directories(${USDPLUGIN_PYTHON_NAME} PRIVATE ${TBB_INCLUDE_DIRS})
# target_include_directories(${USDPLUGIN_PYTHON_NAME} PRIVATE ${Python3_INCLUDE_DIRS})

set_property(TARGET ${USDPLUGIN_PYTHON_NAME} PROPERTY PREFIX "")
set_property(TARGET ${USDPLUGIN_PYTHON_NAME} PROPERTY SUFFIX ".pyd")

# Install python wrapper
install(TARGETS ${USDPLUGIN_PYTHON_NAME} DESTINATION lib/python/rs/${USDPLUGIN_NAME})
install(FILES __init__.py DESTINATION lib/python/rs/${USDPLUGIN_NAME})
