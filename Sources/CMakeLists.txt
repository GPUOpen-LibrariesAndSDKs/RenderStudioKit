cmake_minimum_required(VERSION 3.12)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set (CMAKE_CXX_STANDARD 20)

set (USD_MONOLITHIC_REQUIRED 1)

if (MAYA_SUPPORT)
    set (USD_MONOLITHIC_REQUIRED 0)
endif()

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

# Find packages

if (USD_MONOLITHIC_REQUIRED)
    find_package(USDMonolithic REQUIRED)
else()
    if(MAYA_SUPPORT)
        find_package(pxr CONFIG REQUIRED)

        if (pxr_FOUND)
           find_package(PythonInterp 3 REQUIRED)
           find_package(PythonLibs 3 REQUIRED)
           list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../Boost_1_76_0/bin")

           include_directories(
               ${PYTHON_INCLUDE_DIRS})

           target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
               ${PYTHON_LIBRARIES}
               usd)
        endif()
    endif()
endif()

find_package(Boost REQUIRED COMPONENTS python)

add_subdirectory(External)
add_subdirectory(Logger)
add_subdirectory(Networking)
add_subdirectory(Serialization)

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Boost::boost
    Boost::python
    ar
    sdf
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
file(GLOB HEADERS *.h)
install(FILES ${HEADERS} DESTINATION include/RenderStudioResolver)

# Export CMake target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    EXPORT_NAME Resolver
)

export(TARGETS ${CMAKE_PROJECT_NAME}
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioResolverConfig.cmake
)
