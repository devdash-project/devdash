# clang-tidy integration

function(enable_clang_tidy target_name)
    if(NOT DEVDASH_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        # Exclude auto-generated Qt files from analysis
        # Pattern: build/**/.rcc/** and build/**/moc_** and build/**/*_autogen/**
        set(CLANG_TIDY_COMMAND
            "${CLANG_TIDY}"
            "--line-filter=[{\"name\":\".*\\\\.rcc/.*\",\"lines\":[[0,0]]},{\"name\":\".*/moc_.*\",\"lines\":[[0,0]]},{\"name\":\".*_autogen/.*\",\"lines\":[[0,0]]},{\"name\":\".*qmlcache.*\",\"lines\":[[0,0]]}]"
        )
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
        )
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
endfunction()
