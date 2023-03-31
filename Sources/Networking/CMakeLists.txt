cmake_minimum_required(VERSION 3.12)
project(RenderStudioNetworking)

set(CMAKE_CXX_STANDARD 20)

# Create target
file(GLOB SOURCES *.h *.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Find packages
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(BinaryOpenSSL REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Boost::boost 
    OpenSSL::SSL 
    RenderStudioLogger
    RenderStudioExternal)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Crypt32)
endif()

# Set compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    HAVE_SNPRINTF 
    WIN32_LEAN_AND_MEAN NOMINMAX
    _WIN32_WINNT=0x0601)

setmaxwarninglevel(${PROJECT_NAME})

# Install headers
install(FILES WebsocketClient.h RestClient.h MaterialLibraryApi.h Url.h
    DESTINATION include/RenderStudioResolver/Networking)

install(FILES ${OPENSSL_SSL_BINARY} ${OPENSSL_CRYPTO_BINARY}
    DESTINATION lib)

# Export CMake target
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME Networking)

export(
    TARGETS ${PROJECT_NAME}
    NAMESPACE RenderStudio::
    FILE ${CMAKE_INSTALL_PREFIX}/cmake/RenderStudioNetworkingConfig.cmake)
