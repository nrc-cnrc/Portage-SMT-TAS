#!/bin/bash

# @file getcolumn.sh 
# @brief Retrieves the COLNUM-th column from the tab-delimited file FILE.
#
# @author Aaron Tikuisis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, Her Majesty in Right of Canada

##
## Usage: getcolumn.sh COLNUM [FILE]
##
##   Retrieves the COLNUM-th column from the tab-delimited file FILE.
##
## Options:
##   COLNUM The column number to retrieve.  If there are fewer than COLNUM columns
##          in any line of the file then an error message is output instead.
##   FILE   The file, or - for standard input.  [-]
##

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }


[[ $PORTAGE_INTERNAL_CALL ]] ||
print_nrc_copyright getcolumn.sh 2004
export PORTAGE_INTERNAL_CALL=1

if [ $# -lt 1 -o $# -gt 2 -o "$1" == "-h" ]; then
    cat $0 | grep "^##" | cut -c4-
    exit 1
fi

file=$2
filename=$file
TMPFILE=

# If using standard input, put it into a temporary file
if [ $# -lt 2 -o "$file" == "-" ]; then
    TMPFILE=`mktemp /tmp/$$.in.XXX` || error_exit "Cannot create temp file."
    filename="the input"
    while [ -e $TMPFILE ]; do
	$TMPFILE=$TMPFILE.
    done
    cat - > $TMPFILE
    file=$TMPFILE
fi

# Build the regular expression that matches everything before the $1-th column.
# This is an expansion of ([^\t]*\t){K}, where K = $1 - 1.  However, we don't use the
# compact form because we want to don't want additional ()-references.
n=$1
exp=^
while [ $n -gt 1 ]; do
    exp="$exp"[^\\t]*\\t
    n=$(($n - 1))
done

# Check that every line has the correct number of columns
if [ "`sed "/$exp/ d" $file | head -c 1`" != "" ]; then
    error_exit "Fewer than $1 columns in $filename."
fi

# Use sed to select only the correct column
sed s/"$exp"\\\([^\\t]*\\\).*\$/\\1/ $file

if [ "$TMPFILE" != "" ]; then
    # Delete the temporary file
    rm -f $TMPFILE;
fi
