#!/bin/bash

make clean
run-parallel.sh -psub "-mem 8" -e 'make all -B' 1
