function(find_pybind11)
    if(TARGET pybind11::module)
        message(STATUS "pybind11::module target already imported")
    elseif(openPMD_pybind11_src)
        message(STATUS "Compiling local pybind11 ...")
        message(STATUS "pybind11 source path: ${openPMD_pybind11_src}")
        if(NOT IS_DIRECTORY ${openPMD_pybind11_src})
            message(FATAL_ERROR "Specified directory openPMD_pybind11_src='${openPMD_pybind11_src}' does not exist!")
        endif()
    elseif(openPMD_USE_INTERNAL_PYBIND11)
        message(STATUS "Downloading pybind11 ...")
        message(STATUS "pybind11 repository: ${openPMD_pybind11_repo} (${openPMD_pybind11_branch})")
        include(FetchContent)
    endif()
    if(TARGET pybind11::module)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_PYBIND11 OR openPMD_pybind11_src)
        if(openPMD_pybind11_src)
            add_subdirectory(${openPMD_pybind11_src} _deps/localpybind11-build/)
        else()
            FetchContent_Declare(fetchedpybind11
                GIT_REPOSITORY ${openPMD_pybind11_repo}
                GIT_TAG        ${openPMD_pybind11_branch}
                BUILD_IN_SOURCE 0
            )
            FetchContent_MakeAvailable(fetchedpybind11)

            # advanced fetch options
            mark_as_advanced(FETCHCONTENT_BASE_DIR)
            mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
            mark_as_advanced(FETCHCONTENT_QUIET)
            mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDpybind11)
            mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
            mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDpybind11)
        endif()
    elseif(NOT openPMD_USE_INTERNAL_PYBIND11)
        if(openPMD_USE_PYTHON STREQUAL AUTO)
            find_package(pybind11 2.12.0 CONFIG)
        elseif(openPMD_USE_PYTHON)
            find_package(pybind11 2.12.0 CONFIG REQUIRED)
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

# Git fetcher
set(openPMD_pybind11_repo "https://github.com/pybind/pybind11.git"
    CACHE STRING
    "Repository URI to pull and build pybind11 from if(openPMD_USE_INTERNAL_PYBIND11)")
set(openPMD_pybind11_branch "v2.12.0"
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
