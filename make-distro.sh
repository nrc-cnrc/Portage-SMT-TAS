#!/bin/bash
# $Id$

# make-distro.sh - Make a CD or a copyable distro of PORTAGEshared.
# 
# PROGRAMMER: Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

echo 'make-distro.sh, NRC-CNRC, (c) 2007 - 2008, Her Majesty in Right of Canada'

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: make-distro.sh [-h(elp)] [-bin] [-nosrc] [-licence PROJECT] [-n]
       [-compile-only] [-compile-host HOST] [-rCVS_TAG|-DCVS_DATE]
       [-patch-from OLD_CD_DIR:PREREQ_TOKEN
          [-patch-from OLD_CD_DIR2:PREREQ_TOKEN2 [...]]]
       [-aachen] [-smart-bin] [-smart-src] [-can-univ] [-can-biz]
       [-d cvs_dir]
       -dir OUTPUT_DIR

  Make a PORTAGEshared distribution folder, ready to burn on CD or copy to a
  remote site as is.

Arguments:

  -r or -D      What source to use for this distro, as a well formatted CVS
                option: either -rCVS_TAG, with CVS_TAG (typically vX_Y) having
                been created first using "cvs tag -R CVS_TAG" on the whole
                PORTAGEshared repository.  Such a tag is recommended, but any
                valid cvs -r or -D option can be used, if necessary.

  -dir          The distro will be created in OUTPUT_DIR, which must not
                already exist.

Options:

  -h(elp):      print this help message
  -bin:         include compiled code [don't, unless -nosrc is specified]
  -d            cvs root repository
  -compile-only use this to compile code on a different architecture, with an
                OUTPUT_DIR where -bin has already been used.  All other options
                are ignored, except -dir.
  -compile-host with -bin, compile locally and also log on to HOST to compile
                again - must be on the same file system as the local machine.
  -nosrc        do not include src code [do]
  -licence      Copy the LICENCE file for PROJECT.  Can be CanUniv, SMART,
		Altera, CanBiz, or BinOnly (which means no LICENCE* file is
		copied).  [CanUniv, or BinOnly if -nosrc is specified]
  -n            Not Really: just show what will be done.
  -no-doxy      Don't generate the doxy files (use for faster testing) [do]
  -archive-name Infix to insert in .tar and .iso filenames. []
  -patch-from   Create patch OLD_CD_DIR_to_OUTPUT_DIR.patch for users who want
                to patch existing installations instead of doing a fresh
                install.  May be repeated.  Patches will be included in the
                .iso file.  PREREQ_TOKEN must be a word that exists in the old
                INSTALL but in no other distributed versions of INSTALL, past
                or future.  For v1.0, it should be "2004-2006,", for v1.1 (and
                subsequent) it should be PORTAGEshared_v1.1 (or vX.Y)
                For cutting and pasting for the -can-univ distro:
                  -patch-from v1.0:2004-2006,
                  -patch-from v1.1:PORTAGEshared_v1.1

Canned options for specific licensees:

  These options chooose the right combinations of the above for specific
  licensees.  -dir and -r must still be specified.

  -can-univ     Same as: -licence CanUniv -archive-name CanUniv
  -aachen       Same as: -bin -nosrc -licence BinOnly -compile-host leclerc
                         -archive-name BinOnly
  -smart-bin    Same as: -aachen
  -smart-src    Same as: -licence SMART -archive-name SMART
  -altera       Same as: -licence ALTERA -archive-name Altera
  -can-biz	Same as: -licence CanBiz -archive-name CanBiz

==EOF==

    exit 1
}

error_exit() {
   for msg in "$@"; do
      echo $msg
   done
   echo "Use -h for help."
   exit 1
}

run_cmd() {
   if [ "$1" = "-no-error" ]; then
      shift
      RUN_CMD_NO_ERROR=1
   fi
   echo "$*"
   if [ -z "$NOT_REALLY" ]; then
      eval $*
      rc=$?
   else
      rc=0
   fi
   if [ -z "$RUN_CMD_NO_ERROR" -a "$rc" != 0 ]; then
      echo "Exit status: $rc is not zero - aborting."
      exit 1
   fi
   RUN_CMD_NO_ERROR=
}

arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

SAVED_COMMAND_LINE="$*"

while [ $# -gt 0 ]; do
   case "$1" in
   -d)                  arg_check 1 $# $1; CVS_DIR="-d $2"; shift;;
   -bin)                INCLUDE_BIN=1;;
   -compile-only)       COMPILE_ONLY=1;;
   -compile-host)       arg_check 1 $# $1; COMPILE_HOST=$2; shift;;
   -nosrc)              NO_SOURCE=1;;
   -r*|-D*)             VERSION_TAG=$1;;
   -dir)                arg_check 1 $# $1; OUTPUT_DIR=$2; shift;;
   -licence|-license)   arg_check 1 $# $1; LICENCE=$2; shift;;
   -patch-from)         arg_check 1 $# $1; PATCH_FROM="$PATCH_FROM $2"; shift;;
   -archive-name)       arg_check 1 $# $1; ARCHIVE_NAME=$2; shift;;
   -can-univ)           LICENCE=CanUniv; ARCHIVE_NAME=CanUniv;;
   -smart-src)          LICENCE=SMART; ARCHIVE_NAME=SMART;;
   -aachen|-smart-bin)  INCLUDE_BIN=1; NO_SOURCE=1; LICENCE=BinOnly
                        COMPILE_HOST=leclerc; ARCHIVE_NAME=BinOnly;;
   -altera)             LICENCE=ALTERA; ARCHIVE_NAME=Altera;;
   -can-biz)		LICENCE=CanBiz; ARCHIVE_NAME=CanBiz;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -debug)              DEBUG=1;;
   -n)                  NOT_REALLY=1;;
   -no-doxy)            NO_DOXY=2;;
   -h|-help)            usage;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   error_exit "Extraneous command line argument $1.";;
   esac
   shift
done

if [ -n "$PATCH_FROM" ]; then
   for PATCH_ARG in $PATCH_FROM; do
      OLD_DIR=${PATCH_ARG%:*}
      PATCHLEVEL_TOKEN=${PATCH_ARG##*:}
      if [ "$PATCHLEVEL_TOKEN" = "$PATCH_ARG" ]; then
         error_exit "Invalid -patch-from specification: missing PREREQ_TOKEN"
      fi
      if [ ! -f $OLD_DIR/PORTAGEshared/INSTALL ]; then
         error_exit "Invalid -patch-from specification:" \
                    "$OLD_DIR/PORTAGEshared/INSTALL doesn't exist"
      fi
      if LC_ALL=C egrep -q '(^|[ 	])'$PATCHLEVEL_TOKEN'($|[ 	])' $OLD_DIR/PORTAGEshared/INSTALL; then
         true
      else
         error_exit "Invalid -patch-from specification:" \
            "$OLD_DIR/PORTAGEshared/INSTALL doesn't contain $PATCHLEVEL_TOKEN"
      fi
   done
fi

do_checkout() {
   if [ -z "$NOT_REALLY" ]; then
      test -d $OUTPUT_DIR && error_exit "$OUTPUT_DIR exists - won't overwrite -delete it first."
   fi

   run_cmd mkdir $OUTPUT_DIR
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd cvs $CVS_DIR co $VERSION_TAG PORTAGEshared '>&' cvs.log
      run_cmd find PORTAGEshared -name CVS \| xargs rm -rf

      if [ "$LICENCE" = SMART ]; then
         echo Keeping only SMART licence info.
         run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| \
                 grep -v -x PORTAGEshared/LICENCE_SMART \| xargs rm -f
      elif [ "$LICENCE" = CanUniv -o \( -z "$LICENCE" -a -z "$NO_SOURCE" \) ]; then
         echo Keeping only Canadian University licence info.
         run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| \
                 grep -v -x PORTAGEshared/LICENCE \| xargs rm -f
      elif [ "$LICENCE" = BinOnly -o -n "$NO_SOURCE" ]; then
         echo No source: removing all LICENCE files.
         run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| xargs rm -f
      elif [ "$LICENCE" = ALTERA ]; then
         echo Keeping only the Altera licence info.
         run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| \
                 grep -v -x PORTAGEshared/LICENCE_ALTERA \| xargs rm -f
      elif [ "$LICENCE" = CanBiz ]; then
         echo Keeping only the Canadian companies licence info.
         run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| \
                 grep -v -x PORTAGEshared/LICENCE_COMPANY \| xargs rm -f
      else
         error_exit "Invalid -licence specfication"
      fi

      run_cmd rm -f PORTAGEshared/make-distro.sh

   run_cmd popd
}

get_user_manual() {
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd rsync -arz ilt.iit-iti.priv:/export/projets/Lizzy/PORTAGEshared/snapshot/ \
                         PORTAGEshared/doc/user-manual
      run_cmd find PORTAGEshared/doc/user-manual/uploads -name Layout* \| xargs rm -f
      run_cmd rm PORTAGEshared/doc/user-manual/uploads/{cameleon_07.gif,images,notices,styles}
   run_cmd popd
}

make_pdfs() {
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared

      run_cmd pushd ./src
         run_cmd make docs '>&' ../../docs.log
         run_cmd cp */*.pdf ../doc/
         run_cmd make clean '>&' /dev/null
         run_cmd rm -f canoe/uml.eps
      run_cmd popd

      run_cmd pushd ./test-suite/toy
         run_cmd make doc
         run_cmd make clean
      run_cmd popd

   run_cmd popd
}

make_doxy() {
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared/src
      run_cmd make doxy '>&' ../../doxy.log
   run_cmd popd
}

make_bin() {
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared
      run_cmd pushd ./src
         run_cmd MY_PORTAGE_INSTALL= make install -j 5 '>&' ../../make_`arch`.log
         run_cmd make clean '>&' /dev/null
      run_cmd popd
      run_cmd pushd ./bin
         run_cmd mkdir -p `arch`
         run_cmd file \* \| grep ELF \| sed "'s/:.*//'" \| xargs -i{} mv {} `arch`
      run_cmd popd
      run_cmd pushd ./lib
         run_cmd mkdir -p `arch`
         run_cmd file \* \| grep ELF \| sed "'s/:.*//'" \| xargs -i{} mv {} `arch`
      run_cmd popd
   run_cmd popd
}

make_iso_and_tar() {
   VERSION=${VERSION_TAG#-r}
   VERSION=`echo $VERSION | sed 's/_//g'`
   VOLID=PORTAGEshared${VERSION}
   if [ -n "$ARCHIVE_NAME" ]; then
      ARCHIVE_FILE=${VOLID}_${ARCHIVE_NAME}
   else
      ARCHIVE_FILE=$VOLID
   fi
   run_cmd pushd ./$OUTPUT_DIR
      if [ -n "$PATCH_FROM" ]; then
         PATCH_FILES=*_to_*.patch
      else
         PATCH_FILES=
      fi
      run_cmd mkisofs -V $VOLID -joliet-long -o $ARCHIVE_FILE.iso \
              PORTAGEshared $PATCH_FILES '&>' iso.log
      run_cmd tar -cvzf $ARCHIVE_FILE.tar.gz PORTAGEshared '>&' tar.log
   run_cmd popd
}

test $OUTPUT_DIR  || error_exit "Missing mandatory -dir argument"

if [ -z "$COMPILE_ONLY" ]; then
   test $VERSION_TAG ||
      error_exit "Missing mandatory -rCVS_TAG or -DCVS_DATE argument"

   do_checkout
   get_user_manual
   make_pdfs
   if [ -z "$NO_SOURCE" -a -z "$NO_DOXY" ]; then
      echo Including source code documentation
      make_doxy
   fi
fi

if [ -n "$INCLUDE_BIN" -o -n "$COMPILE_ONLY" ]; then
   echo Including compiled code
   make_bin
fi

if [ -n "$COMPILE_HOST" ]; then
   echo Logging on to $COMPILE_HOST to compile code
   run_cmd ssh $COMPILE_HOST cd `pwd` \\\; $0 -compile-only -dir $OUTPUT_DIR
fi

if [ -z "$COMPILE_ONLY" ]; then

   if [ -n "$NO_SOURCE" ]; then
      run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared
         run_cmd mv ./doc/code-doc-binonly.html ./doc/code-documentation.html
         run_cmd rm -r src
      run_cmd popd
   else
      run_cmd rm ./$OUTPUT_DIR/PORTAGEshared/doc/code-doc-binonly.html
   fi

   if [ -n "PATCH_FROM" ]; then
      for PATCH_ARG in $PATCH_FROM; do
         OLD_DIR=${PATCH_ARG%:*}
         PATCHLEVEL_TOKEN=${PATCH_ARG##*:}
         run_cmd echo Prereq: $PATCHLEVEL_TOKEN \
                 \> $OUTPUT_DIR/${OLD_DIR}_to_${OUTPUT_DIR}.patch
         run_cmd -no-error LC_ALL=C TZ=UTC0 diff -Naur \
                 $OLD_DIR/PORTAGEshared $OUTPUT_DIR/PORTAGEshared \
                 \>\> $OUTPUT_DIR/${OLD_DIR}_to_${OUTPUT_DIR}.patch
         if head -2 $OUTPUT_DIR/${OLD_DIR}_to_${OUTPUT_DIR}.patch |
            grep -q PORTAGEshared/INSTALL
         then
            true
         else
            echo "WARNING: patch doesn't start with INSTALL,"
            echo "Prereq: $PATCHLEVEL_TOKEN line will likely not behave as expected."
            echo "A FIX IS LIKELY REQUIRED TO $0 ITSELF!!!"
         fi
      done
   fi

   make_iso_and_tar
fi

run_cmd echo "$0 $SAVED_COMMAND_LINE" \> $OUTPUT_DIR/make-distro-cmd-used

