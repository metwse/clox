#!/bin/bash

: "${MODE:=release}"

export MODE

make -j tests

for test in $(ls dist/$MODE/tests/*); do
    valgrind $test
done

gcovr *
