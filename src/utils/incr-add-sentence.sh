#!/bin/bash
# @file incr-add-sentence.sh
# @brief Add one sentence pair to an incremental document model.
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

# [Is flock & exec safe in bash?](https://unix.stackexchange.com/a/203719)

#PSEUDO-CODE:
#
#Acquire a blocking lock on the queue
#if block_mode:
#   Add block pair to block queue (two files) or Fail
#else:
#   Add sentence pair to queue or Fail
#
#if Acquire non-blocking lock on istraining:
#   while !queue.empty or !block_queue.empty:
#      Move block queue to block corpus (rename two files)
#      Append queue to corpora
#      Release lock on the queue
#      Train (which aligns block corpus and appends it to corpora, then does regular training)
#      Acquire a blocking lock on the queue
#   Release lock on istraining first
#   then Release lock on queue
#else:
#   Release lock on the queue
#   Do nothing and Return since training is already happening


#Notes
#train requires corpus src_block_corpus tgt_block_corpus
#Only the current two locks, no need for a block_lock
#src_block_queue -> src_block_corpus
#tgt_block_queue -> tgt_block_corpus




# File names of where we store data - do not rename any of these file in the future, to
# avoid breaking backwards compatibility!
readonly corpus=corpora
readonly queue=queue
readonly src_block_corpus=src_block_corpus
readonly src_block_queue=src_block_queue
readonly tgt_block_corpus=tgt_block_corpus
readonly tgt_block_queue=tgt_block_queue

# IMPORTANT NOTE:
# We need a separate and empty file for the lock.  We cannot use `queue` as the
# queue file AND the lock file.
readonly queues_fd=200
readonly queues_lock=queue.lock
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

Usage: $prog [options] [SOURCE TRANSLATION]

  When called with SOURCE and TRANSLATION, it adds the pair to a queue and then
  tries to start an incremental update.
  When called with no arguments, triggers an incremental update.

Options:

  -unittest         special flag when performing unittesting.
  -c                context's canoe.ini.cow path.
  -block            SOURCE and TRANSLATION are blocks of text, with multiple
                    sentences in newline-separated paragraphs, requiring
                    alignment, instead of an individual sentence pair.
  -extra-data  ARG  translation pair extra data, preferably a json object.

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

VERBOSE=0
block_mode=
while [ $# -gt 0 ]; do
   case "$1" in
   -c)                  arg_check 1 $# $1; readonly canoe_ini="-c $2"; shift;;
   -extra-data)         arg_check 1 $# $1; readonly extra_data=$2; shift;;
   -block)              block_mode=1;;

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

# TODO:
# - error if only one sentence is provided?
# - error if either sentence is empty?
readonly source_sentence=`clean-utf8-text.pl <<< "$1" 2> /dev/null`
readonly target_sentence=`clean-utf8-text.pl <<< "$2" 2> /dev/null`

# When running in unittest mode, it should automatically trigger verbose.
[[ $unittest ]] && VERBOSE=$(( $VERBOSE + 1 ))


function guard_block {
   local -r block_marker='__BLOCK_END__'
   local -r text=$1
   sed "s/$block_marker/__BLOCK_END_PROTECTED__/g" <<< "$text"
   echo $block_marker
}


eval "exec $queues_fd>$queues_lock"
verbose 1 "Locking the queue"
flock --exclusive $queues_fd

if [[ -n "$source_sentence" && -n "$target_sentence" ]]; then
   if [[ -n "$block_mode" ]]; then
      verbose 1 "Adding to the block queues: $source_sentence $target_sentence"
      guard_block "$source_sentence" >> $src_block_queue \
      && guard_block "$target_sentence" >> $tgt_block_queue
      #head $src_block_queue $tgt_block_queue >&2
      if [[ $? -ne 0 ]]; then
         echo "Error writing block sentence pair to the queues" >&2
         verbose 1 "Unlocking queue"
         flock --unlock $queues_fd  # If we are exiting anyway, we don't need to unlock!?
         exit 1
      fi
   else
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
      if [[ $? -ne 0 ]]; then
         echo "Error writing sentence pair to the queue" >&2
         verbose 1 "Unlocking queue"
         flock --unlock $queues_fd  # If we are exiting anyway, we don't need to unlock!?
         exit 1
      fi
   fi
fi



function move_queue_to_corpus {
   local -r _queue=$1
   local -r _corpus=$2

   # TODO: $_corpus should be empty but if it isn't, what can we do?  We are in
   # a backgrounded process.
   mv $_queue  $_corpus
   cat /dev/null > $_queue
}



function train {
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
   if [[ $? -eq 1 ]]; then
      verbose 1 "Training is already in progress"
      verbose 1 "Unlocking isTraining"
      flock --unlock $queues_fd
      exit 2  # At this point in time, there shouldn't be anyone to see this error code.
   fi

   # As long as there is something in the queue, let's train.
   while [[ -s $queue || -s $src_block_queue || -s $tgt_block_queue ]]; do
      verbose 1 "Moving the block queues to corpora"
      move_queue_to_corpus $src_block_queue $src_block_corpus
      move_queue_to_corpus $tgt_block_queue $tgt_block_corpus

      verbose 1 "Adding the queue to corpora"
      cat $queue >> $corpus
      cat /dev/null > $queue

      verbose 1 "Unlocking the queues"
      flock --unlock $queues_fd

      # Train
      verbose 1 "$$ is training"
      incr-update.sh $canoe_ini $corpus $src_block_corpus $tgt_block_corpus &> incr-update.log
      echo "$?" > incr-update.status

      verbose 1 "Locking the queue"
      flock --exclusive $queues_fd
   done

   verbose 1 "Unlocking isTraining"
   flock --unlock $istraining_fd
   verbose 1 "Unlocking the queue"
   flock --unlock $queues_fd
}



train &

disown -h %1
