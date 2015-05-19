#!/bin/bash

set -o errexit

DIR=$1
OPTIONS=$2
rm -f $DIR/port
mkfifo $DIR/port
cmd="r-parallel-d.pl -bind $$ 1 $DIR"
echo $cmd \& >&2
R_PARALLEL_D_PL_SLEEP_TIME=1 $cmd &
DAEMON_PID=$!
PORT=`cat $DIR/port`
HOST=127.0.0.1

cmd="canoe -v 1 -f canoe.ini -input input -canoe-daemon $HOST:$PORT $OPTIONS"

echo $cmd >&2
exec $cmd

