# clang-tidy integration

function(enable_clang_tidy target_name)
    if(NOT DEVDASH_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY}"
        )
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
endfunction()
