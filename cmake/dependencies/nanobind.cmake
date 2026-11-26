function(find_nanobind)
    if(COMMAND nanobind_add_module)
        message(STATUS "nanobind CMake functions already imported")
    elseif(openPMD_USE_INTERNAL_NANOBIND)
        if(openPMD_nanobind_src)
            message(STATUS "Compiling local nanobind ...")
            message(STATUS "nanobind source path: ${openPMD_nanobind_src}")
            if(NOT IS_DIRECTORY ${openPMD_nanobind_src})
                message(FATAL_ERROR "Specified directory openPMD_nanobind_src='${openPMD_nanobind_src}' does not exist!")
            endif()
        elseif(openPMD_nanobind_tar)
            message(STATUS "Downloading nanobind ...")
            message(STATUS "nanobind source: ${openPMD_nanobind_tar}")
        elseif(openPMD_nanobind_branch)
            message(STATUS "Downloading nanobind ...")
            message(STATUS "nanobind repository: ${openPMD_nanobind_repo} (${openPMD_nanobind_branch})")
        endif()
    endif()

    if(COMMAND nanobind_add_module)
        # nothing to do, functions already exist in the superbuild
    elseif(openPMD_USE_INTERNAL_NANOBIND AND openPMD_nanobind_src)
        add_subdirectory(${openPMD_nanobind_src} _deps/localnanobind-build/)
    elseif(openPMD_USE_INTERNAL_NANOBIND AND (openPMD_nanobind_tar OR openPMD_nanobind_branch))
        include(FetchContent)
        if(openPMD_nanobind_tar)
            # note: GitHub release tarballs lack the git submodules of
            #       nanobind, the PyPI source distribution contains them
            FetchContent_Declare(fetchednanobind
                URL             ${openPMD_nanobind_tar}
                URL_HASH        ${openPMD_nanobind_tar_hash}
                BUILD_IN_SOURCE OFF
            )
        else()
            FetchContent_Declare(fetchednanobind
                GIT_REPOSITORY ${openPMD_nanobind_repo}
                GIT_TAG        ${openPMD_nanobind_branch}
                BUILD_IN_SOURCE OFF
            )
        endif()
        FetchContent_MakeAvailable(fetchednanobind)

        # advanced fetch options
        mark_as_advanced(FETCHCONTENT_BASE_DIR)
        mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_QUIET)
        mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDNANOBIND)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDNANOBIND)
    elseif(NOT openPMD_USE_INTERNAL_NANOBIND)
        # pip-installed nanobind: ask the Python interpreter for its location
        execute_process(
            COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE _nanobind_cmake_dir
            ERROR_QUIET)
        if(openPMD_USE_PYTHON STREQUAL AUTO)
            find_package(nanobind 3.0.0 CONFIG HINTS "${_nanobind_cmake_dir}")
        elseif(openPMD_USE_PYTHON)
            find_package(nanobind 3.0.0 CONFIG REQUIRED HINTS "${_nanobind_cmake_dir}")
        endif()
        if(nanobind_FOUND)
            message(STATUS "nanobind: Found version '${nanobind_VERSION}'")
        endif()
    endif()
endfunction()

# local source-tree
set(openPMD_nanobind_src ""
    CACHE PATH
    "Local path to nanobind source directory (preferred if set)")

# tarball fetcher
set(openPMD_nanobind_tar "https://files.pythonhosted.org/packages/source/n/nanobind/nanobind-3.1.0.tar.gz"
        CACHE STRING
        "Remote tarball link to pull and build nanobind from if(openPMD_USE_INTERNAL_NANOBIND)")
set(openPMD_nanobind_tar_hash "SHA256=6661e3dc1434d78781ccf7913442d90361dab395c3424efadb63bbb6ddea7ea3"
        CACHE STRING
        "Hash checksum of the tarball of nanobind if(openPMD_USE_INTERNAL_NANOBIND)")

# Git fetcher
set(openPMD_nanobind_repo "https://github.com/wjakob/nanobind.git"
    CACHE STRING
    "Repository URI to pull and build nanobind from if(openPMD_USE_INTERNAL_NANOBIND)")
set(openPMD_nanobind_branch "v3.1.0"
    CACHE STRING
    "Repository branch for openPMD_nanobind_repo if(openPMD_USE_INTERNAL_NANOBIND)")

if(openPMD_USE_PYTHON STREQUAL AUTO)
    find_package(Python 3.10.0 COMPONENTS Interpreter Development.Module)
elseif(openPMD_USE_PYTHON)
    find_package(Python 3.10.0 COMPONENTS Interpreter Development.Module REQUIRED)
else()
    set(openPMD_HAVE_PYTHON FALSE)
endif()

if(Python_FOUND)
    find_nanobind()
endif()

if(COMMAND nanobind_add_module)
    set(openPMD_HAVE_PYTHON TRUE)
else()
    set(openPMD_HAVE_PYTHON FALSE)
endif()
