#!/bin/bash
sed 's/$/ẞ/' < SerialIOTest.cpp \
    | tr -d '\n' \
    | sed -E 's/(TEST_CASE\( *"([^"]*)"[^ẞ]*ẞ[^ẞ]*ẞ)/\1    std::cout << "STARTING TEST: \2" << std::endl;\n/g' \
    | sed 's/ẞ/\n/g'