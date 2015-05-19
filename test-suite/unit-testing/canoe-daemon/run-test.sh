#!/bin/bash
set -o errexit
set -o verbose

make clean
make -B
rename long cluster-long log.long-*
PORTAGE_NOCLUSTER=1 make -B cmp-long
rename long local-long log.long-*
echo All tests PASSED.
