function(find_catch2)
    if(TARGET Catch2::Catch2)
        message(STATUS "Catch2::Catch2 target already imported")
    elseif(openPMD_catch_src)
        message(STATUS "Compiling local Catch2 ...")
        message(STATUS "Catch2 source path: ${openPMD_catch_src}")
        if(NOT IS_DIRECTORY ${openPMD_catch_src})
            message(FATAL_ERROR "Specified directory openPMD_catch_src='${openPMD_catch_src}' does not exist!")
        endif()
    elseif(openPMD_USE_INTERNAL_CATCH)
        message(STATUS "Downloading Catch2 ...")
        message(STATUS "Catch2 repository: ${openPMD_catch_repo} (${openPMD_catch_branch})")
        include(FetchContent)
    endif()
    if(TARGET Catch2::Catch2)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_CATCH OR openPMD_catch_src)
        if(openPMD_catch_src)
            add_subdirectory(${openPMD_catch_src} _deps/localCatch2-build/)
        else()
            FetchContent_Declare(fetchedCatch2
                GIT_REPOSITORY ${openPMD_catch_repo}
                GIT_TAG        ${openPMD_catch_branch}
                BUILD_IN_SOURCE 0
            )
            FetchContent_MakeAvailable(fetchedCatch2)

            # advanced fetch options
            mark_as_advanced(FETCHCONTENT_BASE_DIR)
            mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
            mark_as_advanced(FETCHCONTENT_QUIET)
            #mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDCatch2)
            mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
            #mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDCatch2)
        endif()
    elseif(NOT openPMD_USE_INTERNAL_CATCH)
        find_package(Catch2 2.13.10 CONFIG REQUIRED)
        message(STATUS "Catch2: Found version '${Catch2_VERSION}'")
    endif()
endfunction()

# local source-tree
set(openPMD_catch_src ""
    CACHE PATH
    "Local path to Catch2 source directory (preferred if set)")

# Git fetcher
set(openPMD_catch_repo "https://github.com/catchorg/Catch2.git"
    CACHE STRING
    "Repository URI to pull and build Catch2 from if(openPMD_USE_INTERNAL_CATCH)")
set(openPMD_catch_branch "v2.13.10"
    CACHE STRING
    "Repository branch for openPMD_catch_repo if(openPMD_USE_INTERNAL_CATCH)")

find_catch2()
