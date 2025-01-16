function(find_catch2)
    if(TARGET Catch2::Catch2)
        message(STATUS "Catch2::Catch2 target already imported")
    elseif(openPMD_USE_INTERNAL_CATCH)
        if(openPMD_catch_src)
            message(STATUS "Compiling local Catch2 ...")
            message(STATUS "Catch2 source path: ${openPMD_catch_src}")
            if(NOT IS_DIRECTORY ${openPMD_catch_src})
                message(FATAL_ERROR "Specified directory openPMD_catch_src='${openPMD_catch_src}' does not exist!")
            endif()
        elseif(openPMD_catch_tar)
            message(STATUS "Downloading Catch2 ...")
            message(STATUS "Catch2 source: ${openPMD_catch_tar}")
        elseif(openPMD_catch_branch)
            message(STATUS "Downloading Catch2 ...")
            message(STATUS "Catch2 repository: ${openPMD_catch_repo} (${openPMD_catch_branch})")
        endif()
    endif()
    if(TARGET Catch2::Catch2)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_CATCH AND openPMD_catch_src)
        add_subdirectory(${openPMD_catch_src} _deps/localCatch2-build/)
    elseif(openPMD_USE_INTERNAL_CATCH AND (openPMD_catch_tar OR openPMD_catch_branch))
        include(FetchContent)
        if(openPMD_catch_tar)
            FetchContent_Declare(fetchedCatch2
                URL             ${openPMD_catch_tar}
                URL_HASH        ${openPMD_catch_tar_hash}
                BUILD_IN_SOURCE OFF
            )
        else()
            FetchContent_Declare(fetchedCatch2
                GIT_REPOSITORY ${openPMD_catch_repo}
                GIT_TAG        ${openPMD_catch_branch}
                BUILD_IN_SOURCE OFF
            )
        endif()
        FetchContent_MakeAvailable(fetchedCatch2)

        # advanced fetch options
        mark_as_advanced(FETCHCONTENT_BASE_DIR)
        mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_QUIET)
        #mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDCatch2)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
        #mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDCatch2)
    elseif(NOT openPMD_USE_INTERNAL_CATCH)
        find_package(Catch2 2.13.10 CONFIG REQUIRED)
        message(STATUS "Catch2: Found version '${Catch2_VERSION}'")
    endif()
endfunction()

# local source-tree
set(openPMD_catch_src ""
    CACHE PATH
    "Local path to Catch2 source directory (preferred if set)")

# tarball fetcher
set(openPMD_catch_tar "https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.10.tar.gz"
    CACHE STRING
    "Remote tarball link to pull and build Catch2 from if(openPMD_USE_INTERNAL_CATCH)")
set(openPMD_catch_tar_hash "SHA256=d54a712b7b1d7708bc7a819a8e6e47b2fde9536f487b89ccbca295072a7d9943"
    CACHE STRING
    "Hash checksum of the tarball of Catch2 if(openPMD_USE_INTERNAL_CATCH)")

# Git fetcher
set(openPMD_catch_repo "https://github.com/catchorg/Catch2.git"
    CACHE STRING
    "Repository URI to pull and build Catch2 from if(openPMD_USE_INTERNAL_CATCH)")
set(openPMD_catch_branch "v2.13.10"
    CACHE STRING
    "Repository branch for openPMD_catch_repo if(openPMD_USE_INTERNAL_CATCH)")

find_catch2()
