cmake_minimum_required(VERSION 3.6)
project(libopenpmd)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CC_COMPILER clang)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat -Wno-documentation -Wno-weak-vtables -Wno-padded -Wno-missing-variable-declarations")
#Silence BOOST_TEST
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion -Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-conversion")
#Silence HDF5
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-id-macro -Wno-deprecated")

find_package(Boost REQUIRED COMPONENTS system filesystem)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif ()

find_package(HDF5 REQUIRED COMPONENTS C CXX HL)
if (HDF5_FOUND)
    link_directories(${HDF5_LIBRARY_DIRS})
    include_directories(${HDF5_INCLUDE_DIRS})
endif ()


set(CORE_SOURCE
        src/Output.cpp
        src/Iteration.cpp
        src/Record.cpp
        src/RecordComponent.cpp
        src/Dataset.cpp
        src/Mesh.cpp
        src/ParticleSpecies.cpp)
set(IO_SOURCE
        src/IO/AbstractIOHandler.cpp
        src/IO/IOTask.cpp
        src/IO/HDF5/HDF5IOHandler.cpp)
set(CORE_TESTS
        test/CoreTest.cpp)
set(AUX_TESTS
        test/AuxiliaryTest.cpp)

add_executable(libopenpmdCoreTests
        ${CORE_TESTS}
        ${CORE_SOURCE}
        ${IO_SOURCE})
add_executable(libopenpmdAuxiliaryTests
        ${AUX_TESTS})
add_executable(poc_HDF5Writer
        writer.cpp
        ${CORE_SOURCE}
        ${IO_SOURCE})
add_executable(poc_HDF5Reader
        reader.cpp
#        src/reader/HDF5Reader.cpp
        ${SOURCE_FILES})

target_link_libraries(libopenpmdCoreTests
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        ${HDF5_CXX_LIBRARIES}
        ${Boost_LIBRARIES})
target_link_libraries(poc_HDF5Writer
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        ${HDF5_CXX_LIBRARIES}
        ${Boost_LIBRARIES})
target_link_libraries(poc_HDF5Reader
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        ${HDF5_CXX_LIBRARIES})
