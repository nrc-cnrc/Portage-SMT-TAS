#!/bin/bash
# $Id$
#
# @file canoe-optimize-weight, a.k.a., cow.sh 
# @brief Optimizer for canoe's weights renamed from rescoreloop.sh
#
# Aaron Tikuisis / George Foster / Eric Joanis
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004-2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004-2009, Her Majesty in Right of Canada

COMPRESS_EXT=".gz"
VERBOSE=
FILTER=
TTABLE_LIMIT=
RANDOM_INIT=0
RESUME=
FLOOR=-1
NOFLOOR=
FLOOR_ARG=
ESTOP=
SEED=0
NR=0
WACC=1
WIN=3
N=200
CFILE=
CANOE=canoe
CANOE_PARALLEL=canoe-parallel.sh
PARALLEL=
RTRAIN=rescore_train
DEFAULT_MODEL=curmodel
LOAD_BALANCING=1
MICRO=0
MICROSM=1
MICROPAR=3
TRAINING_TYPE="-bleu"
SCORE_MAIN=bleumain

FFVALPARSER=nbest2rescore.pl
FFVALPARSER_OPTS="-canoe"

##
## Usage: cow.sh [-v] [-Z] [-resume] [-nbest-list-size N] [-maxiter MAX]
##        [-filt] [-ttable-limit T] [-filt-no-ttable-limit] [-lb|-no-lb]
##        [-floor index|-nofloor] [-model MODEL] [-mad SEED] [-e] [-path P]
##        [-s SEED] [-nr] [-micro M] [-microsm S] [-micropar N]
##        [-wacc N] [-win WIN]
##        [-f CFILE] [-canoe-options OPTS] [-rescore-options OPTS]
##        [-parallel[:"PARALLEL_OPTS"]]
##        [-bleu | -per | -wer]
##        -workdir WORKDIR SFILE RFILE1 RFILE2 .. RFILEn
##
## cow.sh: Canoe Optimize Weights
##
## Does "outer-loop" learning of decoder parameters.  That is, it iteratively
## runs the canoe decoder to generate n-best lists with feature function
## output, then finds weights to optimize the BLEU score on the given n-best
## lists.  This continues until no new sentences are produced at the decoder
## stage. The results (BLEU scores and weights) are appended to the file
## "rescore-results" after each iteration.
##
## Options:
## -v       Verbose output from canoe (verbose level 1) and rescore_train.
## -Z       Do not compress the large temporary files in WORKDIR [do].
## -resume  Resume a previously interrupted run.  Will resume at the decoding
##          phase, regardless of where it had previous been interupted.  Results
##          are not guaranteed to be the same as if a fresh start was done.
##          Normally, you should resume with parameter settings such as maxiter
##          unchanged from the previously-interrupted run.
##          [Fresh start: delete any existing n-best lists, rename any existing
##           model and results files]
## -nbest-list-size The size of the n-best lists to create.  [200]
## -maxiter Do at most MAX iterations.
## -filt    Filter the phrase tables based on SFILE for faster operation.
##          Should not change your results in a significant way (some changes
##          may be observed due to rounding differences).  Creates a single
##          multi-prob table containing only entries from the phrase tables that
##          match SFILE and meet the ttable-limit criterion in CFILE, if any.
##          Uses this table afterwards in place of the original tables.
##          (Recommended option) [don't filter]
## -ttable-limit Use T as the ttable-limit for -filt, instead of the value 
##          specified in CFILE. T is not written back to filtered versions of
##          CFILE.
## -filt-no-ttable-limit  Similar to -filt, with a somewhat faster filtering
##          phase but less effective filtering for canoe.  Creates individual
##          filtered tables containing only entries that match SFILE (without
##          applying any ttable-limit) and uses them afterwards in place of
##          the original tables.  Unlike -filt, this option does not introduce
##          rounding differences, the output should therefore be identical to
##          that obtained with unfiltered tables.  [don't filter]
## -lb      Run canoe-parallel.sh in load balancing mode to help even out the 
##          translation's load distribution [do].
## -no-lb   Disable load-balancing [don't].
## -floor   Index of parameter at which to start zero-flooring. (0 means all)
##          NB, this is NOT the floor threshold and cannot be used with nofloor.
## -nofloor None of the decoder features are floored.  Cannot be used with floor.
##          One of -floor or -nofloor is required, as there is no good default.
## -model   The name of the model file to use.  If this model file already
##          exists, it should have only entries FileFF:allffvalsNUM, where
##          NUM = 1, .. , number of base ff + number of tms + number of lms.
##          To avoid accidental reuse of an old model file, an existing model
##          file with the default name will only be reused if the -model option
##          was explicitly specified. [curmodel]
## -mad     "Mad cow mode": Call canoe with -random-weights -seed SEED
##          when doing the initial translation of the training corpus;
##          this uses a different random set of weights for each
##          sentence. [don't; note that SEED=0 also means don't]
## -e       Use expectation to determine rescore_train stopping point [don't]
## -path    Prepend path P to existing path for locating programs []
## -s       Use SEED to derive random seeds for rescore_train: SEED*10k,
##          SEED*10k+1, SEED*10k+2, ..., for successive cow iterations. [0]
## -nr      Don't re-randomize: use a fixed seed for all iterations [use a
##          different seed as described above]
## -micro   Do at most the first M iterations in micro mode (tuning weights
##          for each sentence separately). [0]
##          Not compatible with -mad.
## -microsm The value of the -sm switch for rescore_train in micro mode [1]
## -micropar The number of sentence-level rescore_trains to run in parallel
##          in micro mode [3]
## -wacc    Start Powell with the WIN best weights from each of the N iterations
##          before the current one. [1]
## -win     Use the WIN best weights from the previous iteration(s) as starting 
##          points for Powell's algorithm. [3] 
## -f       The configuration file to pass to the decoder. [canoe.ini]
## -canoe-options  Provide additional options to the decoder. []
## -rescore-options  Provide additional options to the rescore_train. []
## -parallel or -parallel:"PARALLEL_OPTIONS":  Use canoe-parallel.sh to
##          parallelize decoding.  Any option to canoe-parallel.sh, such as
##          -n <N> if you want to specify the number of parallel jobs to
##          use, -nolowpri to get more memory, -highmem to really get more
##          memory, should be specified after the : and must be in quotes,
##          e.g.:   -parallel:"-n 4".
##                  -parallel:"-n 4 -nolowpri".
##                  -parallel:"-n 4 -highmem".
## -bleu    Perform training using BLEU as the metric [do]
## -per     Perform training using PER as the metric [don't]
## -wer     Perform training using WER as the metric [don't]
## -workdir Store large temporary files in WORKDIR.  WORKDIR should ideally be
##          on a medium which is not backed up, as these files tend to be
##          enormous.
## SFILE    The name of the file containing source text (required).
## RFILE1..n The names of the files containing reference translations (required).
##          The format should be as required by rescore_train; that is, in each
##          ref file, there must be one reference translation for each source
##          sentence in SFILE.  For backwards compatibility reasons, a single
##          combined RFILE created with combine.pl is also still accepted
##
## Dynamic options:
## While COW is running, you can alter some of the running parameters by
## creating a file called COW_DYNAMIC_OPTIONS in the directory where COW
## is running.  When the file is read, the parameters it contains will be
## processed and then the file will be deleted.
## Dynamic options list:
## STOP_AFTER=MAX     Do at most MAX more interations and then stop.
##                    Use 0 to stop at the end of the current iteration.
## PARALLEL=OFF       Revert to non-parallel processing at the next iteration
## PARALLEL=<options> At the next iteration, change the canoe-parallel.sh
##                    options to <options>.  Quotes are optional.
## Note that spaces are not allowed around the = sign.
## Convenient cut and paste commands:
##     echo PARALLEL=-n 4 > COW_DYNAMIC_OPTIONS
##     echo STOP_AFTER=0 > COW_DYNAMIC_OPTIONS
##

echo 'cow.sh, NRC-CNRC, (c) 2004 - 2009, Her Majesty in Right of Canada'

#------------------------------------------------------------------------------
# Argument processing & checking
#------------------------------------------------------------------------------

error_exit() {
   echo -n "cow.sh fatal error: "
   for msg in "$@"; do
      echo $msg
   done
   echo "Use -h for help."
   exit 1
}

warn() {
   echo -n "cow.sh warning: "
   for msg in "$@"; do
      echo $msg
   done
}

# Verify that enough args remain on the command line
# syntax: one_arg_check <args needed> $# <arg name>
# Note that this function expects to be in a while/case structure for
# handling parameters, so that $# still includes the option itself.
# exits with error message if the check fails.
arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

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
      echo Moving existing file $__FILENAME to $__BACKUP_FILENAME
      mv $__FILENAME $__BACKUP_FILENAME
   fi
   unset __FILENAME __BACKUP_SUFFIX __BACKUP_FILENAME __SUFFIX
}

check_ttable_limit() {
   TTABLE_LIM=$1
   if [ ! $TTABLE_LIM -gt 0 ] ; then
      error_exit "Please set a ttable-limit for use with -filt."
   fi
}

# Make it easy to recover the command line from saved logs.
echo $0 $*
echo Starting on `date`
echo ""

# Command-line processing
while [ $# -gt 0 ]; do
   case "$1" in
   -v|-verbose)    VERBOSE="-v";;
   -d|-debug)      DEBUG=1;;
   -h|-help)       cat $0 | egrep '^##' | cut -c4-; exit 1;;
   -resume)        RESUME=1;;
   -path)          arg_check 1 $# $1; PATH="$2:$PATH"; shift;;
   -nbest-list-size) arg_check 1 $# $1; N="$2"; shift;;
   -f)             arg_check 1 $# $1; CFILE="$2"; shift;;
   -z)             COMPRESS_EXT=".gz";;
   -Z)             COMPRESS_EXT="";;
   -canoe)         arg_check 1 $# $1; CANOE="$2"; shift;;
   -canoe-options) arg_check 1 $# $1; CANOE_OPTS="$2"; shift;;
   -rescore-options) arg_check 1 $# $1; RESCORE_OPTS="$2"; shift;;
   -rescore-train) arg_check 1 $# $1; RTRAIN="$2"; shift;;
   -model)         arg_check 1 $# $1; MODEL="$2"; shift;;
   -filt)          FILTER=$1;;
   -ttable-limit)  arg_check 1 $# $1; TTABLE_LIMIT="$2"; shift;;
   -lb)            LOAD_BALANCING=1;;
   -no-lb)         LOAD_BALANCING=;;
   -filt-no-ttable-limit)   FILTER=$1;;
   -mad)           arg_check 1 $# $1; RANDOM_INIT="$2"; shift;;
   -e)             ESTOP="-e -r50";;
   -s)             arg_check 1 $# $1; SEED="$2"; shift;;
   -nr)            NR=1;;
   -floor)         arg_check 1 $# $1; FLOOR="$2"; shift;;
   -nofloor)       NOFLOOR=1;;
   -wacc)          arg_check 1 $# $1; WACC="$2"; shift;;
   -win)           arg_check 1 $# $1; WIN="$2"; shift;;
   -parallel:*)    PARALLEL=1; PARALLEL_OPTS="${1/-parallel:/}";;
   -parallel)      PARALLEL=1;;
   -workdir)       arg_check 1 $# $1; WORKDIR="$2"; shift;;
   -maxiter)       arg_check 1 $# $1; MAXITER="$2"; shift;;
   -micro)         arg_check 1 $# $1; MICRO="$2"; shift;;
   -microsm)       arg_check 1 $# $1; MICROSM="$2"; shift;;
   -micropar)      arg_check 1 $# $1; MICROPAR="$2"; shift;;
   -bleu)          TRAINING_TYPE="-bleu"; SCORE_MAIN=bleumain;;
   -per)           TRAINING_TYPE="-per"; SCORE_MAIN="wermain -per";;
   -wer)           TRAINING_TYPE="-wer" SCORE_MAIN=wermain;;
   --)             shift; break;;
   -*)             error_exit "Unknown parameter: $1.";;
   *)              break;;
   esac
   shift
done

if [ -n "$NOFLOOR" ] && [ $FLOOR -ge 0 ]; then
   error_exit "Error: You cannot use floor and nofloor at the same time";
fi
if [ -z "$NOFLOOR" ] && [ $FLOOR -lt 0 ]; then
   error_exit "Error: You must provide either floor or nofloor";
fi
if [ $FLOOR -ge 0 ]; then
   FLOOR_ARG="-f $FLOOR"
fi

if [ -z "$CFILE" ]; then
   CFILE=canoe.ini
fi

if [ -z "$MODEL" ]; then
   MODEL=$WORKDIR/$DEFAULT_MODEL
   if [ ! $RESUME ]; then
      rename_old $MODEL
   fi
fi

if [ $# -lt 1 ]; then
   error_exit "Error: Source and reference file not specified."
elif [ $# -lt 2 ]; then
   error_exit "Error: Reference file(s) not specified."
fi

if [ -n "$LOAD_BALANCING" ]; then
   PARALLEL_OPTS="-lb $PARALLEL_OPTS"
fi

if [ $DEBUG ]; then
   echo "
VERBOSE=$VERBOSE
FILTER=$FILTER
FLOOR=$FLOOR
NOFLOOR=$NOFLOOR
FLOOR_ARG=$FLOOR_ARG
RESUME=$RESUME
LOAD_BALANCING=$LOAD_BALANCING
N=$N
CFILE=$CFILE
CANOE=$CANOE
CANOE_OPTS=$CANOE_OPTS
RTRAIN=$RTRAIN
MODEL=$MODEL
FFVALPARSER=$FFVALPARSER
FFVALPARSER_OPTS=$FFVALPARSER_OPTS
PARALLEL=$PARALLEL
PARALLEL_OPTS=$PARALLEL_OPTS
WORKDIR=$WORKDIR"
fi

SFILE=$1
shift
if [ $# -eq 1 -a `cat $1 | wc -l` -gt `cat $SFILE | wc -l` ]; then
   warn "Old style combined RFILE supplied - uncombining it";
   RFILE=$1
   echo uncombine.pl `cat $SFILE | wc -l` $RFILE
   uncombine.pl `cat $SFILE | wc -l` $RFILE
   RVAL=$?
   if [ $RVAL -ne 0 ]; then
      error_exit "can't uncombine ref file"
   fi
   RFILES=`ls $RFILE.[0-9]* | tr '\n' ' '`
else
   RFILES=$*
fi
for x in $RFILES; do
   if [ `cat $x | wc -l` -ne `cat $SFILE | wc -l` ]; then
      error_exit "ref file $x has a different number of lines as source file $SFILE"
   fi
done

if [ $DEBUG ]; then
   echo "SFILE=$SFILE
RFILES=$RFILES"
fi

ORIGCFILE=$CFILE

TMPMODELFILE=$MODEL.tmp
TMPFILE=$WORKDIR/tmp
TRANSFILE=$WORKDIR/transfile
HISTFILE="rescore-results"
# code below assumes that $POWELLFILE* and $POWELLMICRO* are disjoint:
POWELLFILE="$WORKDIR/powellweights.tmp"
POWELLMICRO="$WORKDIR/powellweights.micro" 
MODEL_ORIG=$MODEL.orig

if [ ! $RESUME ]; then
   rename_old $HISTFILE
fi

# Check validity of command-line arguments
if [ $(($N)) -eq 0 ]; then
   error_exit "Bad n-best list size ($N)."
fi

if [ ! -x $CANOE ]; then
   if ! which-test.sh $CANOE; then
      error_exit "Executable canoe not found at $CANOE."
   fi
fi
if [ "$PARALLEL" == 1 -a "$CANOE" != canoe ]; then
   error_exit "cannot specify alternative decoder $CANOE with -parallel option."
fi
if [ ! -x $RTRAIN ]; then
   if ! which-test.sh $RTRAIN; then
      error_exit "Executable rescore train program not found at $RTRAIN."
   fi
fi
if [ -e $MODEL -a ! -w $MODEL ]; then
   error_exit "Model file $MODEL is not writable."
fi

if [ ! -r $CFILE ]; then
   error_exit "Cannot read config file $CFILE."
fi
if configtool check $CFILE; then true; else
   error_exit "problem with config file $CFILE."
fi

if [ $MICRO -gt 0 ]; then
   if [ $RANDOM_INIT != 0 ]; then
      error_exit "-micro and -mad are not compatible."
   fi
fi

if [ ! -r $SFILE ]; then
   error_exit "Cannot read source file $SFILE."
fi
if [ ! -r $RFILE ]; then
   error_exit "Cannot read reference file $RFILE."
fi

if [ -z "$WORKDIR" ]; then
   error_exit "-workdir <dir> is a mandatory argument."
fi

if [ ! -d $WORKDIR ]; then
   error_exit "workdir $WORKDIR is not a directory."
fi

if [ -n "$MAXITER" ]; then
   if [ "`expr $MAXITER + 0 2> /dev/null`" != "$MAXITER" ]; then
      error_exit "max iter $MAXITER is not a valid number."
   elif [ "$MAXITER" -lt 1 ]; then
      error_exit "Max iter $MAXITER is less than 1: no work to do!"
   fi
fi

# Check paths of required programs
if ! which-test.sh $FFVALPARSER; then
   error_exit "$FFVALPARSER not found in path."
fi

# Handle verbose
if [ "$VERBOSE" = "-v" ]; then
   #CANOE="$CANOE -v 2"
   CANOE_OPTS="$CANOE_OPTS -v 1"
   RTRAIN="$RTRAIN -v"
fi

#------------------------------------------------------------------------------
# Build model file and filtered phrase tables
#------------------------------------------------------------------------------

export LC_ALL=C

if [ ! $RESUME ]; then
   for FILE in $WORKDIR/foo.* $WORKDIR/alltargets $WORKDIR/allffvals \
      $WORKDIR/$POWELLFILE* $WORKDIR/$POWELLMICRO*; do
      \rm -f $FILE
   done

   if [ ! -e $MODEL ]; then
      configtool rescore-model:$WORKDIR/allffvals $CFILE > $MODEL
      # For the random ranges
      MODEL_ORIG=$MODEL.orig
      cut -d' ' -f1 $MODEL > $MODEL_ORIG
   else
      if [ `cat $MODEL | wc -l` -ne `configtool nf $CFILE` ]; then
         error_exit "Bad model file"
      fi
      # For the random ranges
      cp $MODEL $MODEL_ORIG
   fi

   # initialize micro rescoring models if needed
   if [ $MICRO -gt 0 ]; then
      configtool rescore-model:.duplicateFree.ffvals$COMPRESS_EXT $CFILE > $MODEL.micro.in
      S=`wc -l < $SFILE`
      for i in `seq -w 0 9999 | head -$S`; do
         cp $MODEL.micro.in $MODEL.micro.$i
      done
   fi

   # Filter phrasetables to retain only matching source phrases.
   if [ "$FILTER" = "-filt" ] ; then
      # Filter-joint, apply -L limit
      if [ -z "$TTABLE_LIMIT" ]; then
          TTABLE_LIMIT=`configtool ttable-limit $CFILE`
      fi
      check_ttable_limit $TTABLE_LIMIT;
      echo "filter_models -z -r -f $CFILE -ttable-limit $TTABLE_LIMIT -suffix .FILT -soft-limit multi.probs.`basename ${SFILE}`.${TTABLE_LIMIT} < $SFILE"
            filter_models -z -r -f $CFILE -ttable-limit $TTABLE_LIMIT -suffix .FILT -soft-limit multi.probs.`basename ${SFILE}`.${TTABLE_LIMIT} < $SFILE
      CFILE=$CFILE.FILT
   elif [ "$FILTER" = "-filt-no-ttable-limit" ] ; then
      # filter-grep only, keep all translations for matching source phrases
      configtool rep-ttable-files-local:.FILT $CFILE > $CFILE.FILT
      echo "filter_models -f $CFILE -suffix .FILT < $SFILE"
            filter_models -f $CFILE -suffix .FILT < $SFILE
      CFILE=$CFILE.FILT
   else
      warn "Not filtering"
   fi
else
   echo Resuming where a previous iteration stopped - reusing existing $MODEL,
   echo "   " $HISTFILE, $POWELLFILE, and $WORKDIR/foo.\* files.

   if [ "$FILTER" = "-filt" ]; then
      CFILE=$CFILE.FILT
   fi
fi

# Find the best (max bleu) line in rescore-results and generate canoe.ini.cow
# and canoe.ini.FILT.cow from its weights. Called just before termination.
write_models() {
   echo configtool -p set-weights:$HISTFILE $CFILE $CFILE.cow
   configtool -p set-weights:$HISTFILE $CFILE $CFILE.cow
   RC=$?
   if (( $RC != 0 )); then
      \rm -f $CFILE.cow
      error_exit "configtool had non-zero return code: $RC."
   fi

   if [ "$CFILE" != "$ORIGCFILE" ]; then
      echo configtool -p set-weights:$HISTFILE $ORIGCFILE $ORIGCFILE.cow
      configtool -p set-weights:$HISTFILE $ORIGCFILE $ORIGCFILE.cow
      RC=$?
      if (( $RC != 0 )); then
         \rm -f $ORIGCFILE.cow
         error_exit "configtool had non-zero return code: $RC."
      fi
   fi
}

#------------------------------------------------------------------------------
# Main loop
#------------------------------------------------------------------------------

if [ $NR == 0 ]; then    # re-randomize on each iter
   SEED=$((SEED*10000))
fi

RANDOM_WEIGHTS=
if [ $RANDOM_INIT != 0 ]; then RANDOM_WEIGHTS="-random-weights -seed $RANDOM_INIT"; fi

ITER=0

while [ 1 ]; do

   # convert rescoring model weights to canoe weights
   if [ $DEBUG ]; then echo "configtool arg-weights:$MODEL $CFILE" ; fi
   wtvec=`configtool arg-weights:$MODEL $CFILE`

   if [ $MICRO -gt 0 ]; then
      MICRO_WTS=$WORKDIR/micro-weights.$((ITER+1))
      cat /dev/null > $MICRO_WTS
      S=`wc -l < $SFILE`
      for i in `seq -w 0 9999 | head -$S`; do
         configtool arg-weights:$MODEL.micro.$i $CFILE >> $MICRO_WTS
      done
   fi

   # Run decoder
   echo ""
   echo Running decoder on `date`
   RUNSTR="$CANOE $CANOE_OPTS $RANDOM_WEIGHTS -f $CFILE $wtvec -nbest $WORKDIR/foo$COMPRESS_EXT:$N -ffvals"
   if [ $MICRO -gt 0 ]; then RUNSTR="$RUNSTR -sent-weights $MICRO_WTS"; fi
   if [ "$PARALLEL" == 1 ]; then
      RUNSTR="$CANOE_PARALLEL $PARALLEL_OPTS $RUNSTR"
   fi
   echo "$RUNSTR < $SFILE > $TRANSFILE.ff"
   time  $RUNSTR < $SFILE > $TRANSFILE.ff

   # Check return value
   RVAL=$?
   if [ $RVAL -ne 0 ]; then
      error_exit "Decoder returned $RVAL";
   fi

   # From here on, use given weights (not random weights)
   RANDOM_WEIGHTS=

   $FFVALPARSER $FFVALPARSER_OPTS -in=$TRANSFILE.ff > $TRANSFILE
   echo `$SCORE_MAIN $TRANSFILE $RFILES | egrep score` " $wtvec" >> $HISTFILE

   # For debugging, also put the whole output of bleumain in the output
   # stream
   echo Current weight vector: $wtvec
   echo bleumain $TRANSFILE $RFILES
   bleumain $TRANSFILE $RFILES

   if [ -n "$MAXITER" ]; then
      MAXITER=$((MAXITER - 1))
   fi
   if [ $MICRO -gt 0 ]; then
      MICRO=$((MICRO - 1))
   fi

   # Read the dynamic options file for any updates to MAXITER or
   # PARALLEL_OPTS
   if [ -r COW_DYNAMIC_OPTIONS ]; then
      echo ""
      echo File COW_DYNAMIC_OPTIONS found, processing dynamic options.
      while read; do
         echo -n "   Opt $REPLY => "
         case $REPLY in
         PARALLEL=OFF)
            PARALLEL=""
            echo Turning parallel decoding off
            ;;
         PARALLEL=*)
            PARALLEL=1
            PARALLEL_OPTS="${REPLY#PARALLEL=}"
            PARALLEL_OPTS="${PARALLEL_OPTS//\"/}"
            echo Setting parallel option\(s\) to $PARALLEL_OPTS
            ;;
         STOP_AFTER=*)
            STOP_AFTER="${REPLY#STOP_AFTER=}"
            STOP_AFTER="${STOP_AFTER// /}"
            if [ "`expr $STOP_AFTER + 0 2> /dev/null`" != "$STOP_AFTER" ]
            then
               echo Ignoring non integer STOP_AFTER parameter $STOP_AFTER
            else
               MAXITER=$STOP_AFTER
               echo Doing $STOP_AFTER more iteration\(s\) and then stopping.
            fi
            ;;
         *)  Ignoring unknown dynamic option: "$REPLY"
         esac
      done < COW_DYNAMIC_OPTIONS
      \rm COW_DYNAMIC_OPTIONS
   fi

   new=

   if [ -n "$MAXITER" ]; then
      if [ $MAXITER -lt 1 ]; then
         echo
         echo Reached maximum number of iterations.  Stopping.
         write_models
         echo Done on `date`
         exit
      else
         echo
         echo $MAXITER iteration\(s\) remaining.
      fi
   fi

   # Create all N-best lists and check if any of them have anything new to add
   echo ""
   echo Producing n-best lists on `date`

   FOO_FILES=$WORKDIR/foo.????."$N"best$COMPRESS_EXT
   totalPrevK=0
   totalNewK=0
   for x in $FOO_FILES; do
      #echo Append-uniq $x on `date`
      x=${x%$COMPRESS_EXT}
      f=${x%."$N"best}

      # We use gzip in case the user requested compressed foo files
      touch $f.duplicateFree$COMPRESS_EXT $f.duplicateFree.ffvals$COMPRESS_EXT
      prevK=`gzip -cqfd $f.duplicateFree$COMPRESS_EXT | wc -l`
      totalPrevK=$((totalPrevK + prevK))
      append-uniq.pl -nbest=$f.duplicateFree$COMPRESS_EXT -addnbest=$x$COMPRESS_EXT \
         -ffvals=$f.duplicateFree.ffvals$COMPRESS_EXT -addffvals=$x.ffvals$COMPRESS_EXT

      # Check return value
      RVAL=$?
      if [[ $RVAL -ne 0 ]]; then
         error_exit "append-uniq.pl returned $RVAL"
      fi

      newK=`gzip -cqfd $f.duplicateFree$COMPRESS_EXT | wc -l`
      totalNewK=$((totalNewK + newK))

      # Check if there was anything new
      if [ $prevK -ne $newK ]; then
         new=1
      fi

      echo -n ".";
   done
   echo
   echo "Total size of n-best list -- previous: $totalPrevK; current: $totalNewK."
   echo

   # If nothing new, then we're done, unless we just resumed or we were running
   # in micro mode, in which case we do at least one iteration.
   if [ -z "$new" ]; then
      echo
      echo No new sentences in the N-best lists.
      if [ $RESUME ]; then
         echo -n But this is the first iteration after a resume, ""
         echo so running rescore_train anyway.
         echo
         MICRO=0                # needs to be turned off
      elif [ $MICRO -gt 0 ]; then
         echo "So switching out of micro mode"
         MICRO=0
      else
         write_models
         echo Done on `date`
         exit
      fi
   fi

   echo Preparing to run rescore_train on `date`

   \rm $WORKDIR/alltargets $WORKDIR/allffvals >& /dev/null
   S=$((`wc -l < $SFILE`))
   for((n=0;n<$S;++n))
   {
      m=`printf "%4.4d" $n`
      # We use gzip in case the user requested compress foo files
      gzip -cqfd $WORKDIR/foo.${m}.duplicateFree$COMPRESS_EXT        | perl -pe "s/^/$n\t/"  >> $WORKDIR/alltargets
      gzip -cqfd $WORKDIR/foo.${m}.duplicateFree.ffvals$COMPRESS_EXT | perl -pe "s/^/$n\t/"  >> $WORKDIR/allffvals
   }

   # Find the parameters that optimize the BLEU score over the given set of all targets
   echo Running rescore_train on `date`

   if [ $RESUME ]; then
      if [ `ls $POWELLFILE.*|wc -l` -ge 1 ]; then
         WEIGHTINFILE=`ls -tr1 $POWELLFILE.* | tail -1`
         ITER=`echo $WEIGHTINFILE | cut -d"." -f3`
         ITER_CHECK=`wc -l < $HISTFILE`
         echo "$ITER / $ITER_CHECK iterations have been done already."
         if [ $ITER -gt $ITER_CHECK ]; then
            echo "Inconsistency between number of iterations according to $WEIGHTINFILE and $HISTFILE:"
            echo "   $ITER vs. $ITER_CHECK iterations have been done already."
            if [ -e $POWELLFILE.$ITER_CHECK ]; then
               ITER=$ITER_CHECK
            else
               echo "   File $POWELLFILE.$ITER_CHECK does not exist => stick with $WEIGHTINFILE and"
            fi
            echo "   assume that $ITER have been done"
         fi
         MAXITER=$((MAXITER - $ITER))
         if [ $MICRO -gt $ITER ]; then
            MICRO=$((MICRO - $ITER))
         else
            MICRO=0
         fi
         ITER=$((ITER + 1))
         WEIGHTOUTFILE=$POWELLFILE.$ITER
         echo "Resuming from $WEIGHTINFILE"
         echo "   ==> $MAXITER iteration\(s\) remaining."
      fi
   else
      ITERP=$ITER
      ITER=$((ITER + 1))
      WEIGHTINFILE=$POWELLFILE.$ITERP
      WEIGHTOUTFILE=$POWELLFILE.$ITER
      WINTMP=$WIN
      if [ $WACC -ne 1 ]; then  # accumulate wts from WACC prev iters
         WEIGHTINFILE=$WORKDIR/wacc-wts
         cat /dev/null > $WEIGHTINFILE
         WACCI=0
         while [ $ITERP -gt 0 ] && [ $WACCI -lt $WACC ]; do
            head -$WIN $POWELLFILE.$ITERP >> $WEIGHTINFILE
            ITERP=$((ITERP - 1))
            WACCI=$((WACCI + 1))
         done
         WINTMP=`wc -l < $WEIGHTINFILE`
   fi
   touch $WEIGHTINFILE   # create powellweights.tmp.0 if nec

   # ensure powellweights.micro.IIII files exist on first iteration
   if [ $MICRO -ne 0 ]; then    
      S=`wc -l < $SFILE`
      for i in `seq -w 0 9999 | head -$S`; do
         touch $POWELLMICRO.$i
      done
   fi


   if [ -e $WEIGHTOUTFILE ]; then
      rename_old $WEIGHTOUTFILE
   fi

   
   if [ $MICRO -eq 0 ]; then
      RUNSTR="$RTRAIN $TRAINING_TYPE $RESCORE_OPTS \
         $ESTOP -s $SEED -wi $WEIGHTINFILE -wo $WEIGHTOUTFILE \
         -win $WINTMP \
         -dyn -n $FLOOR_ARG \
         $MODEL_ORIG $TMPMODELFILE $SFILE $WORKDIR/alltargets $RFILES"
   else
      WTS="$POWELLMICRO.IIII"
      RT_OPTS="$RESCORE_OPTS $ESTOP -s $SEED -wi $WTS -wo $WTS -n -sm $MICROSM \
         $FLOOR_ARG -p $WORKDIR/foo.IIII"
      RUNSTR="rescore-train-micro.sh $VERBOSE -n $MICROPAR -rt-opts \"$RT_OPTS\" \
         $MODEL.micro.in $MODEL.micro.IIII $SFILE \
         $WORKDIR/foo.IIII.duplicateFree$COMPRESS_EXT $RFILES"
   fi

   echo "$RUNSTR"
   time eval $RUNSTR

   # Check return value
   RVAL=$?
   if [ $RVAL -ne 0 ]; then
      error_exit "rescore_train returned $RVAL"
   fi

   if [ $MICRO -eq 0 ]; then
      # If the new model file is identical to the old one, we're done, since the
      # next iteration will produce exactly the same output as the last one.
      if diff -q $MODEL $TMPMODELFILE; then
         # We repeat the last line in rescore-results, since that tells the
         # experimenter that a stable point has been reached.
         echo `$SCORE_MAIN $TRANSFILE $RFILES | egrep score` " $wtvec" >> $HISTFILE
         echo
         echo New model identical to previous one.
         write_models
         echo Done on `date`
         exit
      fi
      # Replace the old model with the new one
      mv $TMPMODELFILE $MODEL
   fi
   RESUME=

   if [ $NR == 0 ]; then  # re-randomize on each iter
      SEED=$((SEED+1))
   fi

done
