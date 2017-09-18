cmake_minimum_required(VERSION 2.6)
project(libopenpmd)


set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Force C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables -Wno-padded -Wno-switch-enum -Wno-undefined-func-template")
    #Silence BOOST_TEST
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion -Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-conversion")
    #Silence HDF5
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-id-macro -Wno-deprecated -Wno-old-style-cast")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include)

#TODO remove in release version
set(DEVELOPMENT "on")
if ("${DEVELOPMENT}" STREQUAL "on")
    set(LIBOPENPMD_HDF5 "on")
    set(LIBOPENPMD_ADIOS "off")
    set(LIBOPENPMD_ADIOS2 "off")
    set(LIBOPENPMD_PARALLEL "on")
else()
    #TODO
endif ()

find_package(Boost REQUIRED COMPONENTS system filesystem)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARIES})
endif ()

find_package(MPI)
if (MPI_FOUND)
    if (LIBOPENPMD_PARALLEL)
        add_definitions(-DLIBOPENPMD_WITH_MPI=ON)
    endif ()
    include_directories(${MPI_INCLUDE_PATH})
    link_directories(${MPI_LIBRARIES})
endif ()

find_package(HDF5 QUIET COMPONENTS C)
if (HDF5_FOUND)
    if (LIBOPENPMD_HDF5)
        add_definitions(-DLIBOPENPMD_WITH_HDF5=ON)
        if (HDF5_IS_PARALLEL AND LIBOPENPMD_PARALLEL)
            add_definitions(-DLIBOPENPMD_WITH_PARALLEL_HDF5=ON)
        endif ()
    endif ()
    include_directories(${HDF5_INCLUDE_DIRS})
    link_directories(${HDF5_LIBRARY_DIRS})
endif ()

if (LIBOPENPMD_PARALLEL)
    find_package(ADIOS QUIET)
    if (ADIOS_FOUND AND LIBOPENPMD_ADIOS AND LIBOPENPMD_PARALLEL)
        add_definitions(-DLIBOPENPMD_WITH_PARALLEL_ADIOS1=ON)
    endif()
else()
    find_package(ADIOS QUIET COMPONENTS sequential)
    if (ADIOS_FOUND AND LIBOPENPMD_ADIOS)
        add_definitions(-DLIBOPENPMD_WITH_ADIOS1=ON)
        add_definitions(-D_NOMPI=ON)
    endif()
endif()
if (ADIOS_FOUND)
    include_directories(${ADIOS_INCLUDE_DIRS})
    link_directories(${ADIOS_LIBRARIES})
endif()

find_package(ADIOS2 QUIET)
if (ADIOS2_FOUND)
    if (LIBOPENPMD_ADIOS2)
        add_definitions(-DLIBOPENPMD_WITH_ADIOS2=ON)
        if (ADIOS2_HAVE_MPI AND LIBOPENPMD_PARALLEL)
            add_definitions(-DLIBOPENPMD_WITH_PARALLEL_ADIOS2=ON)
        endif()
    endif()
    include_directories(${ADIOS2_CXX_INCS})
    link_directories(${ADIOS2_CXX_LIBS})
endif()

set(CORE_SOURCE
        src/Writable.cpp
        src/Output.cpp
        src/Iteration.cpp
        src/Record.cpp
        src/RecordComponent.cpp
        src/Dataset.cpp
        src/Mesh.cpp
        src/ParticleSpecies.cpp
        src/Datatype.cpp)
set(IO_SOURCE
        src/IO/AbstractIOHandler.cpp
        src/IO/IOTask.cpp
        src/IO/ADIOS/ADIOS1IOHandler.cpp
        src/IO/ADIOS/ParallelADIOS1IOHandler.cpp
        src/IO/ADIOS/ADIOS2IOHandler.cpp
        src/IO/HDF5/HDF5IOHandler.cpp
        src/IO/HDF5/ParallelHDF5IOHandler.cpp)
set(CORE_TESTS
        test/CoreTest.cpp)
set(AUX_TESTS
        src/Writable.cpp
        src/IO/IOTask.cpp
        src/IO/AbstractIOHandler.cpp
        test/AuxiliaryTest.cpp)
set(SERIAL_IO_TESTS
        test/SerialIOTest.cpp)
set(PARALLEL_IO_TESTS
        test/ParallelIOTest.cpp)

add_executable(libopenpmdCoreTests
        ${CORE_TESTS}
        ${CORE_SOURCE}
        ${IO_SOURCE})
add_executable(libopenpmdAuxiliaryTests
        ${AUX_TESTS})
add_executable(libopenpmdSerialIOTests
        ${CORE_SOURCE}
        ${IO_SOURCE}
        ${SERIAL_IO_TESTS})
add_executable(libopenpmdParallelIOTests
        ${CORE_SOURCE}
        ${IO_SOURCE}
        ${PARALLEL_IO_TESTS})
add_executable(poc_HDF5Writer
        writer.cpp
        ${CORE_SOURCE}
        ${IO_SOURCE})
add_executable(poc_HDF5Reader
        reader.cpp
        ${CORE_SOURCE}
        ${IO_SOURCE})

if (LIBOPENPMD_HDF5)
    target_link_libraries(libopenpmdCoreTests ${HDF5_LIBRARIES})
    target_link_libraries(libopenpmdSerialIOTests ${HDF5_LIBRARIES})
    target_link_libraries(libopenpmdParallelIOTests ${HDF5_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${HDF5_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${HDF5_LIBRARIES})
endif ()
if (LIBOPENPMD_ADIOS)
    target_link_libraries(libopenpmdCoreTests ${ADIOS_LIBRARIES})
    target_link_libraries(libopenpmdSerialIOTests ${ADIOS_LIBRARIES})
    target_link_libraries(libopenpmdParallelIOTests ${ADIOS_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${ADIOS_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${ADIOS_LIBRARIES})
endif ()
if (Boost_FOUND)
    target_link_libraries(libopenpmdCoreTests ${Boost_LIBRARIES})
    target_link_libraries(libopenpmdSerialIOTests ${Boost_LIBRARIES})
    target_link_libraries(libopenpmdParallelIOTests ${Boost_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${Boost_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${Boost_LIBRARIES})
endif ()
if (LIBOPENPMD_PARALLEL)
    target_link_libraries(libopenpmdCoreTests ${MPI_LIBRARIES})
    target_link_libraries(libopenpmdSerialIOTests ${MPI_LIBRARIES})
    target_link_libraries(libopenpmdParallelIOTests ${MPI_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${MPI_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${MPI_LIBRARIES})
endif ()
