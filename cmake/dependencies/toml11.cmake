function(find_toml11)
    if(TARGET toml11::toml11)
        message(STATUS "toml11::toml11 target already imported")
    elseif(openPMD_toml11_src)
        message(STATUS "Compiling local toml11 ...")
        message(STATUS "toml11 source path: ${openPMD_toml11_src}")
        if(NOT IS_DIRECTORY ${openPMD_toml11_src})
            message(FATAL_ERROR "Specified directory openPMD_toml11_src='${openPMD_toml11_src}' does not exist!")
        endif()
    elseif(openPMD_toml11_tar)
        message(STATUS "Downloading toml11 ...")
        message(STATUS "toml11 source: ${openPMD_toml11_tar}")
    elseif(openPMD_USE_INTERNAL_TOML11)
        message(STATUS "Downloading toml11 ...")
        message(STATUS "toml11 repository: ${openPMD_toml11_repo} (${openPMD_toml11_branch})")
    endif()
    if(TARGET toml11::toml11)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_TOML11 OR openPMD_toml11_src)
        if(openPMD_toml11_src)
            add_subdirectory(${openPMD_toml11_src} _deps/localtoml11-build/)
        else()
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
        endif()
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
set(openPMD_toml11_tar "https://github.com/ToruNiina/toml11/archive/refs/tags/v3.7.1.tar.gz"
        CACHE STRING
        "Remote tarball link to pull and build toml11 from if(openPMD_USE_INTERNAL_TOML11)")
set(openPMD_toml11_tar_hash "SHA256=afeaa9aa0416d4b6b2cd3897ca55d9317084103077b32a852247d8efd4cf6068"
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
