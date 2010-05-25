#!/bin/bash
# $Id$

# @file run-test.sh
# @brief Unittest for r-parallel-d.pl.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologiesm
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

set -e   # Exit when an error occurs.


# Start run-parallel.sh and have the watch dog thread to verify for run-parallel.sh every 30 seconds.
R_PARALLEL_D_PL_DEBUG=1 R_PARALLEL_D_PL_SLEEP_TIME=15 run-parallel.sh -v -v -e "sleep 30" -e "sleep 30" -e "sleep 30" 1 >& log.debug & 
TO_CHECK=$!
echo PID $TO_CHECK

# Checking the functionality:
# - wait some time, let say enough for the first job to be finished plus half
#   way through the first watch dog loop for the second job;
# - kill the master which is run-parallel.sh
# - look for r-parallel-d.pl which should still be running at this point;
# - wait for for the watch dog to do its job;
# - check again to see if r-parallel-d.pl which should be gone at this point;
# - for user, print a status message about the process.
EXIT_STATUS=0
sleep 37 \
&& kill -9 $TO_CHECK \
&& { ps | egrep r-parallel-d.pl; } \
&& sleep 16 \
&& ! { ps | egrep r-parallel-d.pl; } \
&& echo "SUCCESS" || { echo "FAILED" ; EXIT_STATUS=1; }

# Clean-up
find -type d -name run-p.\* | xargs rm -r

exit $EXIT_STATUS
