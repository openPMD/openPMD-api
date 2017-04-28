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

set(SOURCE_FILES
        src/Output.cpp
        src/Iteration.cpp
        src/Record.cpp
        src/RecordComponent.cpp
        src/Dataset.cpp
        src/Mesh.cpp
        src/ParticleSpecies.cpp)
add_executable(libopenpmd ${SOURCE_FILES})

set(TESTS test/CoreTest.cpp)
set(AUXTESTS test/AuxiliaryTest.cpp)
add_executable(libopenpmdCoreTests ${TESTS} ${SOURCE_FILES})
add_executable(libopenpmdAuxiliaryTests ${AUXTESTS})

find_package(Boost REQUIRED COMPONENTS system filesystem)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif ()

find_package(HDF5 COMPONENTS C CXX HL REQUIRED)
if (HDF5_FOUND)
    link_directories(${HDF5_LIBRARY_DIRS})
    include_directories(${HDF5_INCLUDE_DIRS})
endif ()

add_executable(poc main.cpp)
add_executable(poc_HDF5Writer
        writer.cpp
        src/IO/IOTask.cpp
        src/IO/AbstractIOHandler.cpp
        src/IO/HDF5/HDF5IOHandler.cpp
        ${SOURCE_FILES})
add_executable(poc_HDF5Reader
        reader.cpp
        src/reader/HDF5Reader.cpp
        ${SOURCE_FILES})

target_link_libraries(poc_HDF5Writer
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        ${HDF5_CXX_LIBRARIES}
        ${Boost_LIBRARIES})
target_link_libraries(poc_HDF5Reader
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        ${HDF5_CXX_LIBRARIES})
