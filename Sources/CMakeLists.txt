cmake_minimum_required(VERSION 3.12)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

# Find packages
find_package(USDMonolithic REQUIRED)
find_package(Boost REQUIRED COMPONENTS python random)

add_subdirectory(Logger)
add_subdirectory(Networking)
add_subdirectory(Serialization)

set(URIPARSER_BUILD_TESTS OFF)
set(URIPARSER_BUILD_TOOLS OFF)
set(URIPARSER_BUILD_DOCS OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(External/uriparser)

# Link libraries
add_library(External INTERFACE)
target_include_directories(External SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/External
    ${CMAKE_CURRENT_SOURCE_DIR}/External/websocketpp
    ${CMAKE_CURRENT_SOURCE_DIR}/External/fmt/include
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::python
    Boost::random
    ar
    sdf
    External
    RenderStudioNetworking
    RenderStudioLogger
    RenderStudioSerialization
)

# Temporary hack to make fmt working in other projects
target_include_directories(${CMAKE_PROJECT_NAME} INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/External/fmt/include)

if(WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        wsock32 
        ws2_32
        bcrypt
    )
endif()

# Set compile options
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN 
    AR_EXPORTS
    NOMINMAX
    _WIN32_WINNT=0x0601
)

if (WIN32)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLATFORM_UNIX)
endif()

SetMaxWarningLevel(${CMAKE_PROJECT_NAME})
SetVisualStudioDebuggerOptions(${CMAKE_PROJECT_NAME})

# Install library
install(TARGETS ${CMAKE_PROJECT_NAME}
    DESTINATION plugin/usd)

# Install plugInfo
if(WIN32)
    set(PLUGIN_LIBRARY_NAME ${CMAKE_PROJECT_NAME}.dll)
elseif(UNIX)
    set(PLUGIN_LIBRARY_NAME lib${CMAKE_PROJECT_NAME}.so)
endif()

configure_file(Plugin/plugInfo.json Plugin/plugInfo.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Plugin/plugInfo.json
    DESTINATION plugin/usd/${CMAKE_PROJECT_NAME}/resources)

# Install headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Resolver.h ${CMAKE_CURRENT_SOURCE_DIR}/Context.h ${CMAKE_CURRENT_SOURCE_DIR}/Data.h ${CMAKE_CURRENT_SOURCE_DIR}/FileFormat.h
    DESTINATION include/RenderStudioResolver)

# Export CMake target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    EXPORT_NAME Resolver
)

export(TARGETS ${CMAKE_PROJECT_NAME}
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioResolverConfig.cmake
)
