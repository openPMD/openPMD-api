# Python bindings #############################################################
#
# This directory can be configured in two ways:
#
#  1. As part of the main openPMD-api build. The top-level CMakeLists.txt adds
#     this directory when Python bindings are enabled. In this case the C++
#     core library is built as part of the same project.
#
#  2. Standalone, against an already installed openPMD-api C++ library. This
#     makes it possible to build the Python bindings for several Python
#     versions without rebuilding the C++ core each time, and lets pure C++
#     dependents avoid a dependency on Python. Point CMake at this source
#     directory, e.g.:
#
#         cmake -S <openPMD-api>/src/binding/python -B build-python \
#               -DopenPMD_DIR=<prefix>/lib/cmake/openPMD
#         cmake --build build-python
#
#     See https://github.com/openPMD/openPMD-api/issues/1455

# Detect whether we are configured as part of the main openPMD-api C++ build
# (the `openPMD::openPMD` target already exists) or standalone against an
# installed C++ library.
set(openPMD_python_standalone TRUE)
if(TARGET openPMD::openPMD)
    set(openPMD_python_standalone FALSE)
endif()

if(openPMD_python_standalone)
    cmake_minimum_required(VERSION 3.22.0)
    project(openPMD-python LANGUAGES CXX)

    # let us reuse the module functions of the main project
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")
    include(openPMDFunctions)

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE "Release")
    endif()

    # path to the C++ sources/project root (for shared auxiliary files)
    set(openPMD_ROOT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
    set(openPMD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(openPMD_BINARY_DIR "${CMAKE_BINARY_DIR}")

    set(openPMD_USE_PYTHON ON CACHE STRING "Enable Python bindings")
    option(openPMD_USE_INTERNAL_PYBIND11
        "Use internally shipped pybind11" OFF)
    option(openPMD_INSTALL "Add installation targets" ON)
    option(openPMD_INSTALL_RPATH "Add RPATHs to installed binaries" ON)
    option(openPMD_USE_FILESYSTEM_HEADER
        "Enable filesystem header. May be disabled for old compiler versions."
        OFF)
    mark_as_advanced(openPMD_USE_FILESYSTEM_HEADER)
    option(openPMD_BUILD_NO_CFG_SUBPATH
        "For multi-config builds, do not append the config to build dir" OFF)
    mark_as_advanced(openPMD_BUILD_NO_CFG_SUBPATH)
    option(openPMD_BUILD_CLI_TOOLS "Install the Python command line tools" OFF)

    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    include(GNUInstallDirs)
    if(WIN32)
        set(openPMD_INSTALL_LIBDIR "Lib")
    else()
        set(openPMD_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
    endif()
    set(openPMD_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")

    # locate the installed C++ core library (defines openPMD::openPMD and sets
    # openPMD_HAVE_MPI / openPMD_HAVE_HDF5 / openPMD_HAVE_ADIOS2)
    find_package(openPMD CONFIG REQUIRED)

    # Python interpreter and pybind11
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/dependencies/pybind11.cmake)
endif()

if(openPMD_HAVE_PYTHON)
    add_library(openPMD.py MODULE
        openPMD.cpp
        auxiliary.cpp
        Access.cpp
        Attributable.cpp
        BaseRecordComponent.cpp
        ChunkInfo.cpp
        Dataset.cpp
        Datatype.cpp
        Error.cpp
        Helper.cpp
        Iteration.cpp
        IterationEncoding.cpp
        Mesh.cpp
        ParticlePatches.cpp
        ParticleSpecies.cpp
        PatchRecord.cpp
        PatchRecordComponent.cpp
        Pickle.cpp
        Record.cpp
        RecordComponent.cpp
        MeshRecordComponent.cpp
        Series.cpp
        UnitDimension.cpp
    )
    target_link_libraries(openPMD.py PRIVATE openPMD::openPMD)
    target_link_libraries(openPMD.py PRIVATE pybind11::module pybind11::windows_extras)

    set_filesystem_header_for_target(openPMD.py)

    # LTO/IPO: CMake target properties work well for 3.18+ and are buggy before
    set(_USE_PY_LTO ON)  # default shall be ON
    if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)  # overwrite default if defined
        if(NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(_USE_PY_LTO OFF)
        endif()
    endif()
    message(STATUS "Python LTO/IPO: ${_USE_PY_LTO}")
    set_target_properties(openPMD.py PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ${_USE_PY_LTO})
    unset(_USE_PY_LTO)

    if(EMSCRIPTEN)
        set_target_properties(openPMD.py PROPERTIES
            PREFIX "")
    else()
        pybind11_extension(openPMD.py)
    endif()
    if(NOT MSVC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug|RelWithDebInfo)
        pybind11_strip(openPMD.py)
    endif()

    set_target_properties(openPMD.py PROPERTIES CXX_VISIBILITY_PRESET "hidden"
                                                CUDA_VISIBILITY_PRESET "hidden")

    # Co-load safety: statically-embedded dependencies (HDF5, ADIOS2, blosc2,
    # and the openPMD core archive) are compiled without hidden visibility, so
    # their symbols would otherwise be re-exported from this Python extension.
    # If another extension loaded in the same process also statically embeds
    # them (ImpactX, WarpX, a second openPMD in pip wheels), the duplicate
    # exported symbols cross-bind, then two HDF5/openPMD instances are in one
    # Python process, and usually crash on the first file open. Export only
    # the module init symbol so each extension keeps its dependencies private.
    # Windows does not auto-export, so it needs nothing. Emscripten needs BOTH
    # halves: -sSIDE_MODULE=2 stops Pyodide's default -sSIDE_MODULE=1 from
    # whole-archiving and re-exporting every symbol, and the statically linked
    # deps must additionally be built -fvisibility=hidden
    # so their definitions are DSO-local. Only then can wasm-ld relax our GOT
    # references to HDF5 into direct calls into our own copy, instead of binding
    # to a co-loaded second HDF5 (h5py, ImpactX).
    if(APPLE)
        target_link_options(openPMD.py PRIVATE
            "LINKER:-exported_symbol,_PyInit_openpmd_api_cxx")
    elseif(EMSCRIPTEN)
        target_link_options(openPMD.py PRIVATE
            "SHELL:-sSIDE_MODULE=2"
            "SHELL:-sEXPORTED_FUNCTIONS=_PyInit_openpmd_api_cxx")
    elseif(NOT WIN32)
        target_link_options(openPMD.py PRIVATE "LINKER:--exclude-libs,ALL")
    endif()

    # ancient Clang releases
    #   https://github.com/openPMD/openPMD-api/issues/542
    #   https://pybind11.readthedocs.io/en/stable/faq.html#recursive-template-instantiation-exceeded-maximum-depth-of-256
    #   https://bugs.llvm.org/show_bug.cgi?id=18417
    #   https://github.com/llvm/llvm-project/commit/e55b4737c026ea2e0b44829e4115d208577a67b2
    if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" AND
        CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1) OR
       ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND
        CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0))
            message(STATUS "Clang: Passing -ftemplate-depth=1024")
            target_compile_options(openPMD.py
                PRIVATE -ftemplate-depth=1024)
    endif()

    if(WIN32)
        set(openPMD_INSTALL_PYTHONDIR_DEFAULT
            "${openPMD_INSTALL_LIBDIR}/site-packages")
    else()
        set(openPMD_INSTALL_PYTHONDIR_DEFAULT
            "${openPMD_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages"
        )
    endif()
    set(openPMD_INSTALL_PYTHONDIR "${openPMD_INSTALL_PYTHONDIR_DEFAULT}"
        CACHE STRING "Location for installed python package")
    set(openPMD_PYTHON_OUTPUT_DIRECTORY "${openPMD_BINARY_DIR}/${openPMD_INSTALL_PYTHONDIR}"
        CACHE STRING "Build directory for python modules")
    set_target_properties(openPMD.py PROPERTIES
        ARCHIVE_OUTPUT_NAME openpmd_api_cxx
        LIBRARY_OUTPUT_NAME openpmd_api_cxx
        COMPILE_PDB_NAME openpmd_api_cxx
        ARCHIVE_OUTPUT_DIRECTORY ${openPMD_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        LIBRARY_OUTPUT_DIRECTORY ${openPMD_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        RUNTIME_OUTPUT_DIRECTORY ${openPMD_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        PDB_OUTPUT_DIRECTORY ${openPMD_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        COMPILE_PDB_OUTPUT_DIRECTORY ${openPMD_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
    )
    # note: same as above, but for Multi-Config generators
    if(isMultiConfig)
        foreach(CFG IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER "${CFG}" CFG_UPPER)
            if(openPMD_BUILD_NO_CFG_SUBPATH)  # for setup.py
                set(CFG_PATH "")
            else()
                set(CFG_PATH "/${CFG}")
            endif()
            set_target_properties(openPMD.py PROPERTIES
                COMPILE_PDB_NAME_${CFG_UPPER} openpmd_api_cxx
                ARCHIVE_OUTPUT_DIRECTORY_${CFG_UPPER} ${openPMD_PYTHON_OUTPUT_DIRECTORY}${CFG_PATH}/openpmd_api
                LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} ${openPMD_PYTHON_OUTPUT_DIRECTORY}${CFG_PATH}/openpmd_api
                RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} ${openPMD_PYTHON_OUTPUT_DIRECTORY}${CFG_PATH}/openpmd_api
                PDB_OUTPUT_DIRECTORY_${CFG_UPPER} ${openPMD_PYTHON_OUTPUT_DIRECTORY}${CFG_PATH}/openpmd_api
                COMPILE_PDB_OUTPUT_DIRECTORY_${CFG_UPPER} ${openPMD_PYTHON_OUTPUT_DIRECTORY}${CFG_PATH}/openpmd_api
            )
        endforeach()
    endif()
    function(copy_aux_py)
        set(AUX_PY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/openpmd_api/)
        set(AUX_PY_DSR_DIR ${openPMD_PYTHON_OUTPUT_DIRECTORY}/openpmd_api/)
        foreach(src_name IN LISTS ARGN)
            configure_file(${AUX_PY_SRC_DIR}/${src_name} ${AUX_PY_DSR_DIR}/${src_name} COPYONLY)
        endforeach()
    endfunction()
    copy_aux_py(
        __init__.py DaskArray.py DaskDataFrame.py DataFrame.py
        ls/__init__.py   ls/__main__.py
        pipe/__init__.py pipe/__main__.py
    )

    # Standalone install logic. In the main build, the install rules for the
    # Python bindings live in the top-level CMakeLists.txt.
    if(openPMD_python_standalone AND openPMD_INSTALL)
        # RPATHs for the Python extension: it links against the (possibly
        # shared) C++ core library and needs to find it at runtime.
        if(openPMD_INSTALL_RPATH)
            if(NOT DEFINED CMAKE_INSTALL_RPATH)
                if(APPLE)
                    set_target_properties(openPMD.py PROPERTIES
                        INSTALL_RPATH "@loader_path")
                elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
                    set_target_properties(openPMD.py PROPERTIES
                        INSTALL_RPATH "$ORIGIN")
                endif()
            endif()
            if(NOT DEFINED CMAKE_INSTALL_RPATH_USE_LINK_PATH)
                set_target_properties(openPMD.py PROPERTIES
                    INSTALL_RPATH_USE_LINK_PATH ON)
            endif()
        endif()

        install(
            DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/openpmd_api
            DESTINATION ${openPMD_INSTALL_PYTHONDIR}
            PATTERN "*pyc" EXCLUDE
            PATTERN "__pycache__" EXCLUDE
        )
        install(TARGETS openPMD.py
            DESTINATION ${openPMD_INSTALL_PYTHONDIR}/openpmd_api
        )
        if(openPMD_BUILD_CLI_TOOLS)
            install(
                PROGRAMS ${openPMD_ROOT_SOURCE_DIR}/src/cli/pipe.py
                DESTINATION ${openPMD_INSTALL_BINDIR}
                RENAME openpmd-pipe
            )
        endif()
    endif()
endif()
