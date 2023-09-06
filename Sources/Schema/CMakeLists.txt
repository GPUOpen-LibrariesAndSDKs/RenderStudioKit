cmake_minimum_required(VERSION 3.12)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set (CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(RenderStudioSchema SHARED ${SOURCES})

# Link libraries
target_link_libraries(RenderStudioSchema PRIVATE
    Boost::boost
    Boost::python
    usd
    RenderStudioLogger
    RenderStudioNetworking
)

# Set compile options
target_compile_definitions(RenderStudioSchema PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN 
    NOMINMAX
    _WIN32_WINNT=0x0601
    RENDERSTUDIOSCHEMA_EXPORTS
    VS_PLATFORM_TOOLSET=${VS_NUMERIC_PLATFORM_TOOLSET}
)

# Install library
install(TARGETS RenderStudioSchema
    DESTINATION plugin/usd)

# Install plugInfo
if(WIN32)
    set(PLUGIN_LIBRARY_NAME RenderStudioSchema.dll)
elseif(UNIX)
    set(PLUGIN_LIBRARY_NAME libRenderStudioSchema.so)
endif()

configure_file(plugInfo.json plugInfo.json @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION plugin/usd/RenderStudioSchema/resources)

# Install headers
file(GLOB HEADERS *.h)
install(FILES ${HEADERS} DESTINATION include/RenderStudioResolver)

# Export CMake target
set_target_properties(RenderStudioSchema PROPERTIES
    EXPORT_NAME Schema
)

export(TARGETS RenderStudioSchema
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioSchemaConfig.cmake
)
