#!/bin/bash

: "${MODE:=test}"

export MODE

make -j tests

for test in $(ls dist/$MODE/tests/*); do
    valgrind $test
done

gcovr *
