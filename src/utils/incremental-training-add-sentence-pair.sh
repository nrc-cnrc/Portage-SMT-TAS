#!/bin/bash
# @file incremental-training-add-sentence-pair.sh
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


# disown vs nohup
# http://unix.stackexchange.com/a/148698

# Locking solution inspired by:
# http://www.kfirlavi.com/blog/2012/11/06/elegant-locking-of-bash-program/

#PSEUDO-CODE:
#
#Acquire a blocking lock on the queue
#Add sentence pair to queue
#if Acquire non-blocking lock on istraining:
#   while !queue.empty:
#     Append queue to corpora
#     Release lock on the queue
#     Train
#     Acquire a blocking lock on the queue
#   Release lock on istraining first
#   then Release lock on queue
#else:
#   Release lock on the queue
#   Do nothing and Return since training is already happening



# IMPORTANT NOTE:
# We need a separate and empty file for the lock.  We cannot use `queue` as the
# queue file AND the lock file.
readonly CORPORA=corpora
readonly QUEUE=queue
readonly QUEUE_FD=200
readonly QUEUE_LOCK=queue.lock
readonly ISTRAINING_FD=201
readonly ISTRAINING_LOCK=istraining.lock

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   #BIN="$BIN/../utils"
   BIN="$BIN/utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -flag)               FLAG=1;;
#   -option-with-1arg)   arg_check 1 $# $1; OPTION_WITH_1ARG_VALUE=$2; shift;;
#   -option-with-2args)  arg_check 2 $# $1
#                        OPTION_WITH_2ARGS_ARG1=$2;
#                        OPTION_WITH_2ARGS_ARG2=$3;
#                        shift; shift;;
#   -int-opt)            arg_check 1 $# $1; arg_check_int $2 $1
#                        INT_ARG=$2; shift;;
#   -pos-int-opt)        arg_check 1 $# $1; arg_check_pos_int $2 $1
#                        POS_INT_ARG=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -u|-unittest)        UNITTEST=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

readonly INCREMENTAL_TRAINING=$1
readonly SOURCE_SENTENCE=$2
readonly TARGET_SENTENCE=$3

# When running in unittest mode, it should automatically trigger verbose.
[[ $UNITTEST ]] && VERBOSE=$(( $VERBOSE + 1 ))

if [[ -z $INCREMENTAL_TRAINING ]]; then
   ! echo "Error: You must provide an incremental training script." >&2
   exit 1
fi

# TODO:
# - error if only one sentence is provided?
# - error if either sentence is empty?



eval "exec $QUEUE_FD>$QUEUE_LOCK"
verbose 1 "Locking the queue"
flock --exclusive $QUEUE_FD

if [[ -n "$SOURCE_SENTENCE" && -n "$TARGET_SENTENCE" ]]; then
   verbose 1 "Adding to the queue: $SOURCE_SENTENCE $TARGET_SENTENCE"
   [[ -z $UNITTEST ]] && usleep 300000  # Unittest delay
   echo -e "$SOURCE_SENTENCE\t$TARGET_SENTENCE" >> $QUEUE
   if [[ $? -ne 0 ]];
   then
      echo "Error writing sentence pair to the queue" >&2
      verbose 1 "Unlocking isTraining"
      flock --unlock $QUEUE_FD
      exit 1
   fi
fi

{
   eval "exec $ISTRAINING_FD>$ISTRAINING_LOCK"
   verbose 1 "Locking isTraining"
   #flock --exclusive --nonblock $ISTRAINING_FD \
   #|| { echo "Training is already in progress"; echo "Unlocking isTraining"; flock --unlock $ISTRAINING_FD; exit; }
   #flock --exclusive --nonblock $ISTRAINING_FD \
   #|| ! echo "Training is already in progress" \
   #|| ! echo "Unlocking isTraining" \
   #|| ! flock --unlock $ISTRAINING_FD \
   #|| exit
   flock --exclusive --nonblock $ISTRAINING_FD
   if [[ $? -eq 1 ]];
   then
      verbose 1 "Training is already in progress"
      verbose 1 "Unlocking isTraining"
      flock --unlock $QUEUE_FD
      exit 2  # At this point in time, there shouldn't be anyone to see this error code.
   fi

   # As long as there is something in the queue, let's train.
   #while [[ `wc -l < $QUEUE` -gt 0 ]];
   while [[ -s $QUEUE ]];
   do
      verbose 1 "Adding the queue to corpora"
      cat $QUEUE >> $CORPORA
      cat /dev/null > $QUEUE
      verbose 1 "Unlocking the queue"
      flock --unlock $QUEUE_FD

      # Train
      verbose 1 "$$ is training"
      eval $INCREMENTAL_TRAINING

      eval "exec $QUEUE_FD>$QUEUE_LOCK"
      verbose 1 "Locking the queue"
      flock --exclusive $QUEUE_FD
   done

   verbose 1 "Unlocking isTraining"
   flock --unlock $ISTRAINING_FD
   verbose 1 "Unlocking the queue"
   flock --unlock $QUEUE_FD
} &

disown -h %1
