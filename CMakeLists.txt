# Preamble ####################################################################
#
cmake_minimum_required(VERSION 3.10.0)

project(openPMD VERSION 0.2.0) # LANGUAGES CXX

# the openPMD "markup"/"schema" standard version
set(openPMD_STANDARD_VERSION 1.1.0)

list(APPEND CMAKE_MODULE_PATH "${openPMD_SOURCE_DIR}/share/openPMD/cmake")


# Project structure ###########################################################
#
# temporary build directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    CACHE PATH "Build directory for archives")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    CACHE PATH "Build directory for libraries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    CACHE PATH "Build directory for binaries")
# install directories
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/openPMD"
    CACHE PATH "CMake config package location for installed targets")
if(WIN32)
    set(CMAKE_INSTALL_LIBDIR Lib)
    set(CMAKE_INSTALL_CMAKEDIR "cmake")
endif()


# Options and Variants ########################################################
#
function(openpmd_option name description default)
    set(openPMD_USE_${name} ${default} CACHE STRING "${description}")
    set_property(CACHE openPMD_USE_${name} PROPERTY
        STRINGS "ON;TRUE;AUTO;OFF;FALSE"
    )
    if(openPMD_HAVE_${name})
        set(openPMD_HAVE_${name} TRUE)
    else()
        set(openPMD_HAVE_${name})
    endif()
    # list of all possible options
    set(openPMD_CONFIG_OPTIONS ${openPMD_CONFIG_OPTIONS} ${name} PARENT_SCOPE)
endfunction()

openpmd_option(MPI              "Enable MPI support"                        AUTO)
openpmd_option(HDF5             "Enable HDF5 support"                       AUTO)
openpmd_option(ADIOS1           "Enable ADIOS1 support"                     AUTO)
openpmd_option(ADIOS2           "Enable ADIOS2 support"                     OFF)
#openpmd_option(JSON             "Enable JSON support"                      AUTO)
openpmd_option(INVASIVE_TESTS   "Enable unit tests that modify source code" AUTO)
openpmd_option(PYTHON           "Enable Python bindings"                    AUTO)

option(openPMD_USE_INTERNAL_VARIANT "Use internally shipped MPark.Variant" ON)
option(openPMD_USE_INTERNAL_CATCH   "Use internally shipped Catch2" ON)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;MinSizeRel;RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the build type, e.g. Release or Debug." FORCE)
endif()
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    # TODO: add directly to targets
    add_definitions(-DDEBUG)
endif()

include(CTest)
# automatically defines: BUILD_TESTING, default is ON

option(BUILD_EXAMPLES "Build the examples" ON)


# Dependencies ################################################################
#
# external library: MPI (optional)
if(openPMD_USE_MPI STREQUAL AUTO)
    find_package(MPI)
    if(MPI_FOUND)
        set(openPMD_HAVE_MPI TRUE)
    else()
        set(openPMD_HAVE_MPI FALSE)
    endif()
elseif(openPMD_USE_MPI)
    find_package(MPI REQUIRED)
    set(openPMD_HAVE_MPI TRUE)
else()
    set(openPMD_HAVE_MPI FALSE)
endif()

# external library: HDF5 (optional)
if(openPMD_USE_HDF5 STREQUAL AUTO)
    set(HDF5_PREFER_PARALLEL ${openPMD_HAVE_MPI})
    find_package(HDF5 1.8.13 COMPONENTS C)
    if(HDF5_FOUND)
        set(openPMD_HAVE_HDF5 TRUE)
    else()
        set(openPMD_HAVE_HDF5 FALSE)
    endif()
elseif(openPMD_USE_HDF5)
    set(HDF5_PREFER_PARALLEL ${openPMD_HAVE_MPI})
    find_package(HDF5 1.8.13 REQUIRED COMPONENTS C)
    set(openPMD_HAVE_HDF5 TRUE)
else()
    set(openPMD_HAVE_HDF5 FALSE)
endif()

# we imply support for parallel I/O if MPI variant is ON
if(openPMD_HAVE_MPI AND openPMD_HAVE_HDF5 AND NOT HDF5_IS_PARALLEL)
    message(FATAL_ERROR
        "Found MPI but only serial version of HDF5. Either set "
        "openPMD_USE_MPI=OFF to disable MPI or set openPMD_USE_HDF5=OFF "
        "to disable HDF5 or provide a parallel install of HDF5.")
endif()
# HDF5 includes mpi.h in the public header H5public.h if HDF5_IS_PARALLEL
if(openPMD_HAVE_HDF5 AND HDF5_IS_PARALLEL AND NOT openPMD_HAVE_MPI)
    message(FATAL_ERROR
        "Found only parallel version of HDF5 but no MPI. Either set "
        "openPMD_USE_MPI=ON to force using MPI or set openPMD_USE_HDF5=OFF "
        "to disable HDF5 or provide a serial install of HDF5.")
endif()

# external library: ADIOS1 (optional)
set(ADIOS1_PREFER_COMPONENTS )
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
    message(FATAL_ERROR "Found MPI but requested ADIOS1 is serial. "
                        "Set openPMD_USE_MPI=OFF to disable MPI.")
endif()
if(NOT openPMD_HAVE_MPI AND openPMD_HAVE_ADIOS1 AND NOT ADIOS_HAVE_SEQUENTIAL)
    message(FATAL_ERROR "Did not find MPI but requested ADIOS1 is parallel. "
                        "Set openPMD_USE_ADIOS1=OFF to disable ADIOS1.")
endif()

# external library: ADIOS2 (optional)
if(openPMD_USE_ADIOS2 STREQUAL AUTO)
    find_package(ADIOS2 2.1.0)
    if(ADIOS2_FOUND)
        set(openPMD_HAVE_ADIOS2 TRUE)
    else()
        set(openPMD_HAVE_ADIOS2 FALSE)
    endif()
elseif(openPMD_USE_ADIOS2)
    find_package(ADIOS2 2.1.0 REQUIRED)
    set(openPMD_HAVE_ADIOS2 TRUE)
else()
    set(openPMD_HAVE_ADIOS2 FALSE)
endif()

# TODO: Check if ADIOS2 is parallel when openPMD_HAVE_MPI is ON

# external library: pybind11 (optional)
if(openPMD_USE_PYTHON STREQUAL AUTO)
    find_package(pybind11 2.2.1 CONFIG)  # 2.3.0
    if(pybind11_FOUND)
        set(openPMD_HAVE_PYTHON TRUE)
    else()
        set(openPMD_HAVE_PYTHON FALSE)
    endif()
elseif(openPMD_USE_PYTHON)
    find_package(pybind11 2.2.1 CONFIG REQUIRED)  # 2.3.0
    set(openPMD_HAVE_PYTHON TRUE)
else()
    set(openPMD_HAVE_PYTHON FALSE)
endif()


# Targets #####################################################################
#
set(CORE_SOURCE
        src/Dataset.cpp
        src/Datatype.cpp
        src/Iteration.cpp
        src/IterationEncoding.cpp
        src/Mesh.cpp
        src/ParticlePatches.cpp
        src/ParticleSpecies.cpp
        src/Record.cpp
        src/RecordComponent.cpp
        src/Series.cpp
        src/auxiliary/Filesystem.cpp
        src/backend/Attributable.cpp
        src/backend/BaseRecordComponent.cpp
        src/backend/MeshRecordComponent.cpp
        src/backend/PatchRecord.cpp
        src/backend/PatchRecordComponent.cpp
        src/backend/Writable.cpp)
set(IO_SOURCE
        src/IO/AbstractIOHandler.cpp
        src/IO/AbstractIOHandlerImpl.cpp
        src/IO/IOTask.cpp
        src/IO/ADIOS/ADIOS1IOHandler.cpp
        src/IO/ADIOS/ParallelADIOS1IOHandler.cpp
        src/IO/HDF5/HDF5IOHandler.cpp
        src/IO/HDF5/ParallelHDF5IOHandler.cpp)

# library
add_library(openPMD ${CORE_SOURCE} ${IO_SOURCE})

# properties
target_compile_features(openPMD
    PUBLIC cxx_std_11
)
set_target_properties(openPMD PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(openPMD PUBLIC "/bigobj")
endif()

# own headers
target_include_directories(openPMD PUBLIC
    $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++11 std::variant (C++17 stdlib preview)
if(openPMD_USE_INTERNAL_VARIANT)
    target_include_directories(openPMD SYSTEM PUBLIC
        $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/variant/include>
    )
    message(STATUS "MPark.Variant: Using INTERNAL version 1.3.0")
else()
    find_package(mpark_variant 1.3.0 REQUIRED)
    target_link_libraries(openPMD PUBLIC mpark_variant)
    message(STATUS "MPark.Variant: Found version ${mpark_variant_VERSION}")
endif()

# Catch2 for unit tests
if(BUILD_TESTING)
    if(openPMD_USE_INTERNAL_CATCH)
        target_include_directories(openPMD SYSTEM PUBLIC
            $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/share/openPMD/thirdParty/catch2/include>
        )
        message(STATUS "Catch2: Using INTERNAL version 2.2.1")
    else()
        find_package(Catch2 2.2.1 CONFIG REQUIRED)
        message(STATUS "Catch2: Found version ${Catch2_VERSION}")
    endif()
endif()

if(openPMD_HAVE_MPI)
    # MPI targets: CMake 3.9+
    # note: often the PUBLIC dependency to CXX is missing in C targets...
    target_link_libraries(openPMD PUBLIC MPI::MPI_C MPI::MPI_CXX)

    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_MPI=1")
else()
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_MPI=0")
endif()

if(openPMD_HAVE_HDF5)
    target_link_libraries(openPMD PUBLIC ${HDF5_LIBRARIES})
    target_include_directories(openPMD SYSTEM PUBLIC ${HDF5_INCLUDE_DIRS})
    target_compile_definitions(openPMD PUBLIC ${HDF5_DEFINITIONS})
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_HDF5=1")
else()
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_HDF5=0")
endif()

if(openPMD_HAVE_ADIOS1)
    target_link_libraries(openPMD PUBLIC ${ADIOS_LIBRARIES})
    target_include_directories(openPMD SYSTEM PUBLIC ${ADIOS_INCLUDE_DIRS})
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_ADIOS1=1")
    target_compile_definitions(openPMD PRIVATE "${ADIOS_DEFINITIONS}")
else()
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_ADIOS1=0")
endif()

if(openPMD_HAVE_ADIOS2)
    target_link_libraries(openPMD PUBLIC ADIOS2::ADIOS2)
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_ADIOS2=1")
else()
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_ADIOS2=0")
endif()

# python bindings
if(openPMD_HAVE_PYTHON)
    pybind11_add_module(openPMD.py MODULE
        src/binding/python/openPMD.cpp
        src/binding/python/AccessType.cpp
        src/binding/python/BaseRecord.cpp
        src/binding/python/BaseRecordComponent.cpp
        src/binding/python/Container.cpp
        src/binding/python/Dataset.cpp
        src/binding/python/Datatype.cpp
        src/binding/python/Iteration.cpp
        src/binding/python/IterationEncoding.cpp
        src/binding/python/Mesh.cpp
        src/binding/python/ParticlePatches.cpp
        src/binding/python/ParticleSpecies.cpp
        src/binding/python/Record.cpp
        src/binding/python/RecordComponent.cpp
        src/binding/python/MeshRecordComponent.cpp
        src/binding/python/Series.cpp
    )
    target_link_libraries(openPMD.py PRIVATE openPMD)

    if(WIN32)
        set(CMAKE_INSTALL_PYTHONDIR_DEFAULT
            "${CMAKE_INSTALL_LIBDIR}\\site-packages"
        )
    else()
        set(CMAKE_INSTALL_PYTHONDIR_DEFAULT
            "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages"
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
        ARCHIVE_OUTPUT_NAME openPMD
        LIBRARY_OUTPUT_NAME openPMD
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
        PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
        COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
    )

    #if(openPMD_HAVE_MPI)
    #    target_link_libraries(openPMD.py PRIVATE PythonModule::mpi4py)
    #endif()
endif()

# tests
set(openPMD_TEST_NAMES
    Core
    Auxiliary
    SerialIO
    ParallelIO
)
# examples
set(openPMD_EXAMPLE_NAMES
    1_structure
    2_read_serial
    3_write_serial
    4_read_parallel
    5_write_parallel
    6_dump_filebased_series
    7_extended_write_serial
)
set(openPMD_PYTHON_EXAMPLE_NAMES
    2_read_serial
    3_write_serial
)

if(openPMD_USE_INVASIVE_TESTS STREQUAL AUTO)
    if(WIN32)
        set(openPMD_HAVE_INVASIVE_TESTS FALSE)
    else()
        set(openPMD_HAVE_INVASIVE_TESTS TRUE)
    endif()
elseif(openPMD_USE_INVASIVE_TESTS)
    set(openPMD_HAVE_INVASIVE_TESTS TRUE)
else()
    set(openPMD_HAVE_INVASIVE_TESTS FALSE)
endif()

if(BUILD_TESTING)
    foreach(testname ${openPMD_TEST_NAMES})
        add_executable(${testname}Tests test/${testname}Test.cpp)

        if(openPMD_HAVE_INVASIVE_TESTS)
            target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_INVASIVE_TESTS=1")
        endif()

        if(openPMD_HAVE_MPI)
            target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_MPI=1")
        else()
            target_compile_definitions(${testname}Tests PUBLIC "-D_NOMPI=1")
        endif()

        if(openPMD_HAVE_HDF5)
            target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_HDF5=1")
        endif()

        if(openPMD_HAVE_ADIOS1)
            target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_ADIOS1=1")
        endif()

        if(openPMD_HAVE_ADIOS2)
            target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_ADIOS2=1")
        endif()
        target_link_libraries(${testname}Tests PRIVATE openPMD)
        if(TARGET Catch2::Catch)
            target_link_libraries(${testname}Tests PRIVATE Catch2::Catch)
        endif()
    endforeach()
endif()

if(BUILD_EXAMPLES)
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
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,memory,undefined")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    # pybind11 does not fix -Wshadow https://github.com/pybind/pybind11/issues/1267
    if(NOT openPMD_HAVE_PYTHON)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
    endif()
    string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
            "-Wno-unknown-pragmas")
    # silence HDF5 (FIXME: -isystem should be enough)
    string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
                  "-Wno-reserved-id-macro -Wno-deprecated -Wno-old-style-cast "
                  "-Wno-cpp")
    # older clangs: silence unknown warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    # pybind11 does not fix -Wshadow https://github.com/pybind/pybind11/issues/1267
    if(NOT openPMD_HAVE_PYTHON)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
    endif()
    string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} "
            "-Wno-unknown-pragmas")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Warning C4503: "decorated name length exceeded, name was truncated"
    # Symbols longer than 4096 chars are truncated
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4503")
endif ()


# Generate Files with Configuration Options ###################################
#
# TODO configure a version.hpp
configure_file(
    ${openPMD_SOURCE_DIR}/openPMDConfig.cmake.in
    ${openPMD_BINARY_DIR}/openPMDConfig.cmake
    @ONLY
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("openPMDConfigVersion.cmake"
    VERSION ${openPMD_VERSION}
    COMPATIBILITY SameMajorVersion
)


# Installs ####################################################################
#
# headers, libraries and exectuables
install(TARGETS openPMD EXPORT openPMDTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
if(openPMD_HAVE_PYTHON)
    install(TARGETS openPMD.py
        DESTINATION ${CMAKE_INSTALL_PYTHONDIR}
    )
endif()
install(DIRECTORY "${openPMD_SOURCE_DIR}/include/openPMD"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# install third-party libraries
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


# Tests #######################################################################
#
if(BUILD_TESTING)
    enable_testing()

    # OpenMPI root guard: https://github.com/open-mpi/ompi/issues/4451
    if("$ENV{USER}" STREQUAL "root")
        set(MPI_ALLOW_ROOT --allow-run-as-root)
    endif()
    set(MPI_TEST_EXE
        ${MPIEXEC_EXECUTABLE}
        ${MPI_ALLOW_ROOT}
        ${MPIEXEC_NUMPROC_FLAG} 2
    )

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
endif()

# TODO: Python Unit tests
#if(BUILD_TESTING)
#    if(openPMD_HAVE_PYTHON)
#        ...
#    endif()
#endif()


# Examples ####################################################################
#
if(BUILD_EXAMPLES)
    # C++ Examples
    # Current examples all use HDF5, elaborate if other backends are used
    if(openPMD_HAVE_HDF5)
        if(EXISTS "${openPMD_BINARY_DIR}/samples/git-sample/")
            foreach(examplename ${openPMD_EXAMPLE_NAMES})
                if(${examplename} MATCHES "^.*_parallel$")
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
        else()
            message(STATUS "Note: Skipping C++ example tests (missing example files)")
        endif()
    endif()
endif()

# Python Examples
if(openPMD_HAVE_PYTHON)
    if(EXISTS "${openPMD_BINARY_DIR}/samples/git-sample/")
        foreach(examplename ${openPMD_PYTHON_EXAMPLE_NAMES})
            add_custom_command(TARGET openPMD.py POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        ${openPMD_SOURCE_DIR}/examples/${examplename}.py
                        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${examplename}.py
            )
            if(BUILD_TESTING)
                add_test(NAME Example.py.${examplename}
                    COMMAND ${PYTHON_EXECUTABLE}
                        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${examplename}.py
                    WORKING_DIRECTORY
                        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                )
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
    else()
        message(STATUS "Note: Skipping Python example tests (missing example files)")
    endif()
endif()

if(NOT EXISTS "${openPMD_BINARY_DIR}/samples/git-sample/")
    if(WIN32)
        message(STATUS "Note: run\n"
                       "    Powershell.exe -File ${openPMD_SOURCE_DIR}/.travis/download_samples.ps1\n"
                       "to add example files to samples/git-sample/ directory!")
    else()
        message(STATUS "Note: run\n"
                       "    . ${openPMD_SOURCE_DIR}/.travis/download_samples.sh\n"
                       "to add example files to samples/git-sample/ directory!")
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
message("")
message("  Build Type: ${CMAKE_BUILD_TYPE}")
message("  Testing: ${BUILD_TESTING}")
message("  Build Options:")

foreach(opt IN LISTS openPMD_CONFIG_OPTIONS)
  if(${openPMD_HAVE_${opt}})
    message("    ${opt}: ON")
  else()
    message("    ${opt}: OFF")
  endif()
endforeach()
message("")
