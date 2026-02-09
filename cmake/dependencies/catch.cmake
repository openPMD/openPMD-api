function(catch_preconfigure)
    set(_old_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endfunction()

function(catch_postconfigure)
    set(BUILD_SHARED_LIBS ${_old_BUILD_SHARED_LIBS})
    unset(_old_BUILD_SHARED_LIBS)
    # Mark Catch2 as system code to suppress warnings and set position independent code
    # Ensure Catch2 is built with PIC so it can be linked into shared libraries
    set_target_properties(Catch2 PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:Catch2,INTERFACE_INCLUDE_DIRECTORIES>"
    )
    if(CMAKE_CXX_CLANG_TIDY)
        set_target_properties(Catch2 PROPERTIES CXX_CLANG_TIDY "")
    endif()
endfunction()

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
        catch_preconfigure()
        add_subdirectory(${openPMD_catch_src} _deps/localCatch2-build/)
        catch_postconfigure()
    elseif(openPMD_USE_INTERNAL_CATCH AND (openPMD_catch_tar OR openPMD_catch_branch))
        catch_preconfigure()
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
        catch_postconfigure()
        # advanced fetch options
        mark_as_advanced(FETCHCONTENT_BASE_DIR)
        mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_QUIET)
        #mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDCatch2)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
        #mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDCatch2)
    elseif(NOT openPMD_USE_INTERNAL_CATCH)
        find_package(Catch2 3.0.0 CONFIG REQUIRED)
        message(STATUS "Catch2: Found version '${Catch2_VERSION}'")
    endif()
endfunction()

# local source-tree
set(openPMD_catch_src ""
    CACHE PATH
    "Local path to Catch2 source directory (preferred if set)")

# tarball fetcher
set(openPMD_catch_tar "https://github.com/catchorg/Catch2/archive/refs/tags/v3.11.0.tar.gz"
    CACHE STRING
    "Remote tarball link to pull and build Catch2 from if(openPMD_USE_INTERNAL_CATCH)")
set(openPMD_catch_tar_hash "SHA256=82fa1cb59dc28bab220935923f7469b997b259eb192fb9355db62da03c2a3137"
    CACHE STRING
    "Hash checksum of the tarball of Catch2 if(openPMD_USE_INTERNAL_CATCH)")

# Git fetcher
set(openPMD_catch_repo "https://github.com/catchorg/Catch2.git"
    CACHE STRING
    "Repository URI to pull and build Catch2 from if(openPMD_USE_INTERNAL_CATCH)")
set(openPMD_catch_branch "v3.11.0"
    CACHE STRING
    "Repository branch for openPMD_catch_repo if(openPMD_USE_INTERNAL_CATCH)")

find_catch2()
