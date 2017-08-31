cmake_minimum_required(VERSION 2.6)
project(libopenpmd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -std=c++11")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables -Wno-padded -Wno-switch-enum -Wno-vla -Wno-vla-extension -Wno-undefined-func-template")
    #Silence BOOST_TEST
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion -Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-conversion")
    #Silence HDF5
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-id-macro -Wno-deprecated -Wno-old-style-cast")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif ()

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Boost REQUIRED COMPONENTS system filesystem)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARIES})
endif ()

find_package(MPI)
if (MPI_FOUND)
    if (LIBOPENPMD_PARALLEL)
        add_definitions(-DLIBOPENPMD_WITH_MPI)
    endif ()
    include_directories(${MPI_INCLUDE_PATH})
    link_directories(${MPI_LIBRARIES})
endif ()

find_package(HDF5 COMPONENTS C)
if (HDF5_FOUND)
    if (LIBOPENPMD_WITH_HDF5)
        add_definitions(-DLIBOPENPMD_WITH_HDF5)
    endif ()
    if (HDF5_IS_PARALLEL)
        if (LIBOPENPMD_WITH_HDF5 AND LIBOPENPMD_PARALLEL)
            add_definitions(-DLIBOPENPMD_WITH_PARALLEL_HDF5)
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
set(READ_TESTS
        test/ReadTest.cpp)

add_executable(libopenpmdCoreTests
        ${CORE_TESTS}
        ${CORE_SOURCE}
        ${IO_SOURCE})
add_executable(libopenpmdAuxiliaryTests
        ${AUX_TESTS})
add_executable(libopenpmdReadTests
        ${CORE_SOURCE}
        ${IO_SOURCE}
        ${READ_TESTS})
add_executable(poc_HDF5Writer
        writer.cpp
        ${CORE_SOURCE}
        ${IO_SOURCE})
add_executable(poc_HDF5Reader
        reader.cpp
        ${CORE_SOURCE}
        ${IO_SOURCE})

if (HDF5_FOUND)
    target_link_libraries(libopenpmdReadTests ${HDF5_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${HDF5_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${HDF5_LIBRARIES})
endif ()
if (Boost_FOUND)
    target_link_libraries(libopenpmdCoreTests ${Boost_LIBRARIES})
    target_link_libraries(libopenpmdReadTests ${Boost_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${Boost_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${Boost_LIBRARIES})
endif ()
if (MPI_FOUND)
    target_link_libraries(libopenpmdReadTests ${MPI_LIBRARIES})
    target_link_libraries(poc_HDF5Writer ${MPI_LIBRARIES})
    target_link_libraries(poc_HDF5Reader ${MPI_LIBRARIES})
endif ()
