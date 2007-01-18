#!/bin/bash
# $Id$

# \file faucet_launcher.pl - Launch faucet on a free TCP port
#
# PROGRAMMER: Patrick Paul
#
# COMMENTS:
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada


FILE_PREFIX=$1
JOBS_FILENAME=$FILE_PREFIX.jobs
TODO_FILENAME=$FILE_PREFIX.todo
FAUCET=/usr/local/bin/faucet

if [ ! -x $FAUCET ]; then
    FAUCET=`which faucet`
    if [ ! -x $FAUCET ]; then
        echo "Can't execute $FAUCET!  Giving up" >&2
        exit 1
    fi
fi

#parameters are:
#  - file_prefix (UUID.pbs_jobid) for internal work files.
#    The set of instuctions to give upon request must be in file_prefix.jobs
#    The command to launch a new worker must be in file_prefix.psub_cmd

rand_in_range () #floor, max
{
    FLOOR=$1
    RANGE=$2

    ret_val=$(($FLOOR+$(($RANDOM % $(($RANGE - $FLOOR))))))

    echo $ret_val
}

#copy instruction set file to todo file
cp $JOBS_FILENAME $TODO_FILENAME

#get number of instruction in instruction set
NUM_OF_INSTR=$(wc -l < $TODO_FILENAME)

#select a port random within range
port_number=$(rand_in_range 10000 15000)

#store port number in temp file (UUID.port)
echo $port_number > $FILE_PREFIX.port

#start faucet on choosen port
#IMPORTANT do not activate the --err of faucet here this would end up sending
#          the stderr of faucet.pl to the requester this could be undesirable
#          if the requester tries to execute the line.
$FAUCET $port_number --in --out  faucet.pl -file_prefix=$FILE_PREFIX -num=$NUM_OF_INSTR
RC=$?
echo faucet RC $RC >&2


while [ $RC -eq 127 ]
do
    #select new port number to use
    port_number=$(rand_in_range 10000 15000)

    #store port number in file
    echo $port_number > $FILE_PREFIX.port

    #start faucet on a that port number same note as above applies here DO NOT
    #ADD -err to faucet.
    $FAUCET $port_number --in --out faucet.pl -file_prefix=$FILE_PREFIX -num=$NUM_OF_INSTR
    RC=$?
    echo faucet RC $RC >&2
done
