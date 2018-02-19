# Preamble ####################################################################
#
cmake_minimum_required(VERSION 3.10.0)

project(openPMD VERSION 0.1.0) # LANGUAGES CXX

# the openPMD "markup"/"schema" standard version
set(openPMD_STANDARD_VERSION 1.1.0)

set(CMAKE_MODULE_PATH "${openPMD_SOURCE_DIR}/cmake")

# Force C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# Project structure ###########################################################
#
# temporary build directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# install directories
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_INCLUDEDIR include)
set(CMAKE_INSTALL_CMAKEDIR lib/cmake/openPMD)


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
    set(openPMD_CONFIG_OPTIONS ${openPMD_CONFIG_OPTIONS} ${name} PARENT_SCOPE)
endfunction()

openpmd_option(MPI    "Enable MPI support"     AUTO)
openpmd_option(HDF5   "Enable HDF5 support"    AUTO)
openpmd_option(ADIOS1 "Enable ADIOS1 support"  OFF)
openpmd_option(ADIOS2 "Enable ADIOS2 support"  OFF)
# openpmd_option(JSON "Enable JSON support" AUTO)
openpmd_option(PYTHON "Enable Python bindings" AUTO)

option(openPMD_USE_INTERNAL_VARIANT "Use internally shipped MPark.Variant" ON)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;MinSizeRel;RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the build type, e.g. Release or Debug." FORCE)
endif()
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    # TODO: add directly to targets
    add_definitions(-DDEBUG)
endif()


# Warnings ####################################################################
#
# TODO: LEGACY! Use CMake TOOLCHAINS instead!
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,memory,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables -Wno-padded -Wno-switch-enum -Wno-undefined-func-template")
    #Silence BOOST_TEST
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion -Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-conversion")
    #Silence HDF5
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-id-macro -Wno-deprecated -Wno-old-style-cast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cpp")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    #silence BOOST filesystem
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cpp")
endif ()


# Dependencies ################################################################
#
# external library: Boost (mandatory)
find_package(Boost 1.62.0 REQUIRED
    COMPONENTS system filesystem unit_test_framework)

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
    find_package(HDF5 1.8.6 COMPONENTS C)
    if(HDF5_FOUND)
        set(openPMD_HAVE_HDF5 TRUE)
    else()
        set(openPMD_HAVE_HDF5 FALSE)
    endif()
elseif(openPMD_USE_HDF5)
    set(HDF5_PREFER_PARALLEL ${openPMD_HAVE_MPI})
    find_package(HDF5 1.8.6 REQUIRED COMPONENTS C)
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
if(HDF5_IS_PARALLEL AND NOT openPMD_HAVE_MPI)
    message(FATAL_ERROR
        "Found only parallel version of HDF5 but no MPI. Either set "
        "openPMD_USE_MPI=ON to force using MPI or set openPMD_USE_HDF5=OFF "
        "to disable HDF5 or provide a serial install of HDF5.")
endif()

# external library: ADIOS1 (optional)
set(ADIOS1_PREFER_COMPONENTS )
if(openPMD_HAVE_MPI)
    set(ADIOS1_PREFER_COMPONENTS sequential)
endif()
if(openPMD_USE_ADIOS1 STREQUAL AUTO)
    find_package(ADIOS 1.10.0 COMPONENTS ${ADIOS1_PREFER_COMPONENTS})
    if(ADIOS_FOUND)
        set(openPMD_HAVE_ADIOS1 TRUE)
    else()
        set(openPMD_HAVE_ADIOS1 FALSE)
    endif()
elseif(openPMD_USE_ADIOS1)
    find_package(ADIOS 1.10.0 REQUIRED COMPONENTS ${ADIOS1_PREFER_COMPONENTS})
    set(openPMD_HAVE_ADIOS1 TRUE)
else()
    set(openPMD_HAVE_ADIOS1 FALSE)
endif()

# TODO: check!
#if(openPMD_HAVE_MPI AND openPMD_HAVE_ADIOS AND ADIOS_HAVE_SEQUENTIAL)
#    message(FATAL_ERROR "Found MPI but requested ADIOS1 is serial."
#                        "Set openPMD_USE_MPI=OFF to disable MPI.")
#endif()

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
        src/backend/Attributable.cpp
        src/backend/BaseRecordComponent.cpp
        src/backend/GenericPatchData.cpp
        src/backend/MeshRecordComponent.cpp
        src/backend/PatchRecord.cpp
        src/backend/PatchRecordComponent.cpp
        src/backend/Writable.cpp)
set(IO_SOURCE
        src/IO/AbstractIOHandler.cpp
        src/IO/IOTask.cpp
        src/IO/ADIOS/ADIOS1IOHandler.cpp
        src/IO/ADIOS/ParallelADIOS1IOHandler.cpp
        src/IO/ADIOS/ADIOS2IOHandler.cpp
        src/IO/HDF5/HDF5IOHandler.cpp
        src/IO/HDF5/ParallelHDF5IOHandler.cpp)

# library
add_Library(openPMD ${CORE_SOURCE} ${IO_SOURCE})

# properties
set_target_properties(openPMD PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# own headers
target_include_directories(openPMD PUBLIC
    $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${openPMD_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++11 std::variant (C++17 stdlib preview)
if(openPMD_USE_INTERNAL_VARIANT)
    message(STATUS "MPark.Variant: Using INTERNAL version 1.3.0")
else()
    find_package(mpark_variant 1.3.0 REQUIRED)
    target_link_libraries(openPMD PUBLIC mpark_variant)
endif()

if(TARGET Boost::filesystem)
    target_link_libraries(openPMD PUBLIC
        Boost::boost Boost::system Boost::filesystem)
else()
    target_link_libraries(openPMD PUBLIC
        ${Boost_LIBRARIES})
    target_include_directories(openPMD SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
endif()

if(openPMD_HAVE_MPI)
    # MPI targets: CMake 3.9+
    # note: often the PUBLIC dependency to CXX is missing in C targets...
    target_link_libraries(openPMD PUBLIC MPI::MPI_C MPI::MPI_CXX)

    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_MPI=1")
endif()

if(openPMD_HAVE_HDF5)
    target_link_libraries(openPMD PUBLIC ${HDF5_LIBRARIES})
    target_include_directories(openPMD SYSTEM PUBLIC ${HDF5_INCLUDE_DIRS})
    target_compile_definitions(openPMD PUBLIC ${HDF5_DEFINITIONS})
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_HDF5=1")
endif()

if(openPMD_HAVE_ADIOS1)
    target_link_libraries(openPMD PUBLIC ${ADIOS_LIBRARIES})
    target_include_directories(openPMD SYSTEM PUBLIC ${ADIOS_INCLUDE_DIRS})
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_ADIOS1=1")
endif()

if(openPMD_HAVE_ADIOS2)
    target_link_libraries(openPMD PUBLIC ADIOS2::ADIOS2)
    target_compile_definitions(openPMD PUBLIC "-DopenPMD_HAVE_ADIOS2=1")
endif()

# python bindings
if(openPMD_HAVE_PYTHON)
    pybind11_add_module(openPMD.py MODULE
        src/binding/python/openPMD.cpp
        src/binding/python/Dataset.cpp
        src/binding/python/Datatype.cpp
        src/binding/python/Iteration.cpp
        src/binding/python/IterationEncoding.cpp
        src/binding/python/RecordComponent.cpp
        src/binding/python/Series.cpp
    )
    target_link_libraries(openPMD.py PRIVATE openPMD)

    set(CMAKE_INSTALL_PYTHONDIR
        "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages"
    )
    set(CMAKE_INSTALL_PYTHONDIR "${CMAKE_INSTALL_PYTHONDIR}"
        CACHE INTERNAL "" FORCE
    )
    set_target_properties(openPMD.py PROPERTIES
        OUTPUT_NAME openPMD
        LIBRARY_OUTPUT_DIRECTORY ${openPMD_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
        RUNTIME_OUTPUT_DIRECTORY ${openPMD_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
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
    #ParallelIO
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
foreach(testname ${openPMD_TEST_NAMES})
    add_executable(${testname}Tests test/${testname}Test.cpp)
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
    if(TARGET Boost::unit_test_framework)
        target_link_libraries(${testname}Tests PRIVATE Boost::unit_test_framework)
    endif()
endforeach()
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
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
if(openPMD_HAVE_PYTHON)
    install(TARGETS openPMD.py
        DESTINATION ${CMAKE_INSTALL_PYTHONDIR}
    )
endif()
install(DIRECTORY "${openPMD_SOURCE_DIR}/include/."
  DESTINATION include
  PATTERN ".svn" EXCLUDE
  PATTERN ".git" EXCLUDE
)

# CMake package file for find_package(openPMD::openPMD) in depending projects
install(EXPORT openPMDTargets
    FILE openPMDTargets.cmake
    NAMESPACE openPMD::
    DESTINATION lib/cmake/openPMD
)
install(
    FILES
        ${openPMD_BINARY_DIR}/openPMDConfig.cmake
        ${openPMD_BINARY_DIR}/openPMDConfigVersion.cmake
    DESTINATION lib/cmake/openPMD
)


# Tests #######################################################################
#
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

#foreach(examplename ${openPMD_EXAMPLE_NAMES})
#    add_test(${examplename} ${examplename})
#endforeach()


# Status Message for Build Options ############################################
#
message("")
message("openPMD build configuration:")
message("  library Version: ${openPMD_VERSION}")
message("  openPMD Standard: ${openPMD_STANDARD_VERSION}")
message("  C++ Compiler : ${CMAKE_CXX_COMPILER_ID} "
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
message("  Build Type: ${CMAKE_BUILD_TYPE}")
message("  Build Options:")

foreach(opt IN LISTS openPMD_CONFIG_OPTIONS)
  if(${openPMD_HAVE_${opt}})
    message("    ${opt}: ON")
  else()
    message("    ${opt}: OFF")
  endif()
endforeach()
message("")
