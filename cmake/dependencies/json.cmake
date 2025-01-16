function(find_json)
    if(TARGET nlohmann_json::nlohmann_json)
        message(STATUS "nlohmann_json::nlohmann_json target already imported")
    elseif(openPMD_USE_INTERNAL_JSON)
        if(openPMD_json_src)
            message(STATUS "Compiling local nlohmann_json ...")
            message(STATUS "nlohmann_json source path: ${openPMD_json_src}")
            if(NOT IS_DIRECTORY ${openPMD_json_src})
                message(FATAL_ERROR "Specified directory openPMD_json_src='${openPMD_json_src}' does not exist!")
            endif()
        elseif(openPMD_json_tar)
            message(STATUS "Downloading nlohmann_json ...")
            message(STATUS "nlohmann_json source: ${openPMD_json_tar}")
        elseif(openPMD_json_branch)
            message(STATUS "Downloading nlohmann_json ...")
            message(STATUS "nlohmann_json repository: ${openPMD_json_repo} (${openPMD_json_branch})")
        endif()
    endif()
    if(TARGET nlohmann_json::nlohmann_json)
        # nothing to do, target already exists in the superbuild
    elseif(openPMD_USE_INTERNAL_JSON AND openPMD_json_src)
        add_subdirectory(${openPMD_json_src} _deps/localnlohmann_json-build/)
    elseif(openPMD_USE_INTERNAL_JSON AND (openPMD_json_tar OR openPMD_json_branch))
        include(FetchContent)
        if(openPMD_json_tar)
            FetchContent_Declare(fetchednlohmann_json
                URL             ${openPMD_json_tar}
                URL_HASH        ${openPMD_json_tar_hash}
                BUILD_IN_SOURCE OFF
            )
        else()
            FetchContent_Declare(fetchednlohmann_json
                GIT_REPOSITORY ${openPMD_json_repo}
                GIT_TAG        ${openPMD_json_branch}
                BUILD_IN_SOURCE OFF
            )
        endif()
        FetchContent_MakeAvailable(fetchednlohmann_json)

        # advanced fetch options
        mark_as_advanced(FETCHCONTENT_BASE_DIR)
        mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
        mark_as_advanced(FETCHCONTENT_QUIET)
        #mark_as_advanced(FETCHCONTENT_SOURCE_DIR_FETCHEDnlohmann_json)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
        #mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_FETCHEDnlohmann_json)
    elseif(NOT openPMD_USE_INTERNAL_JSON)
        find_package(nlohmann_json 3.9.1 CONFIG REQUIRED)
        message(STATUS "nlohmann_json: Found version '${nlohmann_json_VERSION}'")
    endif()
endfunction()

# local source-tree
set(openPMD_json_src ""
    CACHE PATH
    "Local path to nlohmann_json source directory (preferred if set)")

# tarball fetcher
set(openPMD_json_tar "https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz"
    CACHE STRING
    "Remote tarball link to pull and build nlohmann_json from if(openPMD_USE_INTERNAL_JSON)")
set(openPMD_json_tar_hash "SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406"
    CACHE STRING
    "Hash checksum of the tarball of nlohmann_json if(openPMD_USE_INTERNAL_JSON)")

# Git fetcher
set(openPMD_json_repo "https://github.com/nlohmann/json.git"
    CACHE STRING
    "Repository URI to pull and build nlohmann_json from if(openPMD_USE_INTERNAL_JSON)")
set(openPMD_json_branch "v3.11.3"
    CACHE STRING
    "Repository branch for openPMD_json_repo if(openPMD_USE_INTERNAL_JSON)")

find_json()
