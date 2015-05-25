#!/bin/sh

make gitignore
set -v
zensprune -vo src/cpt.test 2> log | diff -q - ref/cpt.out || exit
zensprune -vo -i1 src/cpt.test 2> log.itg | diff -q - ref/cpt.out.itg || exit
echo All tests PASSED.
