function(SetMaxWarningLevel Target)
    if(MSVC)
        target_compile_options(${Target} PRIVATE
            /WX
            /W4
            /permissive-
            /w14640
            /wd4506)

        if (MAYA_SUPPORT OR HOUDINI_SUPPORT)
            target_compile_options(${Target} PRIVATE
                /Zc:inline-
                /bigobj)
        endif()
    else()
        target_compile_options(${Target} PRIVATE
            -Werror
            -Wall
            -Wextra
            -Wshadow
            -Wnon-virtual-dtor
            -pedantic
            -Wno-unknown-pragmas
            -Wno-deprecated
        )
    endif()

    if (NOT MSVC)
        set_property(TARGET ${Target} PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
endfunction()

function(ExportTargetConfig Target ExportName)
    set_target_properties(${Target} PROPERTIES
        EXPORT_NAME ${ExportName}
    )

    export(TARGETS ${Target} usd sdf
        NAMESPACE RenderStudio::
        FILE ${CMAKE_INSTALL_PREFIX}/cmake/${Target}Config.cmake
    )
endfunction()

function(SetDefaultCompileDefinitions Target)
    target_compile_definitions(${Target} PRIVATE
        HAVE_SNPRINTF
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601
    )

    if (WIN32)
        target_compile_definitions(${Target} PRIVATE PLATFORM_WINDOWS)
    endif()

    if (UNIX)
        target_compile_definitions(${Target} PRIVATE PLATFORM_UNIX)
    endif()
endfunction()
