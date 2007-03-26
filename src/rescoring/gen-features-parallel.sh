#!/bin/bash
#
# gen-features-parallel.sh
#
# George Foster
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

VERBOSE=
NOEXEC=
PREF=""
ALIGFILE=""
CANOEFILE=""

MODEL=
SFILE=
NBEST=

set -o noclobber

##
## Usage: gen-features-parallel.sh [-v][-n][-p pref] [-a pal-file]
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
## -p  Generate feature files with prefix <pref>. [none]
## -a  Load phrase alignment from file <filename>. [none]
## -c  Get canoe weights from file <filename>. [none]
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

echo 'gen-feature-parallel.sh, NRC-CNRC, (c) 2006 - 2007, Her Majesty in Right of Canada' >&2

error_exit() {
    for msg in "$@"; do
        echo $msg >&2
    done
    echo "Use -h for help." >&2
    exit 1
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

# Command-line processing
while [ $# -gt 0 ]; do
    case "$1" in
    -v)        VERBOSE="-v";;
    -n)        NOEXEC="-n";;
    -h)        cat $0 | egrep '^##' | cut -c4-; exit 1;;
    -p)        arg_check 1 $# $1; PREF="$2"; shift;;
    -a)        arg_check 1 $# $1; ALIGFILE=$2; shift;;
    -c)        arg_check 1 $# $1; CANOEFILE=$2; shift;;
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

if [ ! -r $MODEL ]; then
    error_exit "Error: Cannot read MODEL file $MODEL."
fi
if [ ! -r $SFILE ]; then
    error_exit "Error: Cannot read SFILE file $SFILE."
fi
if [ ! -r $NBEST ]; then
    error_exit "Error: Cannot read NBEST file $NBEST."
fi

if which-test.sh qsub; then
    CLUSTER=1
else
    CLUSTER=0
fi


# Make it easy to recover the command line from saved logs.
if [ $VERBOSE ]; then
    echo $0 $*
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
if [ "$CANOEFILE" != "" ]; then
    CANOEWEIGHTFILE=_${PREF}_canoeweights.tmp
    if [ -e $CANOEWEIGHTFILE ]; then
	rename_old $CANOEWEIGHTFILE
    fi
    canoe2rescoreFile.sh $CANOEFILE > $CANOEWEIGHTFILE
fi


# Process the MODEL file and launch gen_feature_value jobs

I=1
while [ $I -le `cat $MODEL | wc -l` ]; do

   # parse line from MODEL

   LINE=`head -$I $MODEL | tail -1`
   FEATURE=`echo $LINE | perl -n -e '/FileFF:ff[.]([^. ]+)/o;print $1'`
   ARGS=`echo $LINE | perl -n -e '/FileFF:ff[.][^. ]+[.](\S+)/o;print $1'`
   FILE=`echo $LINE | perl -n -e '/FileFF:(ff[.]\S+)/o;print $1' | perl -p -e 's#/#_#g'`
   # FILE=`echo $PPP | perl -n -e 's#/#_#go;print'`
   
   if [ "$FEATURE" != "" ]; then

       if [ "$ARGS" == "" ]; then
	   ARGS="no-arg"
       fi

       FEAT=`echo $FEATURE | sed 's/^nbest//' | sed 's/Post//'`
       if [ "$FEATURE" != "$FEAT" ]; then
	    if [ "$CANOEFILE" == "" ]; then
		echo "Canoe weights file needed for calculation of posterior probabilties!" >&2
		echo "Cannot generate feature $FEATURE" >&2
		CMD=""
	    else
		# check whether feature needs phrase alignment file
		FEAT=`echo $FEATURE | sed 's/Phrase//' | sed 's/Src$//'`
		if [ "$FEATURE" != "$FEAT" ]; then
		    if [ "$ALIGFILE" == "" ]; then
			echo "Phrase alignment file needed for calculation of posterior probabilties!" >&2
			echo "Cannot generate feature $FEATURE" >&2
			CMD=""
		    else
                CMD="gen_feature_values -a $ALIGFILE -o $PREF$FILE.gz $VERBOSE $FEATURE $ARGS#${CANOEWEIGHTFILE}#${PREF} $SFILE $NBEST"
		    fi
		else
                CMD="gen_feature_values -o $PREF$FILE.gz $VERBOSE $FEATURE $ARGS#${CANOEWEIGHTFILE}#${PREF} $SFILE $NBEST"
		fi		
	    fi
	else
        CMD="gen_feature_values -o $PREF$FILE.gz $VERBOSE $FEATURE $ARGS $SFILE $NBEST"
       fi
       #echo $CMD       

       # noexec stuff 
       if [ $NOEXEC ] || [ $VERBOSE ]; then
           echo "$CMD"
           if [ $NOEXEC ]; then 
               I=$(($I+1))
	       continue; 
	   fi
       fi

       # do it
       if [ "$CMD" != "" ]; then
	    if [ "$CLUSTER" == 1 ]; then
		psub -e log.$FILE $CMD
	    else
            eval "$CMD"
	    fi
	fi

   fi
   I=$(($I+1))
done


