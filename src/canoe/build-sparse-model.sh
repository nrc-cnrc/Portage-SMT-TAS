#!/bin/bash

# @file build-sparse-model.sh
# @brief Produce the sparse model file from a feature specification input, and
#        generate any necessary data files
#
# @author Eric Joanis, based on George Foster's original work.
#
# This script is splicing the body out of George Foster's
# portage-framelab/models/sparse/default/build-model, so it can be factored out
# and used in both frameworks.
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada


# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG [options] -model M -cmdfile CF -config F -srclang SL -tgtlang TL

  Produce model file M from feature specifications in F, and generate any
  necessary data files.

  The config file F should contain the list of features to include in model
  See palminer -H for available features. You can either specify the feature
  arguments directly or use the 'auto' keyword to generate one of the following
  kinds of data files:

    auto:svoc:C:N - top N source vocab from corpus C
    auto:tvoc:C:N - top N target vocab from corpus C
    auto:svoci:C:N - top N source vocab from corpus C, indexed
    auto:tvoci:C:N - top N target vocab from corpus C, indexed
    auto:sunal:J:N - top N unaligned source words from jpt directory J
    auto:tunal:J:N - top N unaligned target words from jpt directory J
    auto:mkcls:F:M:I - M mkcls clusters from file F, using I iterations
    auto:df1:C - doc freqs of tgt words in corpus C from pseudo_docfreqs
    auto:df2:C:N - doc freqs of tgt words in corpus C from pseudo_docfreqs -d N
    auto:bins11 - 11 freq bins, with upper bounds: 1,2,4,8,16,32,64,128,1k,10k,inf

  Above, corpus C should be a prefix for a corpus consisting of parallel files
  C_sl.sext and C_tl.text, e.g., C_en.tok and C_fr.tok, whereas file F is a
  full file name (possibly using substitution tokens, see below).

  Substitution tokens:

    If <corp>, <srclang>, <tgtlang>, <srcext>, or <tgtext> occur in
    feature specifications, they will be substituted with the values provided
    using the corresponding command-line options.

  Examples:

    [AlignedWordPair] auto:svoc:<corp>:80 auto:tvoc:<corp>:80
    [LexUnalSrcAny] auto:sunal:../../jpt/baseline:50
    [LexUnalTgtAny] auto:tunal:../../jpt/baseline:50
    [DistCurrentSrcFirstWord] auto:mkcls:<corp>_<srclang>.<srcext>:20:2
    [DistCurrentTgtFirstWord] auto:mkcls:<corp>_<tgtlang>.<tgtext>:20:2
    [DistCurrentSrcFirstWord] auto:svoci:<corp>:80
    [DistCurrentTgtFirstWord] auto:tvoci:<corp>:80
    [AvgTgtWordScoreBin] auto:df1:<corp> 10 log
    [AvgTgtWordScoreBin] auto:df2:<corp>:100 10 lin
  The config F should contain one feature specification per line, among those
  output by palminer -H, or automatically generated using the following auto
  specifications:


Options:

  -model M     name of the output model [model]
  -cmdfile CF  name of the output command file [cmds]
  -config F    name of the config file with the feature specifications [STDIN]
  -corp CORP   substitute <corp> by CORP in F; CORP should be the corpus
               basename, e.g, for dir/train_{en,fr}.tok, use -corp dir/train.
               Note: if you need multiple corpora, hard-code them into F
               instead of using <corp>.
  -srclang SL  two-letter code for the source language
  -tgtlang TL  two-letter code for the target language
  -srcext SEXT extension for files in the source language [tok]
  -tgtext TEXT extension for files in the target language [tok]
  -h(elp)      print this help message
  -v(erbose)   increment the verbosity level by 1 (may be repeated)

==EOF==

   exit 1
}

echo $0 "$*" >&2

mfile=model
cmdfile=cmds
config=-
corpus=
srclang=
tgtlang=
srcext=tok
tgtext=tok
VERBOSE=0
while [[ $# -gt 0 ]]; do
   case "$1" in
   -model)              arg_check 1 $# $1; mfile=$2; shift;;
   -cmdfile)            arg_check 1 $# $1; cmdfile=$2; shift;;
   -config)             arg_check 1 $# $1; config=$2; shift;;
   -corp)               arg_check 1 $# $1; corpus="$2"; shift;;
   -srclang)            arg_check 1 $# $1; srclang=$2; shift;;
   -tgtlang)            arg_check 1 $# $1; tgtlang=$2; shift;;
   -srcext)             arg_check 1 $# $1; srcext=$2; shift;;
   -tgtext)             arg_check 1 $# $1; tgtext=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done
test $# -gt 0   && error_exit "Superfluous arguments $*"

if [[ $DEBUG ]]; then
   echo
   for v in mfile cmdfile config corpus srclang tgtlang srcext tgtext; do
      echo -n "$v = "
      eval "echo \"\$$v\""
   done
fi >&2

# This argCheck is redundant with sh_utils.sh's, but not identical, and used
# extensively below.
argCheck() # (cmdname, num_required, num_given+1)
{
   nargs=$(($3-1))
   if [[ $2 != $nargs ]]; then
      error_exit "expecting $2 arguments for $1 command - got $nargs"
   fi
}

# Generate model file and build-model.cmds

cmdfile_tmp=$(mktemp tmp.`basename $cmdfile`.XXX)
[[ -r $cmdfile_tmp ]] || error_exit "Error making temporary command file"
cat /dev/null > $cmdfile_tmp || error_exit "Can't write $cmdfile_tmp"
cat /dev/null > $mfile || error_exit "Can't write $mfile"
features=`cat -- $config |
   perl -ne 'print unless /^ *#/' |
   sed \
      -e "s#<corp>#$corpus#g" \
      -e "s/<srclang>/$srclang/g" \
      -e "s/<tgtlang>/$tgtlang/g" \
      -e "s/<srcext>/$srcext/g" \
      -e "s/<tgtext>/$tgtext/g" \
   `
PF="set -o pipefail;"
for c in $features; do
   #[[ $DEBUG ]] && echo "Processing feature token $c" >&2
   case $c in
      \[*) echo -ne "\n$c" >> $mfile ;;  # [feature]
      auto:*)                            # auto:xxx command
         outfile=data.$(echo $c | perl -pe 's/^auto://; s/[:\/*]/_/g;')
         toks=($(echo $c | perl -pe s'/^auto://; s/:/ /g;')) # eg auto:tvoc:C:N -> (tvoc C N)
         echo -ne "\n$outfile" >> $mfile
         if [[ -s $outfile ]]; then continue; fi
         case $c in
            auto:svoci*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               corp=${toks[1]}
               nw=${toks[2]}
               [[ $srclang ]] || error_exit "Please specify -srclang when using auto:svoci* features"
               echo "get_voc -sc ${corp}_${srclang}.${srcext} | head -$nw | cut -d' ' -f1 |\
                  perl -n -e 'BEGIN {\$i=2}' -e 'chomp; print \$_,\"\t\",\$i++,\"\n\"' > $outfile" \
                  >> $cmdfile_tmp
               ;;
            auto:tvoci*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               corp=${toks[1]}
               nw=${toks[2]}
               [[ $tgtlang ]] || error_exit "Please specify -tgtlang when using auto:tvoci* features"
               echo "get_voc -sc ${corp}_${tgtlang}.${tgtext} | head -$nw | cut -d' ' -f1 |\
                  perl -n -e 'BEGIN {\$i=2}' -e 'chomp; print \$_,\"\t\",\$i++,\"\n\"' > $outfile" \
                  >> $cmdfile_tmp
               ;;
            auto:svoc*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               corp=${toks[1]}
               nw=${toks[2]}
               [[ $srclang ]] || error_exit "Please specify -srclang when using auto:svoc* features"
               echo "get_voc -sc ${corp}_${srclang}.${srcext} | head -$nw | cut -d' ' -f1 > $outfile" >> $cmdfile_tmp
               ;;
            auto:tvoc*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               corp=${toks[1]}
               nw=${toks[2]}
               [[ $tgtlang ]] || error_exit "Please specify -tgtlang when using auto:tvoc* features"
               echo "get_voc -sc ${corp}_${tgtlang}.${tgtext} | head -$nw | cut -d' ' -f1 > $outfile" >> $cmdfile_tmp
               ;;
            auto:sunal*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               jptdir=${toks[1]}
               nw=${toks[2]}
               # Note: '{ sort -nr -k2,2; true; }' is done on purpose to avoid broken pipe errors.
               echo "$PF count_unal_words -l 1 $jptdir/jpt.* | { sort -nr -k2,2; true; } | head -$nw | cut -d' ' -f1 > $outfile" >> $cmdfile_tmp
               ;;
            auto:tunal*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               jptdir=${toks[1]}
               nw=${toks[2]}
               # Note: '{ sort -nr -k2,2; true; }' is done on purpose to avoid broken pipe errors.
               echo "$PF count_unal_words -l 2 $jptdir/jpt.* | { sort -nr -k2,2; true; } | head -$nw | cut -d' ' -f1 > $outfile" >> $cmdfile_tmp
               ;;
            auto:mkcls*)
               argCheck ${toks[0]} 3 ${#toks[*]}
               file=${toks[1]}
               nclust=${toks[2]}
               niters=${toks[3]}
               echo -n ".mmcls" >> $mfile
               echo "mkcls -c$nclust -n$niters -V$outfile -p<(zcat -f $file) opt && wordClasses2MMmap $outfile $outfile.mmcls" >> $cmdfile_tmp
               ;;
            auto:df1*)
               argCheck ${toks[0]} 1 ${#toks[*]}
               corp=${toks[1]}
               [[ $tgtlang ]] || error_exit "Please specify -tgtlang when using auto:df1* features"
               echo "$PF cat ${corp}_${tgtlang}.${tgtext} | pseudo_docfreqs > $outfile" >> $cmdfile_tmp
               ;;
            auto:df2*)
               argCheck ${toks[0]} 2 ${#toks[*]}
               corp=${toks[1]}
               d=${toks[2]}
               [[ $tgtlang ]] || error_exit "Please specify -tgtlang when using auto:df2* features"
               echo "$PF cat ${corp}_${tgtlang}.${tgtext} | pseudo_docfreqs -d$d > $outfile" >> $cmdfile_tmp
               ;;
            auto:bins11)  # this is ridiculous
               argCheck ${toks[0]} 0 ${#toks[*]}
               echo "echo -e '1 1\n2 2\n3 4\n5 8\n9 16\n17 32\n33 64\n65 128\n129 1000\n1001 10000\n10001 0' > $outfile" >> $cmdfile_tmp
               ;;
            # ADD OTHER auto:xxx COMMANDS HERE
            auto:*)
               error_exit "unknown 'auto' command: $c" ;;
         esac
         ;;
      *) echo -n " $c" >> $mfile ;;      # plain argument
   esac
done

echo "" >> $mfile
sort -u $cmdfile_tmp > $cmdfile
[[ $DEBUG ]] || rm $cmdfile_tmp
