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
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


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

openpmd_option(MPI    "Enable MPI support"    AUTO)
openpmd_option(HDF5   "Enable HDF5 support"   AUTO)
openpmd_option(ADIOS1 "Enable ADIOS1 support" OFF)
openpmd_option(ADIOS2 "Enable ADIOS2 support" OFF)
# openpmd_option(JSON "Enable JSON support" AUTO)
# openpmd_option(PYTHON "Enable Python bindings" OFF)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;MinSizeRel;RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the build type, e.g. Debug." FORCE)
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
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
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
    endif()
elseif(openPMD_USE_HDF5)
    set(HDF5_PREFER_PARALLEL ${openPMD_HAVE_MPI})
    find_package(HDF5 1.8.6 REQUIRED COMPONENTS C)
    if(HDF5_FOUND)
        set(openPMD_HAVE_HDF5 TRUE)
    endif()
else()
    set(openPMD_HAVE_HDF5 FALSE)
endif()

if(openPMD_HAVE_MPI AND openPMD_HAVE_HDF5 AND NOT HDF5_IS_PARALLEL)
    message(FATAL_ERROR
        "Found MPI but only serial version of HDF5. Either set "
        "openPMD_USE_MPI=OFF to disable MPI or set openPMD_USE_HDF5=OFF "
        "to disable HDF5 or provide a parallel install of HDF5.")
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
    endif()
elseif(openPMD_USE_ADIOS1)
    find_package(ADIOS 1.10.0 REQUIRED COMPONENTS ${ADIOS1_PREFER_COMPONENTS})
    if(ADIOS_FOUND)
        set(openPMD_HAVE_ADIOS1 TRUE)
    endif()
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
    find_package(ADIOS2 2.0.0)
    if(ADIOS2_FOUND)
        set(openPMD_HAVE_ADIOS2 TRUE)
    endif()
elseif(openPMD_USE_ADIOS2)
    find_package(ADIOS2 2.0.0 REQUIRED)
    if(ADIOS2_FOUND)
        set(openPMD_HAVE_ADIOS2 TRUE)
    endif()
else()
    set(openPMD_HAVE_ADIOS2 FALSE)
endif()

# TODO: Check if ADIOS2 is parallel when openPMD_HAVE_MPI is ON


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
add_library(openPMD.core ${CORE_SOURCE})
add_library(openPMD.io ${IO_SOURCE})

# own headers
target_include_directories(openPMD.core PUBLIC
    $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${openPMD_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(openPMD.io PUBLIC
    $<BUILD_INTERFACE:${openPMD_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${openPMD_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(TARGET Boost::filesystem)
    target_link_libraries(openPMD.core PUBLIC
        Boost::boost Boost::system Boost::filesystem)
    target_link_libraries(openPMD.io PUBLIC
        Boost::boost Boost::system Boost::filesystem)
else()
    target_link_libraries(openPMD.core PUBLIC
        ${Boost_LIBRARIES})
    target_link_libraries(openPMD.io PUBLIC
        ${Boost_LIBRARIES})
    target_include_directories(openPMD.core SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})

    target_include_directories(openPMD.io SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
endif()

if(openPMD_HAVE_MPI)
    # MPI targets: CMake 3.9+
    # note: often the PUBLIC dependency to CXX is missing in C targets...
    target_link_libraries(openPMD.core PUBLIC MPI::MPI_C MPI::MPI_CXX)
    target_link_libraries(openPMD.io PUBLIC MPI::MPI_C MPI::MPI_CXX)

    target_compile_definitions(openPMD.core PUBLIC "-DopenPMD_HAVE_MPI=ON")
    target_compile_definitions(openPMD.io PUBLIC "-DopenPMD_HAVE_MPI=ON")
else()
    target_compile_definitions(openPMD.core PUBLIC "-D_NOMPI=ON")
    target_compile_definitions(openPMD.io PUBLIC "-D_NOMPI=ON")
endif()

if(openPMD_HAVE_HDF5)
    target_link_libraries(openPMD.io PUBLIC ${HDF5_LIBRARIES})
    target_include_directories(openPMD.io SYSTEM PUBLIC ${HDF5_INCLUDE_DIRS})
    target_compile_definitions(openPMD.io PUBLIC ${HDF5_DEFINITIONS})
    target_compile_definitions(openPMD.io PUBLIC "-DopenPMD_HAVE_HDF5=ON")
endif()

if(openPMD_HAVE_ADIOS1)
    target_link_libraries(openPMD.io PUBLIC ${ADIOS_LIBRARIES})
    target_include_directories(openPMD.io SYSTEM PUBLIC ${ADIOS_INCLUDE_DIRS})
    target_compile_definitions(openPMD.io PUBLIC "-DopenPMD_HAVE_ADIOS1=ON")
endif()

if(openPMD_HAVE_ADIOS2)
    target_link_libraries(openPMD.io PUBLIC ADIOS2::ADIOS2)
    target_compile_definitions(openPMD.io PUBLIC "-DopenPMD_HAVE_ADIOS2=ON")
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
    writer
    reader
)
foreach(testname ${openPMD_TEST_NAMES})
    add_executable(${testname}Tests test/${testname}Test.cpp)
    if(openPMD_HAVE_MPI)
        target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_MPI=ON")
    else()
        target_compile_definitions(${testname}Tests PUBLIC "-D_NOMPI=ON")
    endif()

    if(openPMD_HAVE_HDF5)
        target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_HDF5=ON")
    endif()

    if(openPMD_HAVE_ADIOS1)
        target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_ADIOS1=ON")
    endif()

    if(openPMD_HAVE_ADIOS2)
        target_compile_definitions(${testname}Tests PUBLIC "-DopenPMD_HAVE_ADIOS2=ON")
    endif()
    target_link_libraries(${testname}Tests PRIVATE openPMD.core openPMD.io)
    if(TARGET Boost::unit_test_framework)
        target_link_libraries(${testname}Tests PRIVATE Boost::unit_test_framework)
    endif()
endforeach()
foreach(examplename ${openPMD_EXAMPLE_NAMES})
    add_executable(poc_HDF5${examplename} ${examplename}.cpp)
    target_link_libraries(poc_HDF5${examplename} PRIVATE openPMD.core openPMD.io)
    if(TARGET Boost::unit_test_framework)
        target_link_libraries(poc_HDF5${examplename} PRIVATE Boost::unit_test_framework)
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
install(TARGETS openPMD.io openPMD.core EXPORT openPMDTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
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
    ${MPIEXEC_NUMPROC_FLAG} 4
)

foreach(testname ${openPMD_TEST_NAMES})
    if(${testname} MATCHES "^Parallel.*$")
        if(openPMD_HAVE_MPI)
            add_test(NAME MPI.${testname}
                COMMAND ${MPI_TEST_EXE} --oversubscribe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testname}Tests
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
