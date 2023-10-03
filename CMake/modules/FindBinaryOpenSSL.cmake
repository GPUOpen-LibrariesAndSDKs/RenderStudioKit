find_package(OpenSSL REQUIRED)

cmake_path(
    GET OPENSSL_INCLUDE_DIR
    PARENT_PATH OPENSSL_ROOT)

cmake_path(
    APPEND OPENSSL_ROOT bin
    OUTPUT_VARIABLE OPENSSL_BIN)

find_file(OPENSSL_SSL_BINARY
    NAMES libssl-3-x64.dll
    PATHS ${OPENSSL_BIN})

find_file(OPENSSL_CRYPTO_BINARY
    NAMES libcrypto-3-x64.dll
    PATHS ${OPENSSL_BIN})

if (NOT OPENSSL_SSL_BINARY OR NOT OPENSSL_CRYPTO_BINARY)
    message(FATAL_ERROR "Can't find OpenSSL dynamic library")
endif()
