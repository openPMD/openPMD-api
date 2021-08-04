# Preamble ####################################################################
#
cmake_minimum_required(VERSION 3.15.0)

project(openPMD VERSION 0.14.1) # LANGUAGES CXX

# the openPMD "markup"/"schema" standard version
set(openPMD_STANDARD_VERSION 1.1.0)

list(APPEND CMAKE_MODULE_PATH "${openPMD_SOURCE_DIR}/share/openPMD/cmake")


# CMake policies ##############################################################
#
# Search in <PackageName>_ROOT:
#   https://cmake.org/cmake/help/v3.12/policy/CMP0074.html
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()


# Project structure ###########################################################
#
# temporary build directories
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        CACHE PATH "Build directory for archives")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        CACHE PATH "Build directory for libraries")
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        CACHE PATH "Build directory for binaries")
endif()
# install directories
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/openPMD"
        CACHE PATH "CMake config package location for installed targets")
    if(WIN32)
        set(CMAKE_INSTALL_LIBDIR Lib
            CACHE PATH "Object code libraries")
        set_property(CACHE CMAKE_INSTALL_CMAKEDIR PROPERTY VALUE "cmake")
    endif()
endif()


# Options and Variants ########################################################
#
function(openpmd_option name description default)
    set(openPMD_USE_${name} ${default} CACHE STRING "${description}")
    set_property(CACHE openPMD_USE_${name} PROPERTY
        STRINGS "ON;TRUE;AUTO;OFF;FALSE"
    )
    # list of all possible options
    set(openPMD_CONFIG_OPTIONS ${openPMD_CONFIG_OPTIONS} ${name} PARENT_SCOPE)
endfunction()

openpmd_option(MPI            "Parallel, Multi-Node I/O for clusters"     AUTO)
openpmd_option(HDF5           "HDF5 backend (.h5 files)"                  AUTO)
openpmd_option(ADIOS1         "ADIOS1 backend (.bp files)"                AUTO)
openpmd_option(ADIOS2         "ADIOS2 backend (.bp files)"                AUTO)
openpmd_option(PYTHON         "Enable Python bindings"                    AUTO)

option(openPMD_INSTALL               "Add installation targets"             ON)
option(openPMD_HAVE_PKGCONFIG        "Generate a .pc file for pkg-config"   ON)
option(openPMD_USE_INTERNAL_VARIANT  "Use internally shipped MPark.Variant" ON)
option(openPMD_USE_INTERNAL_CATCH    "Use internally shipped Catch2"        ON)
option(openPMD_USE_INTERNAL_PYBIND11 "Use internally shipped pybind11"      ON)
option(openPMD_USE_INTERNAL_JSON     "Use internally shipped nlohmann-json" ON)

option(openPMD_USE_INVASIVE_TESTS "Enable unit tests that modify source code" OFF)
option(openPMD_USE_VERIFY "Enable internal VERIFY (assert) macro independent of build type" ON)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;MinSizeRel;RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the build type, e.g. Release or Debug." FORCE)
endif()

include(CMakeDependentOption)

# change CMake default (static libs):
# build shared libs if supported by target platform
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
cmake_dependent_option(BUILD_SHARED_LIBS
    "Build shared libraries (so/dylib/dll)."
    ${SHARED_LIBS_SUPPORTED}
    "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(BUILD_SHARED_LIBS)
if(DEFINED BUILD_SHARED_LIBS)
    set(openPMD_BUILD_SHARED_LIBS_DEFAULT ${BUILD_SHARED_LIBS})
else()
    set(openPMD_BUILD_SHARED_LIBS_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
option(openPMD_BUILD_SHARED_LIBS "Build the openPMD tests"
    ${openPMD_BUILD_SHARED_LIBS_DEFAULT})
if(openPMD_BUILD_SHARED_LIBS AND NOT SHARED_LIBS_SUPPORTED)
    message(FATAL_ERROR "openPMD_BUILD_SHARED_LIBS requested but not supported by platform")
endif()

# Testing logic with possibility to overwrite on a project basis in superbuilds
include(CTest)
mark_as_advanced(BUILD_TESTING) # automatically defined, default: ON
if(DEFINED BUILD_TESTING)
    set(openPMD_BUILD_TESTING_DEFAULT ${BUILD_TESTING})
else()
    set(openPMD_BUILD_TESTING_DEFAULT ON)
endif()
option(openPMD_BUILD_TESTING "Build the openPMD tests"
    ${openPMD_BUILD_TESTING_DEFAULT})

# deprecated: backwards compatibility to <= 0.13.*
if(NOT DEFINED BUILD_CLI_TOOLS)
    set(BUILD_CLI_TOOLS ON)
endif()
if(NOT DEFINED BUILD_EXAMPLES)
    set(BUILD_EXAMPLES ON)
endif()
option(openPMD_BUILD_CLI_TOOLS "Build the command line tools" ${BUILD_CLI_TOOLS})
option(openPMD_BUILD_EXAMPLES  "Build the examples" ${BUILD_EXAMPLES})


# Dependencies ################################################################
#
# external library: MPI (optional)
# Implementation quirks for BullMPI, Clang+MPI and Brew's MPICH
#   definitely w/o MPI::MPI_C:
#     brew's MPICH with C-flag work-arounds - errors AppleClang for CXX targets
#     https://github.com/Homebrew/homebrew-core/issues/80465
#     https://lists.mpich.org/pipermail/discuss/2020-January/005863.html
#   sometimes needed MPI::MPI_C in the past:
#     Clang+MPI: Potentially needed MPI::MPI_C targets in the past
#                (exact MPI flavor & Clang version lost)
#     BullMPI: PUBLIC dependency to MPI::MPI_CXX is missing in MPI::MPI_C target
set(openPMD_MPI_LINK_C_DEFAULT OFF)
option(openPMD_MPI_LINK_C  "Also link the MPI C targets" ${openPMD_MPI_LINK_C_DEFAULT})
mark_as_advanced(openPMD_MPI_LINK_C)
set(openPMD_MPI_NEED_COMPONENTS CXX)
set(openPMD_MPI_TARGETS MPI::MPI_CXX)
if(openPMD_MPI_LINK_C)
    set(openPMD_MPI_NEED_COMPONENTS C ${openPMD_MPI_NEED_COMPONENTS})
    set(openPMD_MPI_TARGETS MPI::MPI_C ${openPMD_MPI_TARGETS})
endif()

if(openPMD_USE_MPI STREQUAL AUTO)
    find_package(MPI COMPONENTS ${openPMD_MPI_NEED_COMPONENTS})
    if(MPI_FOUND)
        set(openPMD_HAVE_MPI TRUE)
    else()
        set(openPMD_HAVE_MPI FALSE)
    endif()
elseif(openPMD_USE_MPI)
    find_package(MPI REQUIRED COMPONENTS ${openPMD_MPI_NEED_COMPONENTS})
    set(openPMD_HAVE_MPI TRUE)
else()
    set(openPMD_HAVE_MPI FALSE)
endif()


# external library: nlohmann-json (required)
if(openPMD_USE_INTERNAL_JSON)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")  # only used PRIVATE
    add_subdirectory("${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/json")
    message(STATUS "nlohmann-json: Using INTERNAL version '3.9.1'")
else()
    find_package(nlohmann_json 3.9.1 CONFIG REQUIRED)
    message(STATUS "nlohmann-json: Found version '${nlohmann_json_VERSION}'")
endif()
add_library(openPMD::thirdparty::nlohmann_json INTERFACE IMPORTED)
target_link_libraries(openPMD::thirdparty::nlohmann_json
    INTERFACE nlohmann_json::nlohmann_json)


# external library: HDF5 (optional)
#   note: in the new hdf5-cmake.config files, major releases like
#         1.8, 1.10 and 1.12 are not marked compatible versions
#   We could use CMake 3.19.0+ version ranges, but:
#     - this issues a Wdev warning with FindHDF5.cmake
#     - does not work at least with HDF5 1.10:
#       Could not find a configuration file for package "HDF5" that is compatible
#         with requested version range "1.8.13...1.12".
#         The following configuration files were considered but not accepted:
#         ../share/cmake/hdf5/hdf5-config.cmake, version: 1.10.7
#     - thus, we do our own HDF5_VERSION check...
if(openPMD_USE_HDF5 STREQUAL AUTO)
    set(HDF5_PREFER_PARALLEL ${openPMD_HAVE_MPI})
    find_package(HDF5 COMPONENTS C)
    if(HDF5_FOUND)
        set(openPMD_HAVE_HDF5 TRUE)
    else()
        set(openPMD_HAVE_HDF5 FALSE)
    endif()
elseif(openPMD_USE_HDF5)
    set(HDF5_PREFER_PARALLEL ${openPMD_HAVE_MPI})
    find_package(HDF5 REQUIRED COMPONENTS C)
    set(openPMD_HAVE_HDF5 TRUE)
else()
    set(openPMD_HAVE_HDF5 FALSE)
endif()

# HDF5 checks
string(CONCAT openPMD_HDF5_STATUS "")
# version: lower limit
if(openPMD_HAVE_HDF5 AND HDF5_VERSION VERSION_LESS 1.8.13)
    string(CONCAT openPMD_HDF5_STATUS
        "Found HDF5 version ${HDF5_VERSION} is too old. At least "
        "version 1.8.13 is required.\n")
endif()
# we imply support for parallel I/O if MPI variant is ON
if(openPMD_HAVE_MPI AND openPMD_HAVE_HDF5
    AND NOT HDF5_IS_PARALLEL      # FindHDF5.cmake
    AND NOT HDF5_ENABLE_PARALLEL  # hdf5-config.cmake
)
    string(CONCAT openPMD_HDF5_STATUS
            "Found MPI but only serial version of HDF5. Either set "
            "openPMD_USE_MPI=OFF to disable MPI or set openPMD_USE_HDF5=OFF "
            "to disable HDF5 or provide a parallel install of HDF5.\n")
endif()
# HDF5 includes mpi.h in the public header H5public.h if parallel
if(openPMD_HAVE_HDF5 AND
   (HDF5_IS_PARALLEL OR HDF5_ENABLE_PARALLEL)
   AND NOT openPMD_HAVE_MPI)
    string(CONCAT openPMD_HDF5_STATUS
        "Found only parallel version of HDF5 but no MPI. Either set "
        "openPMD_USE_MPI=ON to force using MPI or set openPMD_USE_HDF5=OFF "
        "to disable HDF5 or provide a serial install of HDF5.\n")
endif()

if(openPMD_HDF5_STATUS)
    string(CONCAT openPMD_HDF5_STATUS
        ${openPMD_HDF5_STATUS}
        "If you manually installed a version of HDF5 in "
        "a non-default path, add its installation prefix to the "
        "environment variable CMAKE_PREFIX_PATH to find it: "
        "https://cmake.org/cmake/help/latest/envvar/CMAKE_PREFIX_PATH.html")
    if(openPMD_USE_HDF5 STREQUAL AUTO)
        message(WARNING "${openPMD_HDF5_STATUS}")
        set(openPMD_HAVE_HDF5 FALSE)
    elseif(openPMD_USE_HDF5)
        message(FATAL_ERROR "${openPMD_HDF5_STATUS}")
    endif()
endif()

#   always search for a sequential lib first, so we can mock MPI
find_package(ADIOS 1.13.1 COMPONENTS sequential QUIET)
set(ADIOS_DEFINITIONS_SEQUENTIAL ${ADIOS_DEFINITIONS})
set(ADIOS_LIBRARIES_SEQUENTIAL ${ADIOS_LIBRARIES})
set(ADIOS_INCLUDE_DIRS_SEQUENTIAL ${ADIOS_INCLUDE_DIRS})
unset(ADIOS_FOUND CACHE)
unset(ADIOS_VERSION CACHE)

#   regular logic
set(ADIOS1_PREFER_COMPONENTS)
if(NOT openPMD_HAVE_MPI)
    set(ADIOS1_PREFER_COMPONENTS sequential)
endif()
if(openPMD_USE_ADIOS1 STREQUAL AUTO)
    find_package(ADIOS 1.13.1 COMPONENTS ${ADIOS1_PREFER_COMPONENTS})
    if(ADIOS_FOUND)
        set(openPMD_HAVE_ADIOS1 TRUE)
    else()
        set(openPMD_HAVE_ADIOS1 FALSE)
    endif()
elseif(openPMD_USE_ADIOS1)
    find_package(ADIOS 1.13.1 REQUIRED COMPONENTS ${ADIOS1_PREFER_COMPONENTS})
    set(openPMD_HAVE_ADIOS1 TRUE)
else()
    set(openPMD_HAVE_ADIOS1 FALSE)
endif()

if(openPMD_HAVE_MPI AND openPMD_HAVE_ADIOS1 AND ADIOS_HAVE_SEQUENTIAL)
    string(CONCAT openPMD_ADIOS1_STATUS
        "Found MPI but requested ADIOS1 is serial. "
        "Set openPMD_USE_MPI=OFF to disable MPI.\n"
        "If you manually installed a parallel version of ADIOS1 in "
        "a non-default path, add its installation prefix to the "
        "environment variable CMAKE_PREFIX_PATH to find it: "
        "https://cmake.org/cmake/help/latest/envvar/CMAKE_PREFIX_PATH.html")
    if(openPMD_USE_ADIOS1 STREQUAL AUTO)
        message(WARNING "${openPMD_ADIOS1_STATUS}")
        set(openPMD_HAVE_ADIOS1 FALSE)
    elseif(openPMD_USE_ADIOS1)
        message(FATAL_ERROR "${openPMD_ADIOS1_STATUS}")
    endif()
endif()
if(NOT openPMD_HAVE_MPI AND openPMD_HAVE_ADIOS1 AND NOT ADIOS_HAVE_SEQUENTIAL)
    string(CONCAT openPMD_ADIOS1_STATUS
        "Did not find MPI but requested ADIOS1 is parallel. "
        "Set openPMD_USE_ADIOS1=OFF to disable ADIOS1.\n"
        "If you manually installed a serial version of ADIOS1 in "
        "a non-default path, add its installation prefix to the "
        "environment variable CMAKE_PREFIX_PATH to find it: "
        "https://cmake.org/cmake/help/latest/envvar/CMAKE_PREFIX_PATH.html")
    if(openPMD_USE_ADIOS1 STREQUAL AUTO)
        message(WARNING "${openPMD_ADIOS1_STATUS}")
        set(openPMD_HAVE_ADIOS1 FALSE)
    elseif(openPMD_USE_ADIOS1)
        message(FATAL_ERROR "${openPMD_ADIOS1_STATUS}")
    endif()
endif()

# external library: ADIOS2 (optional)
if(openPMD_USE_ADIOS2 STREQUAL AUTO)
    find_package(ADIOS2 2.7.0 CONFIG)
    if(ADIOS2_FOUND)
        set(openPMD_HAVE_ADIOS2 TRUE)
    else()
        set(openPMD_HAVE_ADIOS2 FALSE)
    endif()
elseif(openPMD_USE_ADIOS2)
    find_package(ADIOS2 2.7.0 REQUIRED CONFIG)
    set(openPMD_HAVE_ADIOS2 TRUE)
else()
    set(openPMD_HAVE_ADIOS2 FALSE)
endif()

# TODO: Check if ADIOS2 is parallel when openPMD_HAVE_MPI is ON

# external library: pybind11 (optional)
set(_PY_DEV_MODULE Development.Module)
if(CMAKE_VERSION VERSION_LESS 3.18.0)
    # over-specification needed for CMake<3.18
    #   https://pybind11.readthedocs.io/en/latest/compiling.html#findpython-mode
    #   https://cmake.org/cmake/help/v3.18/module/FindPython.html
    set(_PY_DEV_MODULE Development)
endif()
if(openPMD_USE_PYTHON STREQUAL AUTO)
    find_package(Python 3.6.0 COMPONENTS Interpreter ${_PY_DEV_MODULE})
    if(Python_FOUND)
        if(openPMD_USE_INTERNAL_PYBIND11)
            add_subdirectory("${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/pybind11")
            set(openPMD_HAVE_PYTHON TRUE)
            message(STATUS "pybind11: Using INTERNAL version 2.6.2")
        else()
            find_package(pybind11 2.6.2 CONFIG)
            if(pybind11_FOUND)
                set(openPMD_HAVE_PYTHON TRUE)
                message(STATUS "pybind11: Found version '${pybind11_VERSION}'")
            else()
                set(openPMD_HAVE_PYTHON FALSE)
            endif()
        endif()
    else()
        set(openPMD_HAVE_PYTHON FALSE)
    endif()
elseif(openPMD_USE_PYTHON)
    find_package(Python COMPONENTS Interpreter ${_PY_DEV_MODULE} REQUIRED)
    if(openPMD_USE_INTERNAL_PYBIND11)
        add_subdirectory("${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/pybind11")
        set(openPMD_HAVE_PYTHON TRUE)
        message(STATUS "pybind11: Using INTERNAL version 2.6.2")
    else()
        find_package(pybind11 2.6.2 REQUIRED CONFIG)
        set(openPMD_HAVE_PYTHON TRUE)
        message(STATUS "pybind11: Found version '${pybind11_VERSION}'")
    endif()
else()
    set(openPMD_HAVE_PYTHON FALSE)
endif()


# Targets #####################################################################
#
set(CORE_SOURCE
        src/config.cpp
        src/ChunkInfo.cpp
        src/Dataset.cpp
        src/Datatype.cpp
        src/Format.cpp
        src/Iteration.cpp
        src/IterationEncoding.cpp
        src/Mesh.cpp
        src/ParticlePatches.cpp
        src/ParticleSpecies.cpp
        src/ReadIterations.cpp
        src/Record.cpp
        src/RecordComponent.cpp
        src/Series.cpp
        src/version.cpp
        src/WriteIterations.cpp
        src/auxiliary/Date.cpp
        src/auxiliary/Filesystem.cpp
        src/auxiliary/JSON.cpp
        src/backend/Attributable.cpp
        src/backend/BaseRecordComponent.cpp
        src/backend/Container.cpp
        src/backend/MeshRecordComponent.cpp
        src/backend/PatchRecord.cpp
        src/backend/PatchRecordComponent.cpp
        src/backend/Writable.cpp
        src/benchmark/mpi/OneDimensionalBlockSlicer.cpp
        src/helper/list_series.cpp)
set(IO_SOURCE
        src/IO/AbstractIOHandlerHelper.cpp
        src/IO/DummyIOHandler.cpp
        src/IO/IOTask.cpp
        src/IO/HDF5/HDF5IOHandler.cpp
        src/IO/HDF5/ParallelHDF5IOHandler.cpp
        src/IO/HDF5/HDF5Auxiliary.cpp
        src/IO/JSON/JSONIOHandler.cpp
        src/IO/JSON/JSONIOHandlerImpl.cpp
        src/IO/JSON/JSONFilePosition.cpp
        src/IO/ADIOS/ADIOS2IOHandler.cpp
        src/IO/ADIOS/ADIOS2Auxiliary.cpp
        src/IO/ADIOS/ADIOS2PreloadAttributes.cpp
        src/IO/InvalidatableFile.cpp)
set(IO_ADIOS1_SEQUENTIAL_SOURCE
        src/auxiliary/Filesystem.cpp
        src/ChunkInfo.cpp
        src/IO/ADIOS/ADIOS1IOHandler.cpp)
set(IO_ADIOS1_SOURCE
        src/auxiliary/Filesystem.cpp
        src/ChunkInfo.cpp
        src/IO/ADIOS/ParallelADIOS1IOHandler.cpp)

# library
if(openPMD_BUILD_SHARED_LIBS)
    set(_openpmd_lib_type SHARED)
else()
    set(_openpmd_lib_type STATIC)
endif()
add_library(openPMD ${_openpmd_lib_type} ${CORE_SOURCE} ${IO_SOURCE})
add_library(openPMD::openPMD ALIAS openPMD)

# properties
target_compile_features(openPMD
    PUBLIC cxx_std_14
)
set_target_properties(openPMD PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
set(_cxx_msvc   "$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>")
set(_msvc_1914  "$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19.14>")
set(_msvc_options)
list(APPEND _msvc_options
    $<${_cxx_msvc}:/bigobj>
    $<${_cxx_msvc}:$<${_msvc_1914}:/Zc:__cplusplus>>
)
target_compile_options(openPMD PUBLIC ${_msvc_options})

# own headers
target_include_directories(openPMD PUBLIC
    $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${openPMD_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++11 std::variant (C++17 stdlib preview)
# TODO not needed with C++17 compiler
add_library(openPMD::thirdparty::mpark_variant INTERFACE IMPORTED)
set(openPMD_PC_EXTRA_INCLUDE "")
if(openPMD_USE_INTERNAL_VARIANT)
    target_include_directories(openPMD::thirdparty::mpark_variant SYSTEM INTERFACE
        $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/variant/include>
    )
    message(STATUS "MPark.Variant: Using INTERNAL version '1.4.0'")
else()
    find_package(mpark_variant 1.3.0 REQUIRED) # TODO: we want 1.4.1+
    target_link_libraries(openPMD::thirdparty::mpark_variant
        INTERFACE mpark_variant)
    get_target_property(EXTERNAL_MPARK_INCLUDE mpark_variant INTERFACE_INCLUDE_DIRECTORIES)
    if(openPMD_HAVE_PKGCONFIG AND EXTERNAL_MPARK_INCLUDE)
        set(openPMD_PC_EXTRA_INCLUDE "-I${EXTERNAL_MPARK_INCLUDE}")
    endif()
    message(STATUS "MPark.Variant: Found version '${mpark_variant_VERSION}'")
endif()
target_link_libraries(openPMD PUBLIC openPMD::thirdparty::mpark_variant)

# Catch2 for unit tests
if(openPMD_BUILD_TESTING)
    add_library(openPMD::thirdparty::Catch2 INTERFACE IMPORTED)
    if(openPMD_USE_INTERNAL_CATCH)
        target_include_directories(openPMD::thirdparty::Catch2 SYSTEM INTERFACE
            $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/catch2/include>
        )
        message(STATUS "Catch2: Using INTERNAL version '2.13.4'")
    else()
        find_package(Catch2 2.13.4 REQUIRED CONFIG)
        target_link_libraries(openPMD::thirdparty::Catch2
            INTERFACE Catch2::Catch2)
        message(STATUS "Catch2: Found version '${Catch2_VERSION}'")
    endif()
endif()

if(openPMD_HAVE_MPI)
    target_link_libraries(openPMD PUBLIC ${openPMD_MPI_TARGETS})
endif()

# JSON Backend and User-Facing Runtime Options
#target_link_libraries(openPMD PRIVATE openPMD::thirdparty::nlohmann_json)
target_include_directories(openPMD SYSTEM PRIVATE
    $<TARGET_PROPERTY:openPMD::thirdparty::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>)

# HDF5 Backend
if(openPMD_HAVE_HDF5)
    target_link_libraries(openPMD PRIVATE ${HDF5_LIBRARIES})
    target_include_directories(openPMD SYSTEM PRIVATE ${HDF5_INCLUDE_DIRS})
    target_compile_definitions(openPMD PRIVATE ${HDF5_DEFINITIONS})
endif()

# ADIOS1 Backend
if(openPMD_HAVE_ADIOS1)
    add_library(openPMD.ADIOS1.Serial SHARED ${IO_ADIOS1_SEQUENTIAL_SOURCE})
    add_library(openPMD.ADIOS1.Parallel SHARED ${IO_ADIOS1_SOURCE})
    target_compile_features(openPMD.ADIOS1.Serial
        PUBLIC cxx_std_14
    )
    target_compile_features(openPMD.ADIOS1.Parallel
        PUBLIC cxx_std_14
    )
    target_compile_options(openPMD.ADIOS1.Serial PUBLIC ${_msvc_options})
    target_compile_options(openPMD.ADIOS1.Parallel PUBLIC ${_msvc_options})
    target_link_libraries(openPMD.ADIOS1.Serial PUBLIC openPMD::thirdparty::mpark_variant)
    target_link_libraries(openPMD.ADIOS1.Parallel PUBLIC openPMD::thirdparty::mpark_variant)

    target_include_directories(openPMD.ADIOS1.Serial PRIVATE
        ${openPMD_SOURCE_DIR}/include ${openPMD_BINARY_DIR}/include)
    target_include_directories(openPMD.ADIOS1.Parallel PRIVATE
        ${openPMD_SOURCE_DIR}/include ${openPMD_BINARY_DIR}/include)

    if(openPMD_HAVE_MPI)
        target_link_libraries(openPMD.ADIOS1.Parallel PUBLIC ${openPMD_MPI_TARGETS})
        target_compile_definitions(openPMD.ADIOS1.Parallel PRIVATE openPMD_HAVE_MPI=1)
    else()
        target_compile_definitions(openPMD.ADIOS1.Parallel PRIVATE openPMD_HAVE_MPI=0)
    endif()

    set_target_properties(openPMD.ADIOS1.Serial PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
        set_target_properties(openPMD.ADIOS1.Serial PROPERTIES
            LINK_FLAGS "-Wl,--exclude-libs,ALL")
    elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
        set_target_properties(openPMD.ADIOS1.Serial PROPERTIES
            XCODE_ATTRIBUTE_STRIP_STYLE "non-global"
            XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING "YES"
            XCODE_ATTRIBUTE_SEPARATE_STRIP "YES"
        )
    endif()
    foreach(adlib ${ADIOS_LIBRARIES_SEQUENTIAL})
        target_link_libraries(openPMD.ADIOS1.Serial PRIVATE ${adlib})
    endforeach()
    target_include_directories(openPMD.ADIOS1.Serial SYSTEM PRIVATE ${ADIOS_INCLUDE_DIRS_SEQUENTIAL})
    target_compile_definitions(openPMD.ADIOS1.Serial PRIVATE "${ADIOS_DEFINITIONS_SEQUENTIAL}")
    target_compile_definitions(openPMD.ADIOS1.Serial PRIVATE openPMD_HAVE_ADIOS1=1)
    target_compile_definitions(openPMD.ADIOS1.Serial PRIVATE openPMD_HAVE_MPI=0)
    target_compile_definitions(openPMD.ADIOS1.Serial PRIVATE _NOMPI=1)

    if(openPMD_HAVE_MPI)
        set_target_properties(openPMD.ADIOS1.Parallel PROPERTIES
            CXX_EXTENSIONS OFF
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN 1
        )
        if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
            set_target_properties(openPMD.ADIOS1.Parallel PROPERTIES
                LINK_FLAGS "-Wl,--exclude-libs,ALL")
        elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
            set_target_properties(openPMD.ADIOS1.Parallel PROPERTIES
                XCODE_ATTRIBUTE_STRIP_STYLE "non-global"
                XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING "YES"
                XCODE_ATTRIBUTE_SEPARATE_STRIP "YES"
            )
        endif()
        foreach(adlib ${ADIOS_LIBRARIES})
            target_link_libraries(openPMD.ADIOS1.Parallel PRIVATE ${adlib})
        endforeach()

        target_include_directories(openPMD.ADIOS1.Parallel SYSTEM PRIVATE ${ADIOS_INCLUDE_DIRS})
        target_compile_definitions(openPMD.ADIOS1.Parallel PRIVATE "${ADIOS_DEFINITIONS}")
        target_compile_definitions(openPMD.ADIOS1.Parallel PRIVATE openPMD_HAVE_ADIOS1=1)
    endif()

    # Runtime parameter and API status checks ("asserts")
    if(openPMD_USE_VERIFY)
        target_compile_definitions(openPMD.ADIOS1.Serial PRIVATE openPMD_USE_VERIFY=1)
        target_compile_definitions(openPMD.ADIOS1.Parallel PRIVATE openPMD_USE_VERIFY=1)
    else()
        target_compile_definitions(openPMD.ADIOS1.Serial PRIVATE openPMD_USE_VERIFY=0)
        target_compile_definitions(openPMD.ADIOS1.Parallel PRIVATE openPMD_USE_VERIFY=0)
    endif()

    target_link_libraries(openPMD PUBLIC openPMD.ADIOS1.Serial)
    target_link_libraries(openPMD PUBLIC openPMD.ADIOS1.Parallel)
endif()

# ADIOS2 Backend
if(openPMD_HAVE_ADIOS2)
    if(openPMD_HAVE_MPI)
        target_link_libraries(openPMD PUBLIC adios2::cxx11_mpi)
    else()
        target_link_libraries(openPMD PUBLIC adios2::cxx11)
    endif()
endif()

# Runtime parameter and API status checks ("asserts")
if(openPMD_USE_VERIFY)
    target_compile_definitions(openPMD PRIVATE openPMD_USE_VERIFY=1)
else()
    target_compile_definitions(openPMD PRIVATE openPMD_USE_VERIFY=0)
endif()

# python bindings
if(openPMD_HAVE_PYTHON)
    add_library(openPMD.py MODULE
        src/binding/python/openPMD.cpp
        src/binding/python/Access.cpp
        src/binding/python/Attributable.cpp
        src/binding/python/BaseRecord.cpp
        src/binding/python/BaseRecordComponent.cpp
        src/binding/python/ChunkInfo.cpp
        src/binding/python/Container.cpp
        src/binding/python/Dataset.cpp
        src/binding/python/Datatype.cpp
        src/binding/python/Helper.cpp
        src/binding/python/Iteration.cpp
        src/binding/python/IterationEncoding.cpp
        src/binding/python/Mesh.cpp
        src/binding/python/ParticlePatches.cpp
        src/binding/python/ParticleSpecies.cpp
        src/binding/python/PatchRecord.cpp
        src/binding/python/PatchRecordComponent.cpp
        src/binding/python/Record.cpp
        src/binding/python/RecordComponent.cpp
        src/binding/python/MeshRecordComponent.cpp
        src/binding/python/Series.cpp
        src/binding/python/UnitDimension.cpp
    )
    target_link_libraries(openPMD.py PRIVATE openPMD)
    target_link_libraries(openPMD.py PRIVATE pybind11::module pybind11::windows_extras)

    # LTO/IPO: CMake target properties work well for 3.18+ and are buggy before
    set(_USE_PY_LTO ON)  # default shall be ON
    if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)  # overwrite default if defined
        if(NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(_USE_PY_LTO OFF)
        endif()
    endif()
    message(STATUS "Python LTO/IPO: ${_USE_PY_LTO}")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
        set_target_properties(openPMD.py PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION ${_USE_PY_LTO})
    else()
        if(_USE_PY_LTO)
            target_link_libraries(openPMD.py PRIVATE pybind11::lto)
        endif()
    endif()
    unset(_USE_PY_LTO)

    pybind11_extension(openPMD.py)
    pybind11_strip(openPMD.py)

    set_target_properties(openPMD.py PROPERTIES CXX_VISIBILITY_PRESET "hidden"
                                                CUDA_VISIBILITY_PRESET "hidden")

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
        set(CMAKE_INSTALL_PYTHONDIR_DEFAULT
            "${CMAKE_INSTALL_LIBDIR}/site-packages"
        )
    else()
        set(CMAKE_INSTALL_PYTHONDIR_DEFAULT
            "${CMAKE_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages"
        )
    endif()
    set(CMAKE_INSTALL_PYTHONDIR "${CMAKE_INSTALL_PYTHONDIR_DEFAULT}"
        CACHE STRING "Location for installed python package"
    )
    set(CMAKE_PYTHON_OUTPUT_DIRECTORY
        "${openPMD_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}"
        CACHE PATH "Build directory for python modules"
    )
    set_target_properties(openPMD.py PROPERTIES
        ARCHIVE_OUTPUT_NAME openpmd_api_cxx
        LIBRARY_OUTPUT_NAME openpmd_api_cxx
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
        COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/openpmd_api
    )
    function(copy_aux_py)
        set(AUX_PY_SRC_DIR ${openPMD_SOURCE_DIR}/src/binding/python/openpmd_api/)
        set(AUX_PY_DSR_DIR ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/openpmd_api/)
        foreach(src_name IN LISTS ARGN)
            configure_file(${AUX_PY_SRC_DIR}/${src_name} ${AUX_PY_DSR_DIR}/${src_name} COPYONLY)
        endforeach()
    endfunction()
    copy_aux_py(
        __init__.py DaskArray.py DaskDataFrame.py DataFrame.py
        ls/__init__.py   ls/__main__.py
        pipe/__init__.py pipe/__main__.py
    )
endif()

# tests
set(openPMD_TEST_NAMES
    Core
    Auxiliary
    SerialIO
    ParallelIO
)
# command line tools
set(openPMD_CLI_TOOL_NAMES
    ls
)
set(openPMD_PYTHON_CLI_TOOL_NAMES
    pipe
)
set(openPMD_PYTHON_CLI_MODULE_NAMES ${openPMD_CLI_TOOL_NAMES})
# examples
set(openPMD_EXAMPLE_NAMES
    1_structure
    2_read_serial
    2a_read_thetaMode_serial
    3_write_serial
    3a_write_thetaMode_serial
    3b_write_resizable_particles
    4_read_parallel
    5_write_parallel
    6_dump_filebased_series
    7_extended_write_serial
    8_benchmark_parallel
    8a_benchmark_write_parallel
    8b_benchmark_read_parallel
    10_streaming_write
    10_streaming_read
    12_span_write
)
set(openPMD_PYTHON_EXAMPLE_NAMES
    2_read_serial
    2a_read_thetaMode_serial
    3_write_serial
    3a_write_thetaMode_serial
    3b_write_resizable_particles
    4_read_parallel
    5_write_parallel
    7_extended_write_serial
    9_particle_write_serial
    11_particle_dataframe
    12_span_write
)

if(openPMD_USE_INVASIVE_TESTS)
    if(WIN32)
        message(WARNING "Invasive tests that redefine class signatures are "
                        "known to fail on Windows!")
    endif()
endif()

if(openPMD_BUILD_TESTING)
    # compile Catch2 implementation part separately
    add_library(CatchRunner ${_openpmd_lib_type}
        test/CatchRunner.cpp)  # Always MPI_Init with Serial Fallback
    add_library(CatchMain   ${_openpmd_lib_type}
        test/CatchMain.cpp)    # Serial only
    target_compile_features(CatchRunner PUBLIC cxx_std_14)
    target_compile_features(CatchMain   PUBLIC cxx_std_14)
    set_target_properties(CatchRunner CatchMain PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
    target_compile_options(CatchRunner PUBLIC ${_msvc_options})
    target_compile_options(CatchMain   PUBLIC ${_msvc_options})
    target_link_libraries(CatchRunner PUBLIC openPMD::thirdparty::Catch2)
    target_link_libraries(CatchMain   PUBLIC openPMD::thirdparty::Catch2)
    if(openPMD_HAVE_MPI)
        target_link_libraries(CatchRunner PUBLIC ${openPMD_MPI_TARGETS})
        target_compile_definitions(CatchRunner PUBLIC openPMD_HAVE_MPI=1)
    endif()

    foreach(testname ${openPMD_TEST_NAMES})
        add_executable(${testname}Tests test/${testname}Test.cpp)

        if(openPMD_USE_INVASIVE_TESTS)
            target_compile_definitions(${testname}Tests PRIVATE openPMD_USE_INVASIVE_TESTS=1)
        endif()
        target_link_libraries(${testname}Tests PRIVATE openPMD)
        if(${testname} MATCHES "Parallel.+$")
            target_link_libraries(${testname}Tests PRIVATE CatchRunner)
        else()
            target_link_libraries(${testname}Tests PRIVATE CatchMain)
        endif()
    endforeach()
endif()

if(openPMD_BUILD_CLI_TOOLS)
    foreach(toolname ${openPMD_CLI_TOOL_NAMES})
        add_executable(openpmd-${toolname} src/cli/${toolname}.cpp)
        target_link_libraries(openpmd-${toolname} PRIVATE openPMD)
    endforeach()
endif()

if(openPMD_BUILD_EXAMPLES)
    foreach(examplename ${openPMD_EXAMPLE_NAMES})
        if(${examplename} MATCHES ".+parallel$")
            if(openPMD_HAVE_MPI)
                add_executable(${examplename} examples/${examplename}.cpp)
                target_link_libraries(${examplename} PRIVATE openPMD)
            endif()
        else()
            add_executable(${examplename} examples/${examplename}.cpp)
            target_link_libraries(${examplename} PRIVATE openPMD)
        endif()
    endforeach()
endif()


# Warnings ####################################################################
#
# TODO: LEGACY! Use CMake TOOLCHAINS instead!
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # On Windows, Clang -Wall aliases -Weverything; default is /W3
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT WIN32)
        # list(APPEND CMAKE_CXX_FLAGS "-fsanitize=address") # address, memory, undefined
        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
        # set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")

        # note: might still need a
        #   export LD_PRELOAD=libclang_rt.asan.so
        # or on Debian 9 with Clang 6.0
        #   export LD_PRELOAD=/usr/lib/llvm-6.0/lib/clang/6.0.0/lib/linux/libclang_rt.asan-x86_64.so:
        #                     /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/linux/libclang_rt.ubsan_minimal-x86_64.so
        # at runtime when used with symbol-hidden code (e.g. pybind11 module)

        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Woverloaded-virtual -Wextra-semi -Wunreachable-code")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w3 -wd193,383,1572")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Woverloaded-virtual -Wunreachable-code")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # Warning C4503: "decorated name length exceeded, name was truncated"
        # Symbols longer than 4096 chars are truncated (and hashed instead)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4503")
        # Warning C4244: "conversion from 'X' to 'Y', possible loss of data"
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4244")
        # Yes, you should build against the same C++ runtime and with same
        # configuration (Debug/Release). MSVC does inconvenient choices for their
        # developers, so be it. (Our Windows-users use conda-forge builds, which
        # are consistent.)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4251")
    endif()
endif()


# Generate Files with Configuration Options ###################################
#
# TODO configure a version.hpp
configure_file(
    ${openPMD_SOURCE_DIR}/include/openPMD/config.hpp.in
    ${openPMD_BINARY_DIR}/include/openPMD/config.hpp
    @ONLY
)

configure_file(
    ${openPMD_SOURCE_DIR}/openPMDConfig.cmake.in
    ${openPMD_BINARY_DIR}/openPMDConfig.cmake
    @ONLY
)

# get absolute paths to linked libraries
function(openpmdreclibs tgtname outname)
    get_target_property(PC_PRIVATE_LIBS_TGT ${tgtname} INTERFACE_LINK_LIBRARIES)
    foreach(PC_LIB IN LISTS PC_PRIVATE_LIBS_TGT)
       if(TARGET ${PC_LIB})
           openpmdreclibs(${PC_LIB} ${outname})
       else()
           if(PC_LIB)
               string(APPEND ${outname} " ${PC_LIB}")
           endif()
       endif()
    endforeach()
    set(${outname} ${${outname}} PARENT_SCOPE)
endfunction()

if(openPMD_HAVE_PKGCONFIG)
    openpmdreclibs(openPMD openPMD_PC_PRIVATE_LIBS)
    if(openPMD_BUILD_SHARED_LIBS)
        set(openPMD_PC_STATIC false)
    else()
        set(openPMD_PC_STATIC true)
    endif()
    configure_file(
        ${openPMD_SOURCE_DIR}/openPMD.pc.in
        ${openPMD_BINARY_DIR}/openPMD.pc
        @ONLY
    )
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file("openPMDConfigVersion.cmake"
    VERSION ${openPMD_VERSION}
    COMPATIBILITY SameMajorVersion
)


# Installs ####################################################################
#
# headers, libraries and executables
if(openPMD_INSTALL)
    set(openPMD_INSTALL_TARGET_NAMES openPMD)

    if(openPMD_HAVE_ADIOS1)
        list(APPEND openPMD_INSTALL_TARGET_NAMES
            openPMD.ADIOS1.Serial openPMD.ADIOS1.Parallel)
    endif()

    if(openPMD_BUILD_CLI_TOOLS)
        foreach(toolname ${openPMD_CLI_TOOL_NAMES})
            list(APPEND openPMD_INSTALL_TARGET_NAMES openpmd-${toolname})
        endforeach()
    endif()

    install(TARGETS ${openPMD_INSTALL_TARGET_NAMES}
        EXPORT openPMDTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    if(openPMD_HAVE_PYTHON)
        install(
            DIRECTORY   ${openPMD_SOURCE_DIR}/src/binding/python/openpmd_api
            DESTINATION ${CMAKE_INSTALL_PYTHONDIR}
            PATTERN "*pyc" EXCLUDE
            PATTERN "__pycache__" EXCLUDE
        )
        install(TARGETS openPMD.py
            DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/openpmd_api
        )
        if(openPMD_BUILD_CLI_TOOLS)
            foreach(toolname ${openPMD_PYTHON_CLI_TOOL_NAMES})
                install(
                    FILES ${openPMD_SOURCE_DIR}/src/cli/${toolname}.py
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                    RENAME openpmd-${toolname}
                    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                )
            endforeach()
        endif()
    endif()
    install(DIRECTORY "${openPMD_SOURCE_DIR}/include/openPMD"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.tpp"
    )
    install(
        FILES ${openPMD_BINARY_DIR}/include/openPMD/config.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openPMD
    )
    # install third-party libraries
    # TODO not needed with C++17 compiler
    if(openPMD_USE_INTERNAL_VARIANT)
        install(DIRECTORY "${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/variant/include/mpark"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    # CMake package file for find_package(openPMD::openPMD) in depending projects
    install(EXPORT openPMDTargets
        FILE openPMDTargets.cmake
        NAMESPACE openPMD::
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
    )
    install(
        FILES
            ${openPMD_BINARY_DIR}/openPMDConfig.cmake
            ${openPMD_BINARY_DIR}/openPMDConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
    )
    install(
        FILES
            ${openPMD_SOURCE_DIR}/share/openPMD/cmake/FindADIOS.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
    )
    # pkg-config .pc file for depending legacy projects
    #   This is for projects that do not use a build file generator, e.g.
    #   because they compile manually on the command line or write their
    #   Makefiles by hand.
    if(openPMD_HAVE_PKGCONFIG)
        install(
            FILES       ${openPMD_BINARY_DIR}/openPMD.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
    endif()
endif()


# Tests #######################################################################
#
if(openPMD_BUILD_TESTING)
    enable_testing()

    # OpenMPI root guard: https://github.com/open-mpi/ompi/issues/4451
    if("$ENV{USER}" STREQUAL "root")
        # calling even --help as root will abort and warn on stderr
        execute_process(COMMAND ${MPIEXEC_EXECUTABLE} --help
            ERROR_VARIABLE MPIEXEC_HELP_TEXT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
            if(${MPIEXEC_HELP_TEXT} MATCHES "^.*allow-run-as-root.*$")
                set(MPI_ALLOW_ROOT --allow-run-as-root)
            endif()
    endif()
    set(MPI_TEST_EXE
        ${MPIEXEC_EXECUTABLE}
        ${MPI_ALLOW_ROOT}
        ${MPIEXEC_NUMPROC_FLAG} 2
    )

    # do we have openPMD-example-datasets?
    if(EXISTS "${openPMD_BINARY_DIR}/samples/git-sample/")
        set(EXAMPLE_DATA_FOUND ON)
        message(STATUS "Found openPMD-example-datasets: TRUE")
    else()
        message(STATUS "Note: Skipping example and tool runs (missing openPMD-example-datasets)")
        if(WIN32)
            message(STATUS "Note: run\n"
                           "    Powershell.exe -File ${openPMD_SOURCE_DIR}/share/openPMD/download_samples.ps1\n"
                           "to add example files to samples/git-sample/ directory!")
        else()
            message(STATUS "Note: run\n"
                           "    ${openPMD_SOURCE_DIR}/share/openPMD/download_samples.sh\n"
                           "to add example files to samples/git-sample/ directory!")
        endif()
    endif()

    if(openPMD_HAVE_PYTHON)
        # do we have mpi4py for MPI-parallel Python tests?
        if(openPMD_HAVE_MPI)
            execute_process(COMMAND ${Python_EXECUTABLE}
                -m mpi4py
                -c "import mpi4py.MPI"
                RESULT_VARIABLE MPI4PY_RETURN
                OUTPUT_QUIET ERROR_QUIET)

            if(MPI4PY_RETURN EQUAL 0)
                message(STATUS "Found mpi4py: TRUE")
            else()
                message(STATUS "Could NOT find mpi4py (will NOT run MPI-parallel Python examples)")
            endif()
        endif()
    endif()

    # C++ Unit tests
    foreach(testname ${openPMD_TEST_NAMES})
        if(${testname} MATCHES "^Parallel.*$")
            if(openPMD_HAVE_MPI)
                add_test(NAME MPI.${testname}
                    COMMAND ${MPI_TEST_EXE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testname}Tests
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                )
            endif()
        else()
            add_test(NAME Serial.${testname}
                COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testname}Tests
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            )
        endif()
    endforeach()

    # Python Unit tests
    if(openPMD_HAVE_PYTHON)
        if(openPMD_HAVE_HDF5)
            if(EXAMPLE_DATA_FOUND)
                add_test(NAME Unittest.py
                    COMMAND ${Python_EXECUTABLE}
                        ${openPMD_SOURCE_DIR}/test/python/unittest/Test.py -v
                    WORKING_DIRECTORY
                        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                )
                if(WIN32)
                    string(REGEX REPLACE "/" "\\\\" WIN_BUILD_BASEDIR ${openPMD_BINARY_DIR})
                    string(REGEX REPLACE "/" "\\\\" WIN_BUILD_BINDIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
                    string(REPLACE ";" "\\;" WIN_PATH "$ENV{PATH}")
                    string(REPLACE ";" "\\;" WIN_PYTHONPATH "$ENV{PYTHONPATH}")
                    set_property(TEST Unittest.py
                        PROPERTY ENVIRONMENT
                            "PATH=${WIN_BUILD_BINDIR}\\${CMAKE_BUILD_TYPE}\;${WIN_PATH}\n"
                            "PYTHONPATH=${WIN_BUILD_BASEDIR}\\${CMAKE_INSTALL_PYTHONDIR}\\${CMAKE_BUILD_TYPE}\;${WIN_PYTHONPATH}"
                    )
                else()
                    set_tests_properties(Unittest.py
                        PROPERTIES ENVIRONMENT
                            "PYTHONPATH=${openPMD_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
                    )
                endif()
            endif()
        endif()
    endif()

    # Examples
    if(openPMD_BUILD_EXAMPLES)
        # C++ Examples
        # Current examples all use HDF5, elaborate if other backends are used
        if(openPMD_HAVE_HDF5)
            if(EXAMPLE_DATA_FOUND)
                foreach(examplename ${openPMD_EXAMPLE_NAMES})
                    if(${examplename} MATCHES "^10.*$")
                        # streaming examples are done separately
                    elseif(${examplename} MATCHES "^.*_parallel$")
                        if(openPMD_HAVE_MPI)
                            add_test(NAME MPI.${examplename}
                                    COMMAND ${MPI_TEST_EXE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${examplename}
                                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                                    )
                        endif()
                    else()
                        add_test(NAME Serial.${examplename}
                                COMMAND ${examplename}
                                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                                )
                    endif()
                endforeach()
            endif()
        endif()
        if(openPMD_HAVE_ADIOS2)
            add_test(NAME Asynchronous.10_streaming
                     COMMAND sh -c "$<TARGET_FILE:10_streaming_write> & sleep 1; $<TARGET_FILE:10_streaming_read>"
                     WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif()
    endif()

    # Command Line Tools
    if(openPMD_BUILD_CLI_TOOLS)
        # all tools must provide a "--help"
        foreach(toolname ${openPMD_CLI_TOOL_NAMES})
            add_test(NAME CLI.help.${toolname}
                COMMAND openpmd-${toolname} --help
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            )
        endforeach()
        if(openPMD_HAVE_HDF5 AND EXAMPLE_DATA_FOUND)
            add_test(NAME CLI.ls
                COMMAND openpmd-ls ../samples/git-sample/data%08T.h5
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            )
        endif()
    endif()

    function(test_set_pythonpath test_name)
        if(WIN32)
            set_property(TEST ${test_name}
                PROPERTY ENVIRONMENT
                    "PATH=${WIN_BUILD_BINDIR}\\${CMAKE_BUILD_TYPE}\;${WIN_PATH}\n"
                    "PYTHONPATH=${WIN_BUILD_BASEDIR}\\${CMAKE_INSTALL_PYTHONDIR}\\${CMAKE_BUILD_TYPE}\;${WIN_PYTHONPATH}"
            )
        else()
            set_tests_properties(${test_name}
                PROPERTIES ENVIRONMENT
                    "PYTHONPATH=${openPMD_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
            )
        endif()
    endfunction()

    # Python CLI Modules
    if(openPMD_HAVE_PYTHON)
        # (Note that during setuptools install, these are furthermore installed as
        #  console scripts and replace the all-binary CLI tools.)
        foreach(pymodulename ${openPMD_PYTHON_CLI_MODULE_NAMES})
             add_test(NAME CLI.py.help.${pymodulename}
                 COMMAND ${Python_EXECUTABLE} -m openpmd_api.${pymodulename} --help
                 WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            )
            test_set_pythonpath(CLI.py.help.${pymodulename})
        endforeach()
    endif()

    # Python-based command line tools
    if(openPMD_BUILD_CLI_TOOLS AND openPMD_HAVE_PYTHON)
        # all tools must provide a "--help"
        foreach(toolname ${openPMD_PYTHON_CLI_TOOL_NAMES})
            configure_file(
                ${openPMD_SOURCE_DIR}/src/cli/${toolname}.py
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-${toolname}
                COPYONLY
            )
            add_test(NAME CLI.help.${toolname}.py
                COMMAND ${Python_EXECUTABLE}
                    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-${toolname} --help
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            )
            test_set_pythonpath(CLI.help.${toolname}.py)
        endforeach()

        # openpmd-pipe (python) test
        if( openPMD_HAVE_HDF5
            AND (openPMD_HAVE_ADIOS2 OR openPMD_HAVE_ADIOS1)
            AND EXAMPLE_DATA_FOUND
        )
            if( openPMD_HAVE_MPI )
                set(MPI_TEST_EXE
                    ${MPIEXEC_EXECUTABLE}
                    ${MPI_ALLOW_ROOT}
                    #${MPIEXEC_NUMPROC_FLAG} 2
                )
                add_test(NAME CLI.pipe.py
                    COMMAND sh -c
                        "${MPI_TEST_EXE} ${Python_EXECUTABLE}                      \
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-pipe         \
                            --infile ../samples/git-sample/data%T.h5               \
                            --outfile ../samples/git-sample/data%T.bp &&           \
                                                                                   \
                        ${MPI_TEST_EXE} ${Python_EXECUTABLE}                       \
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-pipe         \
                            --infile ../samples/git-sample/thetaMode/data%T.h5     \
                            --outfile ../samples/git-sample/thetaMode/data.bp &&   \
                                                                                   \
                        ${Python_EXECUTABLE}                                       \
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-pipe         \
                            --infile ../samples/git-sample/thetaMode/data.bp       \
                            --outfile ../samples/git-sample/thetaMode/data%T.json  \
                        "
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                )
            else()
                add_test(NAME CLI.pipe.py
                    COMMAND sh -c
                        "${Python_EXECUTABLE}                                      \
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-pipe         \
                            --infile ../samples/git-sample/data%T.h5               \
                            --outfile ../samples/git-sample/data%T.bp &&           \
                                                                                   \
                        ${Python_EXECUTABLE}                                       \
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-pipe         \
                            --infile ../samples/git-sample/thetaMode/data%T.h5     \
                            --outfile ../samples/git-sample/thetaMode/data%T.bp && \
                                                                                   \
                        ${Python_EXECUTABLE}                                       \
                            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/openpmd-pipe         \
                            --infile ../samples/git-sample/thetaMode/data%T.bp     \
                            --outfile ../samples/git-sample/thetaMode/data%T.json  \
                        "
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                )
            endif()
            test_set_pythonpath(CLI.pipe.py)
        endif()
    endif()

    # Python Examples
    # Current examples all use HDF5, elaborate if other backends are used
    if(openPMD_HAVE_PYTHON AND openPMD_HAVE_HDF5)
        if(EXAMPLE_DATA_FOUND)
            foreach(examplename ${openPMD_PYTHON_EXAMPLE_NAMES})
                configure_file(
                    ${openPMD_SOURCE_DIR}/examples/${examplename}.py
                    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${examplename}.py
                    COPYONLY
                )
                if(openPMD_BUILD_TESTING)
                    if(${examplename} MATCHES "^.*_parallel$")
                        if(openPMD_HAVE_MPI AND MPI4PY_RETURN EQUAL 0)
                            # see https://mpi4py.readthedocs.io/en/stable/mpi4py.run.html
                            add_test(NAME Example.py.${examplename}
                                COMMAND ${MPI_TEST_EXE} ${Python_EXECUTABLE} -m mpi4py
                                    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${examplename}.py
                                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                            )
                        else()
                            continue()
                        endif()
                    else()
                        add_test(NAME Example.py.${examplename}
                            COMMAND ${Python_EXECUTABLE}
                                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${examplename}.py
                            WORKING_DIRECTORY
                                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                        )
                    endif()
                    if(WIN32)
                        string(REGEX REPLACE "/" "\\\\" WIN_BUILD_BASEDIR ${openPMD_BINARY_DIR})
                        string(REGEX REPLACE "/" "\\\\" WIN_BUILD_BINDIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
                        string(REPLACE ";" "\\;" WIN_PATH "$ENV{PATH}")
                        string(REPLACE ";" "\\;" WIN_PYTHONPATH "$ENV{PYTHONPATH}")
                        set_property(TEST Example.py.${examplename}
                            PROPERTY ENVIRONMENT
                                "PATH=${WIN_BUILD_BINDIR}\\${CMAKE_BUILD_TYPE}\;${WIN_PATH}\n"
                                "PYTHONPATH=${WIN_BUILD_BASEDIR}\\${CMAKE_INSTALL_PYTHONDIR}\\${CMAKE_BUILD_TYPE}\;${WIN_PYTHONPATH}"
                        )
                    else()
                        set_tests_properties(Example.py.${examplename}
                            PROPERTIES ENVIRONMENT
                                "PYTHONPATH=${openPMD_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
                        )
                    endif()
                endif()
            endforeach()
        endif()
    endif()
endif()


# Status Message for Build Options ############################################
#
message("")
message("openPMD build configuration:")
message("  library Version: ${openPMD_VERSION}")
message("  openPMD Standard: ${openPMD_STANDARD_VERSION}")
message("  C++ Compiler: ${CMAKE_CXX_COMPILER_ID} "
                        "${CMAKE_CXX_COMPILER_VERSION} "
                        "${CMAKE_CXX_COMPILER_WRAPPER}")
message("    ${CMAKE_CXX_COMPILER}")
message("")
if(openPMD_INSTALL)
    message("  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
    message("        bin: ${CMAKE_INSTALL_BINDIR}")
    message("        lib: ${CMAKE_INSTALL_LIBDIR}")
    message("    include: ${CMAKE_INSTALL_INCLUDEDIR}")
    message("      cmake: ${CMAKE_INSTALL_CMAKEDIR}")
    if(openPMD_HAVE_PYTHON)
        message("     python: ${CMAKE_INSTALL_PYTHONDIR}")
    endif()
    message("")
    message("  Additionally, install following third party libraries:")
    message("    MPark.Variant: ${openPMD_USE_INTERNAL_VARIANT}")
else()
    message("  Installation: OFF")
endif()
message("")
message("  Build Type: ${CMAKE_BUILD_TYPE}")
if(openPMD_BUILD_SHARED_LIBS)
    message("  Library: shared")
else()
    message("  Library: static")
endif()
message("  CLI Tools: ${openPMD_BUILD_CLI_TOOLS}")
message("  Examples: ${openPMD_BUILD_EXAMPLES}")
message("  Testing: ${openPMD_BUILD_TESTING}")
message("  Invasive Tests: ${openPMD_USE_INVASIVE_TESTS}")
message("  Internal VERIFY: ${openPMD_USE_VERIFY}")
message("  Build Options:")

foreach(opt IN LISTS openPMD_CONFIG_OPTIONS)
  if(${openPMD_HAVE_${opt}})
    message("    ${opt}: ON")
  else()
    message("    ${opt}: OFF")
  endif()
endforeach()
message("")
