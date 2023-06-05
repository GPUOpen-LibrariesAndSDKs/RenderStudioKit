cmake_minimum_required(VERSION 3.12)
project(RenderStudioServer)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

# Find packages
find_package(Boost REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::python
    RenderStudioNetworking
    RenderStudioLogger
    RenderStudioSerialization
    tf
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN 
    NOMINMAX
    _WIN32_WINNT=0x0601
)

SetMaxWarningLevel(${PROJECT_NAME})

# Install headers
install(TARGETS ${PROJECT_NAME} DESTINATION RenderStudioServer)
install(FILES Run.bat DESTINATION RenderStudioServer)
