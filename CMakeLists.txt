cmake_minimum_required(VERSION 3.6)
project(libopenpmd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -std=c++14")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-weak-vtables -Wno-padded -Wno-switch-enum -Wno-vla -Wno-vla-extension -Wno-undefined-func-template")
    #Silence BOOST_TEST
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion -Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-conversion")
    #Silence HDF5
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-id-macro -Wno-deprecated -Wno-old-style-cast")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

find_package(Boost REQUIRED COMPONENTS system filesystem)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARIES})
endif ()

find_package(MPI)
if (MPI_FOUND)
    add_definitions(-DLIBOPENPMD_WITH_MPI)
    include_directories(${MPI_INCLUDE_PATH})
    link_directories(${MPI_LIBRARIES})
endif()

find_package(HDF5 COMPONENTS C)
if (HDF5_FOUND)
    add_definitions(-DLIBOPENPMD_WITH_HDF5)
    if (HDF5_IS_PARALLEL)
        add_definitions(-DLIBOPENPMD_WITH_PARALLEL_HDF5)
    endif()
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

target_link_libraries(libopenpmdCoreTests
        ${HDF5_LIBRARIES}
        ${Boost_LIBRARIES})
target_link_libraries(libopenpmdReadTests
        ${HDF5_LIBRARIES}
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES})
target_link_libraries(poc_HDF5Writer
        ${HDF5_LIBRARIES}
        ${Boost_LIBRARIES})
target_link_libraries(poc_HDF5Reader
        ${HDF5_LIBRARIES}
        ${Boost_LIBRARIES})

