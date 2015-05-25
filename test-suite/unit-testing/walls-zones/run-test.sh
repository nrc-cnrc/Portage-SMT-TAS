#!/bin/bash
make clean
make all -j 2 CANOE_INI=canoe.ini
make clean
make all -j 2 CANOE_INI=canoe.ini.cube
