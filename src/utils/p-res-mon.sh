#!/bin/bash
# $Id$

# @file p-res-mon.sh
# @brief Map/Reduce wall/cpu time, VMEM & RAM.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologiesm
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada
 

# Includes NRC's bash library.
source `dirname $0`/sh_utils.sh


# Summarizes Wall/Cpu time, VMEM & RAM.
# If there is an argument, the output will mimic grep's output => filename:line
parseCommand() {
   ARG=$1
   declare line
   declare VMEM_LIST=0
   declare RAM_LIST=0
   declare CPU_TIME_LIST=0
   declare WALL_TIME=0

   while read line
   do
      # Indicate that we are aware of this line.
      [[ $DEBUG ]] && echo -n "{> "
      echo $line

      # Get all occurences of VMEM
      if [[ $line =~ "VMEM ([0-9.]+)G" ]]; then
         VMEM_LIST="$VMEM_LIST ${BASH_REMATCH[1]}"
      fi

      # Get all occurences of RAM
      if [[ $line =~ "RAM ([0-9.]+)G" ]]; then
         RAM_LIST="$RAM_LIST ${BASH_REMATCH[1]}"
      fi

      #RP-Totals: Wall time 21s CPU time 40.03s Max VMEM 0.076G Max RAM 0.007G
      if [[ $line =~ "CPU time ([0-9.]+)s" ]]; then
         CPU_TIME_LIST="$CPU_TIME_LIST ${BASH_REMATCH[1]}"
      fi
      if [[ $line =~ "User ([0-9.]+)s" ]]; then
         CPU_TIME_LIST="$CPU_TIME_LIST ${BASH_REMATCH[1]}"
      fi

      # Find the overall wall time.
      if [[ $line =~ "Master-Wall-Time ([0-9.]+)s" ]]; then
         WALL_TIME=${BASH_REMATCH[1]}
      fi
   done

   [[ $DEBUG ]] && echo "<D> parseCommand: $WALL_TIME : $CPU_TIME_LIST : $VMEM_LIST : $RAM_LIST" >&2

   MON_VSZ=`echo $VMEM_LIST | egrep -o "[0-9.]+" | sum.pl -m`
   MON_RSS=`echo $RAM_LIST | egrep -o "[0-9.]+" | sum.pl -m`
   CPU_TIME=`echo $CPU_TIME_LIST | egrep -o "[0-9.]+" | sum.pl`

   RESULT="${ARG}P-RES-MON\tWALL TIME: ${WALL_TIME}s\tCPU TIME: ${CPU_TIME}s\tVSZ: ${MON_VSZ}G\tRSS: ${MON_RSS}G"
   [[ $DEBUG ]] && echo -e "<D> $RESULT" >&2
   sync
   echo -e $RESULT | second-to-hms.pl
}



# Combine all summary into one final summary.
reduce() {
   declare line
   declare VMEM_LIST=0
   declare RAM_LIST=0
   declare CPU_TIME_LIST=0
   declare WALL_TIME_LIST=0

   while read line
   do
      [[ $DEBUG ]] && echo "<Dr> $line" >&2
      # Skip commented lines.
      if [[ $line =~ "^([^>]+):(P-RES-MON>WALL TIME: ([0-9.]+)s>CPU TIME: ([0-9.]+)s>VSZ: ([0-9.]+)G>RSS: ([0-9.]+)G)" ]]; then
         echo -e ">`basename ${BASH_REMATCH[1]}`:${BASH_REMATCH[2]}"
         WALL_TIME_LIST=" $WALL_TIME_LIST ${BASH_REMATCH[3]}"
         CPU_TIME_LIST="$CPU_TIME_LIST ${BASH_REMATCH[4]}"
         VMEM_LIST="$VMEM_LIST ${BASH_REMATCH[5]}"
         RAM_LIST="$RAM_LIST ${BASH_REMATCH[6]}"
      elif [[ $line =~ "P-RES-MON>WALL TIME:" ]]; then
         echo -e ">$line"
      fi
   done

   [[ $DEBUG ]] && echo "DEBUG reduce: $WALL_TIME_LIST : $CPU_TIME_LIST : $VMEM_LIST : $RAM_LIST" >&2

   MON_VSZ=`echo $VMEM_LIST | egrep -o "[0-9.]+" | sum.pl -m`
   MON_RSS=`echo $RAM_LIST | egrep -o "[0-9.]+" | sum.pl -m`
   CPU_TIME=`echo $CPU_TIME_LIST | egrep -o "[0-9.]+" | sum.pl`
   WALL_TIME=`echo $WALL_TIME_LIST | egrep -o "[0-9.]+" | sum.pl`

   RESULT="P-RES-MON\tWALL TIME: ${WALL_TIME}s\tCPU TIME: ${CPU_TIME}s\tVSZ: ${MON_VSZ}G\tRSS: ${MON_RSS}G"
   [[ $DEBUG ]] && echo -e "<D> $RESULT" >&2
   echo -e $RESULT
}



TIMEFORMAT="Single-job-total: Real %3Rs User %3Us Sys %3Ss PCPU %P%%"
WORKDIR_ABSOLUTE=/tmp
run_cmd() {
   if [[ "$1" = "-no-error" ]]; then
      shift
      RUN_CMD_NO_ERROR=1
   else
      RUN_CMD_NO_ERROR=
   fi
   (( $VERBOSE > 0 )) && date >&2
   (( $VERBOSE > 0 )) && echo "$@" >&2
   if [[ ! $NOTREALLY ]]; then
      START_TIME=`date +"%s"`
      MON_FILE=`mktemp $WORKDIR_ABSOLUTE/mon.run_cmd.XXXXXXXX`
      process-memory-usage.pl -s 1 30 $$ > $MON_FILE &
      MON_PID=$!
      time {
         # time this block as a whole so that sync (below) can be effective
         # before the output of time itself is printed to STDERR
         eval "$@"
         rc=$?
         sync
      }
      kill -10 $MON_PID
      sync
      echo "run_cmd finished (rc=$rc): $@" >&2
      if [ -s "$MON_FILE" -a $(( `wc -l < $MON_FILE` > 1 )) ]; then
         MON_VSZ=`egrep -ho 'vsz: [0-9.]+G' $MON_FILE 2> /dev/null | egrep -o "[0-9.]+" | sum.pl -m`
         MON_RSS=`egrep -ho 'rss: [0-9.]+G' $MON_FILE 2> /dev/null | egrep -o "[0-9.]+" | sum.pl -m`
         echo "run_cmd rc=$rc Max VMEM ${MON_VSZ}G Max RAM ${MON_RSS}G" >&2
      fi
      END_TIME=`date +%s`
      WALL_TIME=$((END_TIME - START_TIME))
      echo "Master-Wall-Time ${WALL_TIME}s" >&2

      if [[ -z "$RUN_CMD_NO_ERROR" && "$rc" != 0 ]]; then
         [[ $DEBUG ]] && echo "Oups there was an error." >&2
         (( $VERBOSE > 0 )) && echo "Exit status: $rc is not zero - aborting." >&2
         exit $rc
      fi
   fi
}



usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: $0 [options] {cmd | files}

  Map/Reduce Wall/Cpu time, VMEM & RAM.

Options:

  -t(iming)   Does the necesary thing to time a command.

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   #-m|-meta)            META=1;;
   -m|-meta)            META=$2;shift;;
   -t|-time)            TIME_CMD=1;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

# Escape arguments that might have a space in them.
declare -a ARGUMENTS;
ARGUMENTS=( "$@" )
for ((i=0;i<${#ARGUMENTS};i++)); do
   if [[ ${ARGUMENTS[$i]} =~ " " ]]; then
      ARGUMENTS[$i]="\"${ARGUMENTS[$i]}\"";
   fi
done


# If there is no argument, there is nothing to do.
[[ ${#ARGUMENTS[@]} == 0 ]] && exit 0;

# Make sure we detect failure in a serie of pipes.
set -o pipefail;

# The user wants to time a command that doesn't produce statistics and he's
# asking us calculate the statistics for him.
if [[ $TIME_CMD ]]; then
   if [[ $DEBUG ]]; then
      # We need to warn the debugger both on stdout & stderr that this script
      # is screwing up the streams.
      {
         echo "WARNING: Debug mode messes with stderr & stdout."
         echo "<D> cmd: ${ARGUMENTS[@]}";
      } | tee /dev/stderr
   fi
   # Here we want the original stdout as pass-through and we want to process
   # the original stderr.
   { { run_cmd "${ARGUMENTS[@]}"; sync; } 3>&1 1>&2 2>&3 | parseCommand; sync; } 3>&1 1>&2 2>&3

# Is the first argument an executable?
elif which-test.sh $1 && [ -x "`which $1`" ]; then
   echo "Parsing command"
   { eval "${ARGUMENTS[@]}"; } 2>&1 | parseCommand | second-to-hms.pl

# Ok we assume the user provided us with only files thus he wants us to do a
# one line summary.
else
   [[ $META ]] && META="$META:"
   { for f in $*; do
      [[ $DEBUG ]] && echo "Processing $f" >&2
      g=`basename $f`
      if ! { set -o pipefail; grep -h 'P-RES-MON' $f | sed "s/^P-RES-MON/${g}:P-RES-MON/"; }; then
         [[ $DEBUG ]] && echo "Calculating on the fly" >&2
         cat $f | parseCommand "$f:"
      fi
   done } \
   | second-to-hms.pl -r \
   | sed 's/\t/>/g' \
   | reduce \
   | sed 's/>/\t/g' \
   | sed "s/^P-RES-MON/${META}P-RES-MON/" \
   | second-to-hms.pl
fi

