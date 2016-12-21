cmake_minimum_required(VERSION 3.6)
project(libopenpmd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Weverything -Wno-c++98-compat -Wno-documentation")

set(SOURCE_FILES main.cpp)
add_executable(libopenpmd ${SOURCE_FILES})

set(TESTS test/CoreTest.cpp src/Output.cpp src/Iteration.cpp)
set(AUXTESTS test/AuxiliaryTest.cpp)
add_executable(libopenpmdCoreTests ${TESTS})
add_executable(libopenpmdAuxiliaryTests ${AUXTESTS})