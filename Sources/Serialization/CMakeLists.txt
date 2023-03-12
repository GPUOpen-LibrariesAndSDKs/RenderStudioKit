cmake_minimum_required(VERSION 3.12)
project(RenderStudioSerialization)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Find packages
find_package(Boost REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    sdf
)

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN 
    NOMINMAX
    _WIN32_WINNT=0x0601
)

SetMaxWarningLevel(${PROJECT_NAME})

# Install headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Serialization.h ${CMAKE_CURRENT_SOURCE_DIR}/Api.h
    DESTINATION include/RenderStudioResolver/Serialization)
