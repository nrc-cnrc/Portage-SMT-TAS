#!/bin/bash
# @file incremental-training-add-sentence-pair.sh
# @brief
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#    Information and Communications Technologies
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
readonly corpora=corpora
readonly queue=queue
readonly queue_fd=200
readonly queue_lock=queue.lock
readonly istraining_fd=201
readonly istraining_lock=istraining.lock

# Includes NRC's bash library.
bin=`dirname $0`
if [[ ! -r $bin/sh_utils.sh ]]; then
   # assume executing from src/* directory
   #bin="$bin/../utils"
   bin="$bin/utils"
fi
source $bin/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

function usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && prog=$BASH_REMATCH || prog=$0
   cat <<==EOF== >&2

Usage: $prog [options] INCREMENTAL_SCRIPT  SOURCE TRANSLATION

  Brief description

Options:

  -unittest         special flag when performing unittesting.
  -extra-data  ARG  translation pair extra data preferably a json object.

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -extra-data)         arg_check 1 $# $1; readonly extra_data=$2; shift;;

   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -u|-unittest)        unittest=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

readonly incremental_training=$1
readonly source_sentence=`tr "\t" " " <<< $2`
readonly target_sentence=`tr "\t" " " <<< $3`

# When running in unittest mode, it should automatically trigger verbose.
[[ $unittest ]] && VERBOSE=$(( $VERBOSE + 1 ))

if [[ -z $incremental_training ]]; then
   ! echo "Error: You must provide an incremental training script." >&2
   exit 1
fi

# TODO:
# - error if only one sentence is provided?
# - error if either sentence is empty?



eval "exec $queue_fd>$queue_lock"
verbose 1 "Locking the queue"
flock --exclusive $queue_fd

if [[ -n "$source_sentence" && -n "$target_sentence" ]]; then
   verbose 1 "Adding to the queue: $source_sentence $target_sentence"
   [[ -z $unittest ]] && usleep 300000  # Unittest delay
   {
      echo -n `date +"%F %T"`
      echo -n $'\t'
      echo -n $source_sentence
      echo -n $'\t'
      echo -n $target_sentence
      [[ -z "$extra_data" ]] || echo -ne "\t$extra_data"
      echo
   } >> $queue
   if [[ $? -ne 0 ]];
   then
      echo "Error writing sentence pair to the queue" >&2
      verbose 1 "Unlocking isTraining"
      flock --unlock $queue_fd
      exit 1
   fi
fi

{
   eval "exec $istraining_fd>$istraining_lock"
   verbose 1 "Locking isTraining"
   #flock --exclusive --nonblock $istraining_fd \
   #|| { echo "Training is already in progress"; echo "Unlocking isTraining"; flock --unlock $istraining_fd; exit; }
   #flock --exclusive --nonblock $istraining_fd \
   #|| ! echo "Training is already in progress" \
   #|| ! echo "Unlocking isTraining" \
   #|| ! flock --unlock $istraining_fd \
   #|| exit
   flock --exclusive --nonblock $istraining_fd
   if [[ $? -eq 1 ]];
   then
      verbose 1 "Training is already in progress"
      verbose 1 "Unlocking isTraining"
      flock --unlock $queue_fd
      exit 2  # At this point in time, there shouldn't be anyone to see this error code.
   fi

   # As long as there is something in the queue, let's train.
   #while [[ `wc -l < $queue` -gt 0 ]];
   while [[ -s $queue ]];
   do
      verbose 1 "Adding the queue to corpora"
      cat $queue >> $corpora
      cat /dev/null > $queue
      verbose 1 "Unlocking the queue"
      flock --unlock $queue_fd

      # Train
      verbose 1 "$$ is training"
      eval $incremental_training $corpora &> incremental-update.log

      eval "exec $queue_fd>$queue_lock"
      verbose 1 "Locking the queue"
      flock --exclusive $queue_fd
   done

   verbose 1 "Unlocking isTraining"
   flock --unlock $istraining_fd
   verbose 1 "Unlocking the queue"
   flock --unlock $queue_fd
} &

disown -h %1
