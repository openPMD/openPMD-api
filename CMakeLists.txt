cmake_minimum_required(VERSION 2.6)
project(libopenpmd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -std=c++11")
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
    set(LIBOPENPMD_PARALLEL "on")
else()
    if(ENV{LIBOPENPMD_HDF5})
        set(LIBOPENPMD_HDF5 "on")
    endif()
    if(ENV{LIBOPENPMD_PARALLEL})
        set(LIBOPENPMD_PARALLEL "on")
    endif()
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

find_package(HDF5 COMPONENTS C)
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

set(CORE_SOURCE
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
        src/IO/HDF5/HDF5IOHandler.cpp
        src/IO/HDF5/ParallelHDF5IOHandler.cpp)
set(CORE_TESTS
        test/CoreTest.cpp)
set(AUX_TESTS
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
