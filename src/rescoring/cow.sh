#!/bin/bash
# $Id$
#
# canoe-optimize-weight, a.k.a., cow.sh : optimizer for canoe's weights
# renamed from rescoreloop.sh
#
# Aaron Tikuisis / George Foster / Eric Joanis
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004-2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004-2007, Her Majesty in Right of Canada

VERBOSE=
FILTER=
RANDOM_INIT=0
RESUME=
FLOOR=0
N=200
CFILE=canoe.ini
CANOE=canoe
CANOE_PARALLEL=canoe-parallel.sh
PARALLEL=
RTRAIN=rescore_train
DEFAULT_MODEL=curmodel

FFVALPARSER=nbest2rescore.pl
FFVALPARSER_OPTS="-canoe"

##
## Usage: cow.sh [-v] [-resume] [-nbest-list-size N] [-maxiter MAX]
##        [-filt] [-floor index] [-model MODEL]  [-mad SEED] [-path P]
##        [-f CFILE] [-canoe-options CANOE_OPTS] [-parallel[:"PARALLEL_OPTS"]]
##        -workdir WORKDIR SFILE RFILE1 RFILE2 .. RFILEn
##
## cow.sh: Canoe Optimize Weights - formerly known as rescoreloop.sh
##
## Does "outer-loop" learning of decoder parameters.  That is, it iteratively
## runs the canoe decoder to generate n-best lists with feature function
## output, then finds weights to optimize the BLEU score on the given n-best
## lists.  This continues until no new sentences are produced at the decoder
## stage. The results (BLEU scores and weights) are appended to the file
## "rescore-results" after each iteration.
##
## Options:
## -v       Verbose output from Canoe (verbose level 1) and rescore_train.
## -resume  Resume a previously interupted run.  Will resume at the decoding
##          phase, regardless of where it had previous been interupted.  Results
##          are not guaranteed to be the same as if a fresh restart was done.
##          [Fresh start: delete any existing n-best lists, rename any existing
##           model and results files]
## -nbest-list-size The size of the n-best lists to create.  [200]
## -maxiter Do at most MAX iterations.
## -filt    Filter the phrase tables based on SFILE for faster operation.
##          -filt does soft filtering: creates smaller phrase tables (forward &
##          backward) containing ALL entries that match SFILE, and uses them
##          afterwards in place of the original tables.
## -floor   Index of parameter at which to start zero-flooring.
##          NB, this is NOT the floor threshold. [0].
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
## -path    Prepend path P to existing path for locating programs []
## -f       The configuration file to pass to the decoder. [canoe.ini]
## -canoe-options  Provide additional options to the decoder. []
## -parallel or -parallel:"PARALLEL_OPTIONS":  Use canoe-parallel.sh to
##          parallelize decoding.  Any option to canoe-parallel.sh, such as
##          -n <N> if you want to specify the number of parallel jobs to
##          use, -nolowpri to get more memory, -highmem to really get more
##          memory, should be specified after the : and must be in quotes,
##          e.g.:   -parallel:"-n 4".
##                  -parallel:"-n 4 -nolowpri".
##                  -parallel:"-n 4 -highmem".
##
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

echo 'cow.sh, NRC-CNRC, (c) 2004 - 2007, Her Majesty in Right of Canada'

#------------------------------------------------------------------------------
# Argument processing & checking
#------------------------------------------------------------------------------

error_exit() {
    for msg in "$@"; do
        echo $msg
    done
    echo "Use -h for help."
    exit 1
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
    -canoe)         arg_check 1 $# $1; CANOE="$2"; shift;;
    -canoe-options) arg_check 1 $# $1; CANOE_OPTS="$2"; shift;;
    -rescore-train) arg_check 1 $# $1; RTRAIN="$2"; shift;;
    -model)         arg_check 1 $# $1; MODEL="$2"; shift;;
    -filt)          FILTER=$1;;
    -mad)           arg_check 1 $# $1; RANDOM_INIT="$2"; shift;;
    -floor)         arg_check 1 $# $1; FLOOR="$2"; shift;;
    -parallel:*)    PARALLEL=1; PARALLEL_OPTS="${1/-parallel:/}";;
    -parallel)      PARALLEL=1;;
    -workdir)       arg_check 1 $# $1; WORKDIR="$2"; shift;;
    -maxiter)       arg_check 1 $# $1; MAXITER="$2"; shift;;
    --)             shift; break;;
    -*)             error_exit "Unknown parameter: $1.";;
    *)              break;;
    esac
    shift
done


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

if [ $DEBUG ]; then
    echo "
VERBOSE=$VERBOSE
FILTER=$FILTER
FLOOR=$FLOOR
RESUME=$RESUME
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
    echo "Warning: Old style combined RFILE supplied - uncombining it";
    RFILE=$1
    echo uncombine.pl `cat $SFILE | wc -l` $RFILE
    uncombine.pl `cat $SFILE | wc -l` $RFILE
    RVAL=$?
    if [ $RVAL -ne 0 ]; then
        echo "Error: can't uncombine ref file";
        exit 1
    fi
    RFILES=`ls $RFILE.[0-9]* | tr '\n' ' '`
else
    RFILES=$*
fi
for x in $RFILES; do
    if [ `cat $x | wc -l` -ne `cat $SFILE | wc -l` ]; then
        echo "Error: ref file $x has a different number of lines" \
             "as source file $SFILE"
        exit 1
    fi
done

if [ $DEBUG ]; then
    echo "SFILE=$SFILE
RFILES=$RFILES"
fi

TMPMODELFILE=$MODEL.tmp
TMPFILE=$WORKDIR/tmp
TRANSFILE=$WORKDIR/transfile
HISTFILE="rescore-results"
POWELLFILE="powellweights.tmp"
ORIGCFILE=$CFILE

if [ ! $RESUME ]; then
    rename_old $HISTFILE
fi

# Check validity of command-line arguments
if [ $(($N)) -eq 0 ]; then
    echo "Error: Bad n-best list size ($N).  Use -h for help."
    exit 1
fi

if [ ! -x $CANOE ]; then
  if ! which-test.sh $CANOE; then
    echo "Error: Executable canoe not found at $CANOE.  Use -h for help."
    exit 1
  fi
fi
if [ "$PARALLEL" == 1 -a "$CANOE" != canoe ]; then
    echo "Error: cannot specify alternative decoder $CANOE with -parallel option."
    echo "Use -h for help."
    exit 1
fi
if [ ! -x $RTRAIN ]; then
  if ! which-test.sh $RTRAIN; then
    echo "Error: Executable rescore train program not found at $RTRAIN.  Use -h for help."
    exit 1
  fi
fi
if [ -e $MODEL -a ! -w $MODEL ]; then
    echo "Error: Model file $MODEL is not writable.  Use -h for help."
    exit 1
fi

if [ ! -r $CFILE ]; then
    echo "Error: Cannot read config file $CFILE.  Use -h for help."
    exit 1
fi

if configtool check $CFILE; then true; else
    echo "Error: problem with config file $CFILE.  Use -h for help."
    exit 1
fi

if [ ! -r $SFILE ]; then
    echo "Error: Cannot read source file $SFILE.  Use -h for help."
    exit 1
fi
if [ ! -r $RFILE ]; then
    echo "Error: Cannot read reference file $RFILE.  Use -h for help."
    exit 1
fi

if [ -z "$WORKDIR" ]; then
    echo "Error: -workdir <dir> is now a mandatory argument, and should ideally
    point to a directory on un-backed up medium, like exp or scratch_iit, or a
    soft link to such a directory.  Ask Eric or Patrick for an explanation of
    why you have to do this."
    exit 1
fi

if [ ! -d $WORKDIR ]; then
    echo "Error: workdir $WORKDIR is not a directory."
    exit 1
fi

if [ -n "$MAXITER" ]; then
    if [ "`expr $MAXITER + 0 2> /dev/null`" != "$MAXITER" ]; then
        echo "Error: max iter $MAXITER is not a valid number."
        exit 1
    elif [ "$MAXITER" -lt 1 ]; then
        echo "Max iter $MAXITER is less than 1: no work to do!"
        exit 1
    fi
fi


# Check paths of required programs
if ! which-test.sh $FFVALPARSER; then
    echo "Error: $FFVALPARSER not found in path."
    exit 1
fi

# Handle verbose
if [ "$VERBOSE" = "-v" ]; then
#    CANOE="$CANOE -v 2"
    CANOE_OPTS="$CANOE_OPTS -v 1"
    RTRAIN="$RTRAIN -v"
fi

#------------------------------------------------------------------------------
# Build model file and filtered phrase tables
#------------------------------------------------------------------------------

export LC_ALL=C

if [ ! $RESUME ]; then
    for FILE in $WORKDIR/foo.* $WORKDIR/alltargets $WORKDIR/allffvals; do
        \rm -f $FILE
    done

    if [ ! -e $MODEL ]; then
        configtool rescore-model:$WORKDIR/allffvals $CFILE > $MODEL
    else
      if [ `cat $MODEL | wc -l` -ne `configtool nf $CFILE` ]; then
          echo "Error: Bad model file"
          exit 1
      fi
    fi

    numTMText=`configtool nt-text $CFILE`

    # build filtered phrase tables
    if [ "$FILTER" = "-filt" ] ; then
        # soft filtering: keep all matching translations, and subsequently use
        # forward and backward tables
        configtool rep-ttable-files-local:.FILT $CFILE > $CFILE.FILT
        echo "filter_models -f $CFILE -suffix .FILT < $SFILE"
              filter_models -f $CFILE -suffix .FILT < $SFILE
        CFILE=$CFILE.FILT
    else
       echo "Warning: Not filtering"
    fi
elif [ "$FILTER" = "-filt" ]; then
   CFILE=$CFILE.FILT
fi

write_models() {
    echo configtool set-weights:$HISTFILE $CFILE $CFILE.cow
    configtool set-weights:$HISTFILE $CFILE $CFILE.cow
    if (( $? != 0 )); then
        \rm -f $CFILE.cow
        exit 1
    fi

    if [ "$CFILE" != "$ORIGCFILE" ]; then
        echo configtool set-weights:$HISTFILE $ORIGCFILE $ORIGCFILE.cow
        configtool set-weights:$HISTFILE $ORIGCFILE $ORIGCFILE.cow
        if (( $? != 0 )); then
            \rm -f $ORIGCFILE.cow
            exit 1
        fi
    fi
}

#------------------------------------------------------------------------------
# Main loop
#------------------------------------------------------------------------------

if [ $RESUME ]; then
    echo Resuming where a previous iteration stopped - reusing existing $MODEL,
    echo "   " $HISTFILE, $POWELLFILE, and $WORKDIR/foo.\* files.
fi

RANDOM_WEIGHTS=
if [ $RANDOM_INIT != 0 ]; then RANDOM_WEIGHTS="-random-weights -seed $RANDOM_INIT"; fi

ITER=0

while [ 1 ]; do

    if [ $DEBUG ]; then echo "configtool arg-weights:$MODEL $CFILE" ; fi
    wtvec=`configtool arg-weights:$MODEL $CFILE`

    # Run decoder
    echo ""
    echo Running decoder on `date`
    RUNSTR="$CANOE $CANOE_OPTS $RANDOM_WEIGHTS -f $CFILE $wtvec -nbest $WORKDIR/foo:$N -ffvals"
    if [ "$PARALLEL" == 1 ]; then
        RUNSTR="$CANOE_PARALLEL $PARALLEL_OPTS $RUNSTR"
    fi
    echo "$RUNSTR < $SFILE > $TRANSFILE.ff"
    time  $RUNSTR < $SFILE > $TRANSFILE.ff

    # Check return value
    RVAL=$?
    if [ $RVAL -ne 0 ]; then
        echo "Error: Decoder returned $RVAL";
        exit 1
    fi

    # From here on, use given weights (not random weights)
    RANDOM_WEIGHTS=

    $FFVALPARSER $FFVALPARSER_OPTS -in=$TRANSFILE.ff > $TRANSFILE
    echo `bleumain $TRANSFILE $RFILES | egrep BLEU` " $wtvec" >> $HISTFILE

    # For debugging, also put the whole output of bleumain in the output
    # stream
    echo Current weight vector: $wtvec
    echo bleumain $TRANSFILE $RFILES
    bleumain $TRANSFILE $RFILES

    if [ -n "$MAXITER" ]; then
        MAXITER=$((MAXITER - 1))
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
    echo "Producing n-best lists"

    FOO_FILES=$WORKDIR/foo.????."$N"best
    totalPrevK=0
    totalNewK=0
    for x in $FOO_FILES; do
        f=${x%."$N"best}

        touch $f.duplicateFree $f.duplicateFree.ffvals
        prevK=`wc -l < $f.duplicateFree`
        totalPrevK=$((totalPrevK + prevK))
        append-uniq.pl -nbest=$f.duplicateFree -addnbest=$x -ffvals=$f.duplicateFree.ffvals -addffvals=$x.ffvals
        newK=`wc -l < $f.duplicateFree`
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

    # If nothing new, then we're done, unless we just resumed, in which case we
    # do at least one iteration.
    if [ -z "$new" ]; then
        echo
        echo No new sentences in the N-best lists.
        if [ $RESUME ]; then
            echo -n But this is the first iteration after a resume, ""
            echo so running rescore_train anyway.
            echo
        else
            write_models
            echo Done on `date`
            exit
        fi
    fi

    echo "Preparing to run rescore_train"

    \rm $WORKDIR/alltargets $WORKDIR/allffvals >& /dev/null
    S=$((`wc -l < $SFILE`))
    for((n=0;n<$S;++n))
    {
      m=`printf "%4.4d" $n`
      perl -pe "s/^/$n\t/" < $WORKDIR/foo.${m}.duplicateFree        >> $WORKDIR/alltargets
      perl -pe "s/^/$n\t/" < $WORKDIR/foo.${m}.duplicateFree.ffvals >> $WORKDIR/allffvals
    }

    # Find the parameters that optimize the BLEU score over the given set of all targets
    echo Running rescore_train on `date`
    # RUNSTR="$RTRAIN -dyn -n -f $FLOOR $MODEL $TMPMODELFILE $SFILE $WORKDIR/alltargets $RFILES"

    if [ $RESUME ]; then
      if [ `ls pow*|wc -l` -ge 1 ]; then
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
         ITER=$((ITER + 1))
         WEIGHTOUTFILE=$POWELLFILE.$ITER
         echo "Resuming from $WEIGHTINFILE"
         echo "   ==> $MAXITER iteration\(s\) remaining."
      fi
    else
      WEIGHTINFILE=$POWELLFILE.$ITER
      ITER=$((ITER + 1))
      WEIGHTOUTFILE=$POWELLFILE.$ITER
    fi

    if [ -e $WEIGHTOUTFILE ]; then
      rename_old $WEIGHTOUTFILE
    fi
    if [ $ITER -ge 2 ]; then
        RUNSTR="$RTRAIN -wi $WEIGHTINFILE -wo $WEIGHTOUTFILE -dyn -n -f $FLOOR $MODEL $TMPMODELFILE $SFILE $WORKDIR/alltargets $RFILES"
    else
        RUNSTR="$RTRAIN -wo $WEIGHTOUTFILE -dyn -n -f $FLOOR $MODEL $TMPMODELFILE $SFILE $WORKDIR/alltargets $RFILES"
    fi
    echo "$RUNSTR"
    time $RUNSTR

    # Check return value
    RVAL=$?
    if [ $RVAL -ne 0 ]; then
      echo "Error: rescore_train returned $RVAL"
      exit 1
    fi

    # If the new model file is identical to the old one, we're done, since the
    # next iteration will produce exactly the same output as the last one.
    if diff -q $MODEL $TMPMODELFILE; then
        # We repeat the last line in rescore-results, since that tells the
        # experimenter that a stable point has been reached.
        echo `bleumain $TRANSFILE $RFILES | egrep BLEU` " $wtvec" >> $HISTFILE

        echo
        echo New model identical to previous one.
        write_models
        echo Done on `date`
        exit
    fi

    # Replace the old model with the new one
    mv $TMPMODELFILE $MODEL

    RESUME=

done
