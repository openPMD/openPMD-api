#!/usr/bin/env bash

if (( $# > 0 )); then
    # received arguments, format those files
    clang-format-18 -i "$@"
else
    # received no arguments, find files on our own
    find include/ src/ test/ examples/ \
            -regextype egrep \
            -type f -regex '.*\.(hpp|cpp|hpp\.in)$' \
        | xargs clang-format-18 -i
fi
