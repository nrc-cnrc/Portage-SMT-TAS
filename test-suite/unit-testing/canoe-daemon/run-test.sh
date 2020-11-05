#!/bin/bash
set -o errexit
set -o verbose

make clean
make -B
for x in log.long-*; do
   mv $x ${x/long/cluster-long}
done
PORTAGE_NOCLUSTER=1 make -B cmp-long
for x in log.long-*; do
   mv $x ${x/long/local-long}
done
echo All tests PASSED.
