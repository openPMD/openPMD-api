function(find_toml11)
    if(TARGET toml11::toml11)
        message(STATUS "toml11::toml11 target already imported")
    elseif(openPMD_USE_INTERNAL_TOML11)
        if(openPMD_toml11_src)
            message(STATUS "Compiling local toml11 ...")
            message(STATUS "toml11 source path: ${openPMD_toml11_src}")
            if(NOT IS_DIRECTORY ${openPMD_toml11_src})
                message(FATAL_ERROR "Specified directory openPMD_toml11_src='${openPMD_toml11_src}' does not exist!")
            endif()
        elseif(openPMD_toml11_tar)
            message(STATUS "Downloading toml11 ...")
            message(STATUS "toml11 source: ${openPMD_toml11_tar}")
        elseif(openPMD_toml11_branch)
            message(STATUS "Downloading toml11 ...")
            message(STATUS "toml11 repository: ${openPMD_toml11_repo} (${openPMD_toml11_branch})")
        endif()
    endif()
    if(TARGET toml11::toml11)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_TOML11 AND openPMD_toml11_src)
        add_subdirectory(${openPMD_toml11_src} _deps/localtoml11-build/)
    elseif(openPMD_USE_INTERNAL_TOML11 AND (openPMD_toml11_tar OR openPMD_toml11_branch))
        include(FetchContent)
        if(openPMD_toml11_tar)
            FetchContent_Declare(fetchedtoml11
                    URL             ${openPMD_toml11_tar}
                    URL_HASH        ${openPMD_toml11_tar_hash}
                    BUILD_IN_SOURCE OFF
            )
        else()
            FetchContent_Declare(fetchedtoml11
                GIT_REPOSITORY ${openPMD_toml11_repo}
                GIT_TAG        ${openPMD_toml11_branch}
                BUILD_IN_SOURCE OFF
            )
        endif()
        FetchContent_MakeAvailable(fetchedtoml11)

        # advanced fetch options
        mark_as_advanced(FETCHCONTENT_BASE_DIR)
        mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_QUIET)
        #mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDtoml11)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
        #mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDtoml11)
    elseif(NOT openPMD_USE_INTERNAL_TOML11)
        # toml11 4.0 was a breaking change. This is reflected in the library's CMake
        # logic: version 4.0 is not accepted by a call to find_package(toml11 3.7).
        # Since we support both incompatible versions, we use two find_package()
        # calls. Search for version 4 first in order to prefer that
        # in (the unlikely) case that both versions are installed.
        find_package(toml11 4.0 CONFIG QUIET)
        if(NOT toml11_FOUND)
            find_package(toml11 3.7.1 CONFIG REQUIRED)
        endif()
        message(STATUS "toml11: Found version '${toml11_VERSION}'")
    endif()
endfunction()

# local source-tree
set(openPMD_toml11_src ""
    CACHE PATH
    "Local path to toml11 source directory (preferred if set)")

# tarball fetcher
set(openPMD_toml11_tar "https://github.com/ToruNiina/toml11/archive/refs/tags/v4.2.0.tar.gz"
        CACHE STRING
        "Remote tarball link to pull and build toml11 from if(openPMD_USE_INTERNAL_TOML11)")
set(openPMD_toml11_tar_hash "SHA256=9287971cd4a1a3992ef37e7b95a3972d1ae56410e7f8e3f300727ab1d6c79c2c"
        CACHE STRING
        "Hash checksum of the tarball of toml11 if(openPMD_USE_INTERNAL_TOML11)")

# Git fetcher
set(openPMD_toml11_repo "https://github.com/ToruNiina/toml11.git"
    CACHE STRING
    "Repository URI to pull and build toml11 from if(openPMD_USE_INTERNAL_TOML11)")
set(openPMD_toml11_branch "v3.7.1"
    CACHE STRING
    "Repository branch for openPMD_toml11_repo if(openPMD_USE_INTERNAL_TOML11)")

find_toml11()
