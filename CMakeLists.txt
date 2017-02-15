cmake_minimum_required(VERSION 3.6)
project(libopenpmd)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CC_COMPILER clang)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++98-compat -Wno-documentation -Wno-weak-vtables -Wno-padded")
#Silence the verbose BOOST_TEST
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-disabled-macro-expansion -Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-conversion")

set(SOURCE_FILES src/Output.cpp src/Iteration.cpp src/Record.cpp src/RecordComponent.cpp src/Mesh.cpp)
add_executable(libopenpmd ${SOURCE_FILES})

set(TESTS test/CoreTest.cpp)
set(AUXTESTS test/AuxiliaryTest.cpp)
#set(WRITETESTS test/WriterTest.cpp src/TxtWriter.cpp)
add_executable(libopenpmdCoreTests ${TESTS} ${SOURCE_FILES})
add_executable(libopenpmdAuxiliaryTests ${AUXTESTS})
add_executable(libopenpmdWriterTests ${WRITETESTS} ${SOURCE_FILES})
add_executable(poc main.cpp)
