function(find_pybind11)
    if(TARGET pybind11::module)
        message(STATUS "pybind11::module target already imported")
    elseif(openPMD_USE_INTERNAL_PYBIND11)
        if(openPMD_pybind11_src)
            message(STATUS "Compiling local pybind11 ...")
            message(STATUS "pybind11 source path: ${openPMD_pybind11_src}")
            if(NOT IS_DIRECTORY ${openPMD_pybind11_src})
                message(FATAL_ERROR "Specified directory openPMD_pybind11_src='${openPMD_pybind11_src}' does not exist!")
            endif()
        elseif(openPMD_pybind11_tar)
            message(STATUS "Downloading pybind11 ...")
            message(STATUS "pybind11 source: ${openPMD_pybind11_tar}")
        elseif(openPMD_pybind11_branch)
            message(STATUS "Downloading pybind11 ...")
            message(STATUS "pybind11 repository: ${openPMD_pybind11_repo} (${openPMD_pybind11_branch})")
        endif()
    endif()

    # rely on our find_package(Python ...) call
    # https://pybind11.readthedocs.io/en/stable/compiling.html#modules-with-cmake
    set(PYBIND11_FINDPYTHON ON)

    if(TARGET pybind11::module)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_PYBIND11 AND openPMD_pybind11_src)
        add_subdirectory(${openPMD_pybind11_src} _deps/localpybind11-build/)
    elseif(openPMD_USE_INTERNAL_PYBIND11 AND (openPMD_pybind11_tar OR openPMD_pybind11_branch))
        include(FetchContent)
        if(openPMD_pybind11_tar)
            FetchContent_Declare(fetchedpybind11
                    URL             ${openPMD_pybind11_tar}
                    URL_HASH        ${openPMD_pybind11_tar_hash}
                    BUILD_IN_SOURCE OFF
            )
        else()
            FetchContent_Declare(fetchedpybind11
                GIT_REPOSITORY ${openPMD_pybind11_repo}
                GIT_TAG        ${openPMD_pybind11_branch}
                BUILD_IN_SOURCE OFF
            )
        endif()
        FetchContent_MakeAvailable(fetchedpybind11)

        # advanced fetch options
        mark_as_advanced(FETCHCONTENT_BASE_DIR)
        mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_QUIET)
        mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDpybind11)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDpybind11)
    elseif(NOT openPMD_USE_INTERNAL_PYBIND11)
        if(openPMD_USE_PYTHON STREQUAL AUTO)
            find_package(pybind11 2.13.0 CONFIG)
        elseif(openPMD_USE_PYTHON)
            find_package(pybind11 2.13.0 CONFIG REQUIRED)
        endif()
        if(TARGET pybind11::module)
            message(STATUS "pybind11: Found version '${pybind11_VERSION}'")
        endif()
    endif()
endfunction()

# local source-tree
set(openPMD_pybind11_src ""
    CACHE PATH
    "Local path to pybind11 source directory (preferred if set)")

# tarball fetcher
set(openPMD_pybind11_tar "https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.tar.gz"
        CACHE STRING
        "Remote tarball link to pull and build pybind11 from if(openPMD_USE_INTERNAL_PYBIND11)")
set(openPMD_pybind11_tar_hash "SHA256=e08cb87f4773da97fa7b5f035de8763abc656d87d5773e62f6da0587d1f0ec20"
        CACHE STRING
        "Hash checksum of the tarball of pybind11 if(openPMD_USE_INTERNAL_PYBIND11)")

# Git fetcher
set(openPMD_pybind11_repo "https://github.com/pybind/pybind11.git"
    CACHE STRING
    "Repository URI to pull and build pybind11 from if(openPMD_USE_INTERNAL_PYBIND11)")
set(openPMD_pybind11_branch "v2.13.6"
    CACHE STRING
    "Repository branch for openPMD_pybind11_repo if(openPMD_USE_INTERNAL_PYBIND11)")

if(openPMD_USE_PYTHON STREQUAL AUTO)
    find_package(Python 3.7.0 COMPONENTS Interpreter Development.Module)
elseif(openPMD_USE_PYTHON)
    find_package(Python 3.7.0 COMPONENTS Interpreter Development.Module REQUIRED)
else()
    set(openPMD_HAVE_PYTHON FALSE)
endif()

if(Python_FOUND)
    find_pybind11()
endif()

if(TARGET pybind11::module)
    set(openPMD_HAVE_PYTHON TRUE)
else()
    set(openPMD_HAVE_PYTHON FALSE)
endif()
