cmake_minimum_required(VERSION 3.12)
project(RenderStudioExternal)

set(URIPARSER_BUILD_TESTS OFF)
set(URIPARSER_BUILD_TOOLS OFF)
set(URIPARSER_BUILD_DOCS OFF)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(fmt)
add_subdirectory(uriparser)
add_subdirectory(zip)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Empty.cpp "int main() { return 0; }")
add_library(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/Empty.cpp)
target_link_libraries(${PROJECT_NAME} fmt uriparser zip)
set_target_properties(${PROJECT_NAME} fmt uriparser zip uninstall PROPERTIES FOLDER HiddenTargets)

set_target_properties(${PROJECT_NAME} PROPERTIES
    EXPORT_NAME External
)

export(TARGETS ${PROJECT_NAME} fmt uriparser zip
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioExternalConfig.cmake
)
