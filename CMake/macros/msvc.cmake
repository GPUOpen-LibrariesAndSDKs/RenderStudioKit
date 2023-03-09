# Configuration
set(RS_STORAGE_SERVICE_URL "http://127.0.0.1:5002")
set(RS_THUMBNAIL_SERVICE_URL "http://127.0.0.1:5003")

# Unfortunately CMake not allow to modify INSTALL target properties, so just create INSTALL.vcxproj.user file to override debugger properties

function(SetVisualStudioDebuggerOptions)
    # Helper variables
    get_filename_component(RS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/.. ABSOLUTE)
    set(RS_BUILD_DIR ${RS_INSTALL_DIR}/../)

    set(RS_RUNTIME_PATH ${RS_INSTALL_DIR}/USD/lib)
    list(APPEND RS_RUNTIME_PATH ${RS_INSTALL_DIR}/USD/bin)
    list(APPEND RS_RUNTIME_PATH ${RS_INSTALL_DIR}/USD/plugin/usd)
    list(APPEND RS_RUNTIME_PATH ${RS_INSTALL_DIR}/Engine/UsdRenderer)
    find_package (Python3 3.7 EXACT REQUIRED COMPONENTS Interpreter Development)
    list(APPEND RS_RUNTIME_PATH ${Python3_RUNTIME_LIBRARY_DIRS})

    # Visual Studio debugger options
    set(RS_VS_DEBUGGER_COMMAND_ARGUMENTS 
        "--log-level=debug --storage-service-url=${RS_STORAGE_SERVICE_URL} --thumbnail-service-url=${RS_THUMBNAIL_SERVICE_URL}")

    set(RS_VS_DEBUGGER_ENVIRONMENT 
        "PATH=${RS_RUNTIME_PATH}")

    if (EXISTS "${RS_BUILD_DIR}/Engine/StreamingApp/RelWithDebInfo")
        set(RS_VS_DEBUGGER_COMMAND 
            "${RS_BUILD_DIR}/Engine/StreamingApp/RelWithDebInfo/StreamingApp.exe")
    else()
        set(RS_VS_DEBUGGER_COMMAND 
            "${RS_BUILD_DIR}/Engine/StreamingApp/Release/StreamingApp.exe")
    endif()

    set(RS_VS_DEBUGGER_WORKING_DIRECTORY 
        "${RS_INSTALL_DIR}/Engine/StreamingApp")

    configure_file(../CMake/templates/INSTALL.vcxproj.user.in ../INSTALL.vcxproj.user)
endfunction()
