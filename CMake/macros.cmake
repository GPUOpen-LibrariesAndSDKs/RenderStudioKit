function(SetMaxWarningLevel Project)
    if(MSVC)
        target_compile_options(${Project} PRIVATE 
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
            -Weverything
            -Wno-pre-c++17-compat
            -Wno-c++98-compat
            -Wno-weak-vtables
            -Wno-padded
            -Wno-exit-time-destructors
            -Wno-global-constructors
            -Wno-ctad-maybe-unsupported
        )
    endif()
endfunction(SetMaxWarningLevel)
