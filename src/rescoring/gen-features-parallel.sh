#!/bin/bash
#
# gen-features-parallel.sh
#
# George Foster / Samuel Larkin
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

ORIG_ARGUMENTS="$*"
DEBUG=
VERBOSE=
NOEXEC=
SPROXY=""
PREF=""
ALIGFILE=""
CANOEFILE=""
N=3
JOBS_PER_FF=
RESCORING_MODEL_OUT=
FORCE_OVERWRITE=

OUTPUT_FILE_PATTERN=gen-features-parallel-output.$$
CMDS_FILE=$OUTPUT_FILE_PATTERN.commands
test -f $CMDS_FILE && \rm -f $CMDS_FILE

MODEL=
SFILE=
NBEST=

set -o noclobber

##
## Usage: gen-features-parallel.sh [-v][-n][-s sproxy][-p pref][-a pal-file]
##        [-o RESCORING-MODEL][-N #nodes][-J #jobs_per_ff][-F]
##        [-c canoe-file-with-weights] MODEL SFILE NBEST
##
## Generate a set of feature-value files in parallel, as required by the
## rescoring model file MODEL. For each line in MODEL that is in form
## FileFF:ff.FNAME[.ARGS], gen_feature_values will be run with the feature
## FNAME and arguments ARGS, writing results to the file ff.FNAME.ARGS (see
## example below). SFILE and NBEST are source and nbest files.
##
## On the cluster, this script will launch one job per feature. Otherwise, it
## generates features sequentially - to get parallel behaviour, just launch
## multiple instances of gen-features-parallel.sh with the same arguments
## (these will not overwrite each other's outputs).
## [This behaviour is broken, but if you use -n and pipe the output to
## run-parallel.sh, that works.  See, e.g., rat.sh.]
##
## Options:
##
## -v  Verbose output.
## -n  Don't execute commands, just write them to stdout. This turns off -v.
## -s  Use <sproxy> instead of SFILE when substituting for <src> in MODEL.
## -p  Generate feature files with prefix <pref>. [none]
## -a  Load phrase alignment from file <filename>. [none]
## -c  Get canoe weights from file <filename>. [none]
## -o  Produces a rescore_{train,translate} compatible model to RESCORING-MODEL.
## -N  Number of nodes to run job on. [3]
## -J  Number of jobs per feature function when running in parallel {expert mode}.
##     [2*ceil(N/#computable FF)]
## -F  Force feature function files overwrite. [don't]
##
## For example, if the contents of MODEL are:
##
##   NgramFF:sfile
##   FileFF:ff.Consensus
##   FileFF:ff.IBM2TgtGivenSrc.big-model
##   FileFF:extern
##
## Then:
## - NgramFF is ignored because it is not FileFF
## - ff.Consensus causes the Consensus feature to be generated with a dummy
##   argument and written to the file <pref>ff.Consensus
## - ff.IBM2TgtGivenSrc.big-model causes the IBM2TgtGivenSrc feature to be
##   generated with argument "big-model" and written to file
##   <pref>ff.IBM2TgtGivenSrc.big-model. NOTE: each / in "big-model" will be
##   replaced with _ in this file name, in which case you must also make this
##   substitution in MODEL before passing it to rescore_{train,translate}.
## - FileFF:extern is ignored because it does not begin with "ff.".
##
##   Here are three tags available for the rescoring-model to make the
##   model more generic which trigger some action in rat.sh.
##   <src> this is automatically substituted by SFILE's basename.
##   <ffval-wts> this creates a rescoring-model from the canoe.ini and then
##   substitute the tag with the newly created file.
##   <pfx> which is mandatory and gets substituted by the prefix
##   constructed by rat.sh
##

echo 'gen-feature-parallel.sh, NRC-CNRC, (c) 2006 - 2007, Her Majesty in Right of Canada' >&2

error_exit() {
   for msg in "$@"; do
      echo $msg >&2
   done
   echo "Use -h for help." >&2
   exit 1
}

warn()
{
   echo "WARNING: $*" >&2
}

debug()
{
   test -n "$DEBUG" && echo "<D> $*" >&2
}


# Verify that enough args remain on the command line
# syntax: one_arg_check <args needed> $# <arg name>
# Note that this function expects to be in a while/case structure for
# handling parameters, so that $# still des the option itself.
# exits with error message if the check fails.
arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

min ()
{
   if [ $1 -lt $2 ]; then
      echo $1;
   else
      echo $2;
   fi
}

# Command-line processing
while [ $# -gt 0 ]; do
   case "$1" in
   -d)        DEBUG="-d";;
   -v)        VERBOSE="-v";;
   -n)        NOEXEC="-n";;
   -F)        FORCE_OVERWRITE=1;;

   -h)        cat $0 | egrep '^##' | cut -c4-; exit 1;;
   -s)        arg_check 1 $# $1; SPROXY="$2"; shift;;
   -p)        arg_check 1 $# $1; PREF="$2"; shift;;
   -a)        arg_check 1 $# $1; ALIGFILE=$2; shift;;
   -c)        arg_check 1 $# $1; CANOEFILE=$2; shift;;
   -o)        arg_check 1 $# $1; RESCORING_MODEL_OUT=$2; shift;;
   -N)        arg_check 1 $# $1; N=$2; shift;;
   -J)        arg_check 1 $# $1; JOBS_PER_FF=$2; shift;;

   --)        shift; break;;
   -*)        error_exit "Unknown parameter: $1.";;
   *)         break;;
   esac
   shift
done

if [ $NOEXEC ]; then
   VERBOSE=
fi

if   [ $# -lt 1 ]; then
   error_exit "Error: missing arguments."
elif [ $# -lt 2 ]; then
   error_exit "Error: missing SFILE and NBEST arguments."
elif [ $# -lt 3 ]; then
   error_exit "Error: missing NBEST argument."
fi

MODEL=$1
SFILE=$2
NBEST=$3
WC_NBEST=`gzip -cqfd $NBEST | wc -l`

if [ ! -r $MODEL ]; then
   error_exit "Error: Cannot read MODEL file $MODEL."
fi
if [ ! -r $SFILE ]; then
   error_exit "Error: Cannot read SFILE file $SFILE."
fi
if [ ! -r $NBEST ]; then
   error_exit "Error: Cannot read NBEST file $NBEST."
fi
if [ -n "$RESCORING_MODEL_OUT" ] && [ "$RESCORING_MODEL_OUT" == "$MODEL" ]; then
   error_exit "Specify a different output name $MODEL $RESCORING_MODEL_OUT"
fi
if [ ! "$N" -gt "0" ]; then
   error_exit "You should specify a number of nodes greater than 0";
fi
if [ -z "$JOBS_PER_FF" ]; then
   # Computable feature functions are does that are not commented or FileFF
   L=`cat $MODEL | egrep -v "^\s*#" | egrep -v "FileFF" | wc -l`
   JOBS_PER_FF=`perl -e "use POSIX qw(ceil);print 2*ceil($N/$L)"`;
   debug "JOBS_PER_FF: $JOBS_PER_FF"
fi
if [ ! "$JOBS_PER_FF" -gt "0" ]; then
   error_exit "Number of jobs per feature function must be greater than 0";
fi

if which-test.sh qsub; then
   CLUSTER=1
else
   CLUSTER=0
fi


# Make it easy to recover the command line from saved logs.
if [ $VERBOSE ]; then
   echo $0 $ORIG_ARGUMENTS
   echo Starting on `date`
   echo ""
fi

# Process the canoe submodel weights (if given)


# Rename an existing file to avoid accidentally re-using old data from an
# aborted or completed previous run.
rename_old() {
   __FILENAME=$1
   __SUFFIX=${2-old}
   if [ -e $__FILENAME ]; then
      __BACKUP_SUFFIX=${__SUFFIX}01
      __BACKUP_FILENAME=$__FILENAME.$__BACKUP_SUFFIX
      #echo first $__BACKUP_FILENAME >&2
      while [ -e $__BACKUP_FILENAME ]; do
         __BACKUP_SUFFIX=`
               echo $__BACKUP_SUFFIX |
               perl -e '$name = <STDIN>; chomp $name; $name++; print $name'`
         __BACKUP_FILENAME=$__FILENAME.$__BACKUP_SUFFIX
         #echo while $__BACKUP_FILENAME >&2
      done
      if [ $VERBOSE ]; then
         echo Moving existing file $__FILENAME to $__BACKUP_FILENAME
      fi
      mv $__FILENAME $__BACKUP_FILENAME
   fi
   unset __FILENAME __BACKUP_SUFFIX __BACKUP_FILENAME __SUFFIX
}

CANOEWEIGHTFILE=""
REQUIRES_FFVAL_WTS=`grep -m1 '<ffval-wts>' $MODEL`
if [ -n "$REQUIRES_FFVAL_WTS" ]; then
   if [ -z "$CANOEFILE" ]; then
      echo "Canoe weights file needed by at least one feature" >&2
   else
      CANOEWEIGHTFILE=${PREF}_canoeweights.tmp
      CANOEWEIGHTFILE=`dirname $CANOEWEIGHTFILE`"/_"`basename $CANOEWEIGHTFILE`
      rename_old $CANOEWEIGHTFILE
      canoe2rescoreFile.sh $CANOEFILE > $CANOEWEIGHTFILE
   fi
fi

# Extract the basename of the source file to replace the <src> tag
if [ -z "$SPROXY" ]; then
   SPROXY=$SFILE
fi
SPROXY_BASE=`basename $SPROXY`

# if the user asked for a rescoring-model.rat => delete previous
test -n "$RESCORING_MODEL_OUT" && rename_old $RESCORING_MODEL_OUT

# Process the MODEL file and launch gen_feature_value jobs
# Build a process list for generating the feature function values
NUM_SRC_SENT=`wc -l < $SFILE`
I=1
PARALLEL_FILE=   # List files/features that were run in parallel and need merging
while [ $I -le `cat $MODEL | wc -l` ]; do

   # We MUST clear the command array
   CMD=()

   # Generate feature function files for those feature that are not FileFF
   test -n "$DEBUG" && echo "" # Prettify debugging ;)

   # Reads the next feature to process
   LINE=`head -$I $MODEL | tail -1`

   # Skips commented lines
   COMMENTED=`echo $LINE | perl -ne '/^\s*(#)/o;print $1'`
   if [ -n "$COMMENTED" ]; then
      # Keep the commented line in the outputed rescore-model
      test -n "$RESCORING_MODEL_OUT" && echo "$LINE" >> $RESCORING_MODEL_OUT
      warn "Skipping commented feature: ${LINE#\#}"
      I=$(($I+1))
      continue
   fi

   # parse line from MODEL
   # what's the feature's name
   FEATURE=`echo $LINE | perl -ne '/^([^: ]+)/o;print $1 unless /FileFF/'`

   # Get the feature's argument and change the users' tags accordingly
   # if the user forgot to specifix <pfx>, rat.sh still needs it thus we will
   # add it (second perl command in the pipe).
   ARGS=`echo $LINE |
      perl -ne '/^[^: ]+:(\S+)/o; print $1' |
      sed "s@<src>@$SPROXY@" |
      perl -pe "s@<ffval-wts>\\\$@$CANOEWEIGHTFILE#$PREF@" |
      sed -e "s@<ffval-wts>@$CANOEWEIGHTFILE@" \
          -e "s@<pfx>@$PREF@"`

   # Get the output file name and strips ffvals-wts and pfx from the file name to make it shorter
   FILE=`echo $LINE |
      perl -ne '/^([^ ]+)/o;print $1' |
      perl -pe 's#/#_#g;
                s/:/./ unless /FileFF/' |
      sed -e "s@<src>@$SPROXY_BASE@" \
          -e "s@#<ffval-wts>@@" \
          -e "s@#<pfx>@@"`

   # Finally, in translation mode we need the feature's weight
   WEIGHT=`echo $LINE | perl -ne '/^\S+ (.*)/o; print $1'`

   # For debugging the feature function's parsing
   debug "L: $LINE"
   debug "F: $FEATURE"
   debug "A: $ARGS"
   debug "f: $FILE"
   debug "w: $WEIGHT"

   # Create a rescoring-model for rescore_train and rescore_translate
   if [ -n "$RESCORING_MODEL_OUT" ]; then
      echo -n "$FILE" | perl -pe "s/^/FileFF:ff./ unless /FileFF/" >> $RESCORING_MODEL_OUT
      echo " $WEIGHT" >> $RESCORING_MODEL_OUT
   fi

   # What is the output filename for this feature function
   FF_FILE="${PREF}ff.$FILE"
   FF_FILE=${FF_FILE%\.gz}".gz"

   # Notice that we skip the FFval feature function
   if [ "$FEATURE" != "" ]; then

      test -z "$ARGS" && ARGS="no-arg"

      # Check if this file was previously generated
      if [ -e "$FF_FILE" ]; then
         if [ -n "$FORCE_OVERWRITE" ]; then
            warn "Force overwrite for: $FEATURE"
         elif [ `zcat $NBEST | wc -l` -eq `zcat $FF_FILE | wc -l` ]; then
            warn "Feature file already exists for $FEATURE"
            I=$(($I+1))
            continue
               else
            warn "Invalid feature file for $FEATURE, regenerating one"
            test -n "$VERBOSE" && warn "File $FF_FILE contains " `zcat $FF_FILE | wc -l` "/" `zcat $NBEST | wc -l`
               fi
            fi

      # Does this feature requries the canoe.ini file and do we have it
      NEEDS_FFVAL_WTS=`echo $LINE | egrep '<ffval-wts>'`
      if [ -n "$NEEDS_FFVAL_WTS" -a -z "$CANOEWEIGHTFILE" ]; then
         warn "Canoe weights file needed for calculation of $FEATURE"
         warn "Due to missing info, SKIPPING $FEATURE"
         CMD=""
         I=$(($I+1))
         continue
         fi

      # Checking for alignment file requirement
      ALIGN_OPT=
      NEEDS_ALIGN=`feature_function_tool -a $FEATURE $ARGS | egrep "Alignment:" | cut -f2 -d' '`
      if [ "$NEEDS_ALIGN" == "TRUE" ]; then
         test -n "$VERBOSE" && echo "This feature needs alignment $FEATURE"
         if [ -n "$ALIGFILE" ]; then
            ALIGN_OPT="-a $ALIGFILE"
      else
            warn "Alignment file needed for calculation of $FEATURE"
            warn "Due to missing info, SKIPPING $FEATURE"
            CMD=""
            I=$(($I+1))
            continue
         fi
      fi

      # Parallelize complexe features
      RANGE=
      COMPLEXITY=`feature_function_tool -c $FEATURE $ARGS | egrep "Complexity:" | cut -f2 -d' '`
      debug "COMPLEXITY: $COMPLEXITY"
      case "$COMPLEXITY" in
         0)
            CMD[0]="gen_feature_values $ALIGN_OPT -o $FF_FILE $VERBOSE $FEATURE $ARGS $SFILE $NBEST";;
         [1-2])
            # Add to a list of features that need their chunk files merged
            PARALLEL_FILE=("${PARALLEL_FILE[@]}" $FF_FILE)
            NUM_JOB=$(($JOBS_PER_FF*$COMPLEXITY))
            # Calculate the number of lines per block
            LINES_PER_BLOCK=$(( ( $NUM_SRC_SENT + $NUM_JOB - 1 ) / $NUM_JOB ))
            debug "LINES_PER_BLOCK=$LINES_PER_BLOCK"
            # For all but the last block
            for r in `seq 0 $(($NUM_JOB-1))`; do
               INDEX=`printf ".gfv%3.3d" $r`
               MIN=$(($r*$LINES_PER_BLOCK))
               RANGE="-min $MIN -max $(min $(($MIN+$LINES_PER_BLOCK)) $NUM_SRC_SENT)"
               CMD[$r]="gen_feature_values $ALIGN_OPT $RANGE -o ${FF_FILE%.gz}$INDEX.gz $VERBOSE $FEATURE $ARGS $SFILE $NBEST"
            done
            ;;
      esac

      # noexec stuff
         if [ $NOEXEC ]; then
         for cmd in "${CMD[@]}"; do
            echo $cmd
         done
            I=$(($I+1))
            continue;
         fi

      # Queue the requested command
      for cmd in "${CMD[@]}"; do
         echo $cmd >> $CMDS_FILE
      done
         fi
   I=$(($I+1))
done


# Now that we have the processes' list, we can execute them
if [ -s "$CMDS_FILE" ]; then
   test -n "$VERBOSE" && echo "run-parallel.sh $VERBOSE $CMDS_FILE $N"
   eval "run-parallel.sh $VERBOSE $CMDS_FILE $N"
   RC=$?

   # Now merge feature functions' values that were generated horizontally.
   debug "FF to merge: ${PARALLEL_FILE[@]}"
   for ff in ${PARALLEL_FILE[@]}; do
      ALL_FF_PARTS="${ff%.gz}.gfv???*"
      if [ "$RC" == "0" ] || [ $WC_NBEST -eq `zcat $ALL_FF_PARTS | wc -l` ]; then
         zcat $ALL_FF_PARTS | gzip > $ff
         \rm $ALL_FF_PARTS
      fi
   done

   if (( $RC != 0 )); then
      echo "problems with run-parallel.sh(RC=$RC) - quitting!" >&2
      echo "The failed outputs will not be merged or deleted to give" \
           "the user a chance to salvage as much data as possible." >&2
      exit 4
   fi

   # Finally, delete the commands' file
   \rm -f $CMDS_FILE
fi

# Everything is fine if we get to this point
exit 0;
