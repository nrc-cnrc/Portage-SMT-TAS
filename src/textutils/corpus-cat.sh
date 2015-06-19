#!/bin/bash

# @file corpus-cat.sh 
# @brief Cat the files in a parallel corpus together to produce a "one-file" version.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

# Change the program name and year here
print_nrc_copyright corpus-cat.sh 2012

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: $0 corp-in corp-out

  Concatenate all files corp-in/*_lang.ext to corp-out/all_lang.ext, for each
  "_lang.ext" pattern in corp-in. Same for any corp-in/*.id files. Any files
  all* already in corp-out will not be overwritten. corp-out will be created if
  it doesn't exist. Concatenation of compressed (.gz) files is supported, as
  long as all files for a _lang.ext are either compressed or uncompressed, but
  not a mix of compressed and uncompressed.

  Example:

     corp-in contents:  a_en.lc a_en.rul a_fr.lc a.id 
                        b_en.lc b_en.rul b_fr.lc b.id

     corp-out contents: all_en.lc all_en.rul all_fr.lc all.id
  
Options:

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information
  -n NAME     Use NAME instead of 'all' for corp-out files.
  -f          write original basenames (eg, 'a', 'b', etc. in example above) 
              to NAME.fid in corp-out

==EOF==

   exit 1
}

name=all
dofid=

VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -n)   arg_check 1 $# $1; name=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -h|-help)            usage;;
   -f)                  dofid=y;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0   && error_exit "Missing corp-in argument"
corp_in=$1; shift
test $# -eq 0   && error_exit "Missing corp-out argument"
corp_out=$1; shift
test $# -gt 0   && error_exit "Superfluous arguments $*"

[[ -d $corp_out ]] || mkdir $corp_out || exit

sep="_"
dot="."

# list of file suffixes
suffs=$(\find $corp_in/ -xtype f -name \*$sep\* | egrep -v "[$dot]id(.gz)?\$" | rev | cut -d$sep -f1 | rev | sort -u)
read -a corpora <<< $(\find $corp_in/ -xtype f -name \*$sep\* | egrep -v "[$dot]id(.gz)?\$" | rev | cut -d$sep -f2- | rev | sort -u)

nfiles=0
for s in $suffs; do
   if [[ ! -e $corp_out/$name$sep$s ]]; then
      files=${corpora[@]/%/_$s}  # Add file extension to all corpora
      n=$(ls $files | wc -l)
      if [[ $nfiles == 0 ]]; then 
         nfiles=$n
      elif [[ $n != $nfiles ]]; then
         warn "Number of files with suffix $s inconsistent with previous suffix(es): $n versus $nfiles"
      fi
      cat ${files} > $corp_out/$name$sep$s
      [[ $VERBOSE -gt 0 ]] && echo "created $corp_out/$name$sep$s" >&2
   else
      [[ $VERBOSE -gt 0 ]] && echo "$corp_out/$name$sep$s already exists" >&2
   fi
done

if [[ ! -e $corp_out/$name.id && ! -e $corp_out/$name.id.gz ]]; then
   n_nogz=$(ls ${corpora[@]/%/.id} 2> /dev/null | wc -l)
   n_gz=$(ls ${corpora[@]/%/.id.gz} 2> /dev/null | wc -l)
   if [[ $n_nogz != 0 && $n_gz != 0 ]]; then
      files=${corpora[@]/%/.id*}  # Add file extension to all corpora
      cat_cmd="zcat -f"
      ext=".id"
   elif [[ $n_nogz != 0 ]]; then
      files=${corpora[@]/%/.id}  # Add file extension to all corpora
      cat_cmd="cat"
      ext=".id"
   else
      files=${corpora[@]/%/.id.gz}  # Add file extension to all corpora
      cat_cmd="cat"
      ext=".id.gz"
   fi
   n=$(ls $files | wc -l)
   if [[ $n != 0 ]]; then
      if [[ $nfiles != 0 && $n != $nfiles ]]; then
         warn "Number of files with suffix $ext inconsistent with previous suffix(es): $n versus $nfiles"
      fi
      $cat_cmd $files > $corp_out/$name$ext
      [[ $VERBOSE -gt 0 ]] && echo "created $corp_out/$name$ext" >&2
   fi
else
   [[ $VERBOSE -gt 0 ]] && echo "$corp_out/$name.id or $corp_out/$name.id.gz already exists" >&2
fi

if [[ -n $dofid ]]; then
   if [[ ! -e $corp_out/$name.fid ]]; then
      touch $corp_out/$name.fid
      set -- $suffs   # split suffs at $IFS char, assign results to $1, $2, etc (!)
      for f in ${corpora[@]}; do
         b=$(basename $f)
         zcat -f ${f}_$1 | perl -nle "print \"$b\"" >> $corp_out/$name.fid
      done
   else
      [[ $VERBOSE -gt 0 ]] && echo "$corp_out/$name.fid already exists" >&2
   fi
fi
