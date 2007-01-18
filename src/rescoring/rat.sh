#!/bin/bash

# rat.sh
#
# PROGRAMMER: George Foster
#
# COMMENTS:
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

echo 'rat.sh, Copyright (c) 2005 - 2006, Sa Majeste la Reine du Chef du Canada / Her Majesty in Right of Canada'

usage() {
    for msg in "$@"; do
        echo $msg
    done
    cat <<==EOF==

rat.sh [cp-opts] MODE [-v][-f cfg][-K nb][-n jobs][-p pfx][-msrc MSRC] MODEL SRC [REFS]

Train or translate with a rescoring model: first generate nbest lists & ffvals
for a given source file, then generate external feature files required for
rescoring, then either train a model using reference texts, or use an existing
model to translate the source text. All steps can be run in parallel except the
final one.  Arguments are:

MODE     Required keyword, either 'train' or 'trans' to select training or
         translation mode. In training mode, one or more reference texts must
         be provided. In translation mode, the rescored translation is written
         to the file <pfx>rat (where <pfx> is the argument to -p).
MODEL    The rescoring model. Any features of the form FileFF:ff.FNAME[.ARGS]
         will cause feature FNAME to be calculated with args ARGS. Features of
         the form FileFF:ffvals,i refer to the ith column in an automatically-
         generated ffvals file (contents of columns depend on canoe.ini). If
         training, the final model is written back to the MODEL file. Run
         "rescore_train -H" for the list of supported features.
         MODEL.train will be written in train mode, and used in translate mode.
SRC      Source file, in standard format.
REFS     One or more reference translations, in standard format.

Options:

cp-opts  Options to canoe-parallel.sh - just the initial block, before the
         'canoe' keyword. (To set canoe parameters, use the local -f option.)
-v       Verbose output.
-f       Canoe's config file. [canoe.ini]
-K       Size of nbest lists. [1000]
-n       Number of parallel jobs for feature generation. This is independent
         of the number of jobs for translation, which is set by the -n
         parameter in cp-opts. [num features on cluster; 3 on local machines]
-p       Prefix for all intermediate files (single best, nbest, ffvals, feature
         files). If a directory, must already exist. [MSRC.]
-msrc    A version of the source file marked up with rule-based translations,
         used for canoe input but not for feature generation [SRC]

Note: if any intermediate files already exist, this script currently overwrites
them. To add extra features to an existing model, make sure you remove write
permission from existing feature files (pending a better solution!).
To add externally-generated features, eg FileFF:my-feature (NOT FileFF:ff.my-feature!),
call the associated feature files <pfx>my-feature, where pfx is the argument to the -p
option.

==EOF==

    exit 1
}

# error_exit "some error message" "optionally a second line of error message"
# will exit with an error status, printing the specified error message(s) on
# STDERR.
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

# Get the CPOPTS string before regular arg processing

WHOLEARGS="$*"

CPOPTS=""
MODE=
while [ $# -gt 0 ]; do
    case "$1" in
    -h|-help)     usage;;
    train)        MODE="train"; break;;
    trans)        MODE="trans"; break;;
    *)            CPOPTS="$CPOPTS$1 "
    esac
    shift
done

if [ "$MODE" == "" ]; then
    error_exit "Missing MODE keyword"
fi
shift

# Command line processing

DEBUG=
VERBOSE=0
DASHV=
CANOE_CONFIG="canoe.ini"
K=1000
N=0
PFX=
MSRC=

while [ $# -gt 0 ]; do
    case "$1" in
    -v|-verbose) VERBOSE=$(( $VERBOSE + 1 ));;
    -d|-debug)   DEBUG=1;;
    -h|-help)    usage;;

    -f)          arg_check 1 $# $1; CANOE_CONFIG=$2; shift;;
    -K)          arg_check 1 $# $1; K=$2; shift;;
    -n)          arg_check 1 $# $1; N=$2; shift;;
    -p)          arg_check 1 $# $1; PFX=$2; shift;;
    -msrc)       arg_check 1 $# $1; MSRC=$2; shift;;

    --)          shift; break;;
    -*)          error_exit "Unknown option $1.";;
    *)           break;;
    esac
    shift
done

if [ $# -lt 2 ]; then error_exit "Expected at least 2 arguments."; fi
MODEL=$1
if [ "$MODE" = "trans" ]; then
    # in translate mode, accept the model with or without the .w suffix
    MODEL=${MODEL%.w}
fi
# The trained model file need only be different from $MODEL if it contains /
if grep / $MODEL >& /dev/null; then
    TRAINED_MODEL=$MODEL.w
else
    TRAINED_MODEL=$MODEL
fi
SRC=$2
shift; shift

REFS=$*
if [ "$MODE" == "train" ] && (( $# == 0 )); then
    error_exit "Need reference texts for training."
fi

# In trans mode, run bleumain if references are passed.
#if [ "$MODE" == "trans" ] && (( $# != 0 )); then
#    error_exit "Too many arguments for translation mode."
#fi


# default values for some args

if [ "$MSRC" == "" ]; then MSRC=$SRC; fi
if [ "$PFX" == "" ]; then PFX="`basename $MSRC`."; fi
if (( $VERBOSE )); then DASHV="-v"; fi

# arg checking

if [ ! -r $CANOE_CONFIG ]; then
    error_exit "Error: CANOE_CONFIG file $CANOE_CONFIG is not readable."
fi
if ! configtool check $CANOE_CONFIG; then
    error_exit "Error: problem with config file $CANOE_CONFIG."
fi
if [ ! -e $MODEL ]; then
    error_exit "Error: Model file $MODEL does not exist."
fi
if [ "$MODE" == "train" ]; then
    if [ \( -e $TRAINED_MODEL -a ! -w $TRAINED_MODEL \) -o ! -w `dirname $TRAINED_MODEL` ]; then
        error_exit "Error: File $TRAINED_MODEL is not writable."
    fi
fi
if [ "$MODE" == "trans" ]; then
    if [ ! -e $TRAINED_MODEL ]; then
        error_exit "Error: Model file $TRAINED_MODEL does not exist."
    fi
fi
if [ ! -r $MSRC ]; then
    error_exit "Error: MSRC file $MSRC is not readable."
fi
if [ ! -r $SRC ]; then
    error_exit "Error: SRC file $SRC is not readable."
fi

for ref in $REFS ; do
    if [ ! -r $ref ]; then
        error_exit "Error: REFS file $ref is not readable."
    fi
done

if which-test.sh qsub; then
    CLUSTER=1
else
    CLUSTER=0
fi

# RUN

if (( $VERBOSE )); then
    echo $0 $WHOLEARGS
    echo Starting on `date`
    echo ""
fi

# 1. Run canoe to generate nbest lists and ffvals

if (( $VERBOSE )); then
    echo "Generating nbest lists:"
fi

if [ ! -e ${PFX}nbest -a ! -e ${PFX}nbest.gz ]; then
    set -o pipefail
    canoe-parallel.sh $CPOPTS \
        canoe -v $VERBOSE -f $CANOE_CONFIG -nbest ${PFX}nb:$K -ffvals -palign \
        < $MSRC | nbest2rescore.pl -canoe > ${PFX}1best

    if (( $? != 0 )); then
        echo "problems with canoe-parallel.sh - quitting!"
        exit 4
    fi
    set +o pipefail
    if [ `wc -l < $MSRC` -ne `wc -l < ${PFX}1best` ]; then
        echo "problems with canoe-parallel.sh - quitting!"
        exit 5
    fi
      

    ### concatenating separate N-best lists into one big file
    len=`wc -l < $MSRC`

    if [ -e ${PFX}ffvals ]; then
        echo "Ffvals file ${PFX}ffvals exists already! Overwriting it!"
        \rm ${PFX}ffvals
    fi
    if [ -e ${PFX}pal ]; then
        echo "Phrase alignment file ${PFX}pal exists already! Overwriting it!"
        \rm ${PFX}pal
    fi
    s=0;
    while [ $s -lt $len ]; do
        numstr=$s
        if [ $s -le 9 ]; then
            numstr=000$s
        elif [ $s -le 99 ]; then
            numstr=00$s
        elif [ $s -le 999 ]; then
            numstr=0$s
        fi
        cat ${PFX}nb.${numstr}.${K}best >> ${PFX}nbest
        cat ${PFX}nb.${numstr}.${K}best.ffvals >> ${PFX}ffvals
        cat ${PFX}nb.${numstr}.${K}best.pal >> ${PFX}pal

        \rm ${PFX}nb.${numstr}.${K}best
        \rm ${PFX}nb.${numstr}.${K}best.ffvals
        \rm ${PFX}nb.${numstr}.${K}best.pal

        s=$((s + 1));
    done

    gzip -f ${PFX}nbest
    gzip -f ${PFX}ffvals
    gzip -f ${PFX}pal

    NBEST=${PFX}nbest.gz
    PAL=${PFX}pal.gz

else
    echo "${PFX}nbest(.gz) exists - skipping nbest and ffvals generation"
    if [ -e ${PFX}nbest.gz ]; then
        NBEST=${PFX}nbest.gz
    else
        NBEST=${PFX}nbest
    fi
    if [ -e ${PFX}pal.gz ]; then
        PAL=${PFX}pal.gz
    else
        PAL=${PFX}pal
    fi

fi

# 2. Generate feature values for nbest lists

if (( $VERBOSE )); then
    echo "Generating feature files:"
    echo ""
fi

if [ "$N" == 1 ]; then
    gen-features-parallel.sh $DASHV -c $CANOE_CONFIG -a $PAL -p $PFX $MODEL $SRC $NBEST 2>&1
else
    if [ "$N" = 0 ]; then
        if [ "$CLUSTER" = 1 ]; then
            N=`gen-features-parallel.sh -n $MODEL $SRC $NBEST | wc -l`
        else
            N=3
        fi
    fi

    if (( $VERBOSE )); then DASHQ= ; else DASHQ=-q ; fi
    gen-features-parallel.sh -c $CANOE_CONFIG -a $PAL -n -p $PFX $MODEL $SRC $NBEST |
	run-parallel.sh $DASHQ - $N
	#perl -n -e 'print "set -o noclobber; $_";' |
fi

if (( $? != 0 )); then
    echo "problems with gen-features-parallel.sh - quitting!"
    exit 1
fi

# 3. Train or trans with the rescoring model

if (( $VERBOSE )); then
    echo ""
    if [ "$MODE" == "train" ]; then echo "Training rescoring model:"; fi
    if [ "$MODE" == "trans" ]; then echo "Translating with rescoring model:"; fi
    echo ""
fi

if [ "$MODE" == "train" ]; then
    if [ $TRAINED_MODEL != $MODEL ]; then
        perl -pe 's#/#_#g' < $MODEL > $TRAINED_MODEL
    fi
    if (( $VERBOSE )); then
        echo rescore_train $DASHV -n -p $PFX $TRAINED_MODEL $TRAINED_MODEL $SRC $NBEST $REFS
    fi
    rescore_train $DASHV -n -p $PFX $TRAINED_MODEL $TRAINED_MODEL $SRC $NBEST $REFS
else
    if (( $VERBOSE )); then
        echo rescore_translate $DASHV -p $PFX $TRAINED_MODEL $SRC ${NBEST} \> ${PFX}rat
    fi
    rescore_translate $DASHV -p $PFX $TRAINED_MODEL $SRC ${NBEST} > ${PFX}rat
fi

if (( $? != 0 )); then
    echo "problems with rescoring!"
    exit 1
elif (( $VERBOSE )); then
    echo "Completed on `date`"
fi

if [ $MODE = "trans" ]; then
    if [ -n "$REFS" ]; then
        echo ""
        echo "Evaluation of 1-best output from canoe:"
        if (( $VERBOSE )); then
            echo bleumain -c ${PFX}1best $REFS
        fi
        bleumain -c ${PFX}1best $REFS

        echo ""
        echo "Evaluation of rescoring output:"
        if (( $VERBOSE )); then
            echo bleumain -c ${PFX}rat $REFS
        fi
        bleumain -c ${PFX}rat $REFS
    fi
fi

