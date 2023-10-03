function(SetMaxWarningLevel Project)
    if(MSVC)
        target_compile_options(${Project} PRIVATE
            /WX
            /W4
            /permissive-
            /w14640
            /wd4506)

        if (MAYA_SUPPORT)
            target_compile_options(${Project} PRIVATE
                /Zc:inline-
                /bigobj)
        endif()
    else()
        target_compile_options(${Project} PRIVATE
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
        set_property(TARGET ${Project} PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
endfunction(SetMaxWarningLevel)
