cmake_minimum_required(VERSION 3.12)
project(RenderStudioLogger)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Find packages
find_package(Boost REQUIRED COMPONENTS log)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::log
    RenderStudioExternal
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    Boost::log
)

target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN 
    NOMINMAX
    _WIN32_WINNT=0x0601
)

SetMaxWarningLevel(${PROJECT_NAME})

# Export CMake target
set_target_properties(${PROJECT_NAME} PROPERTIES
    EXPORT_NAME Logger
)

export(TARGETS ${PROJECT_NAME}
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioLoggerConfig.cmake
)
