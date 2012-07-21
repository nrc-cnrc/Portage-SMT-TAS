#!/bin/bash
# $Id$

# make-distro.sh - Make a DVD or a copyable distro of Generic Model.
# 
# PROGRAMMER: Darlene Stewart based on Eric Joanis' PORTAGEshared/make-distro.sh
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

echo 'make-distro.sh, NRC-CNRC, (c) 2012, Her Majesty in Right of Canada'

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: make-distro.sh [-h(elp)] [-licence PROJECT] [-n]
       [-rCVS_TAG|-DCVS_DATE] [-d cvs_dir] -models MODELS
       -dir OUTPUT_DIR

  Make a generic-model distribution folder, ready to burn on CD or copy to a
  remote site as is.

Arguments:

  -r or -D      What source to use for this distro, as a well formatted CVS
                option: either -rCVS_TAG, with CVS_TAG (typically vX_Y) having
                been created first using "cvs tag -R CVS_TAG" on the whole
                PORTAGEshared repository, or:
                   cvs rtag -Dnow v1_5_0 PORTAGEshared
                   cvs rtag -Dnow v1_5_0 portage.simple.framework.2
                Such a tag is recommended, but any valid cvs -r or -D option
                can be used, if necessary.
                
    -models     Include models from directory MODELS.

    -dir        The distro will be created in OUTPUT_DIR, which must not
                already exist.

Options:

  -h(elp):      print this help message
  -d            cvs root repository
  -licence      Copy the LICENCE file for PROJECT. []
  -n            Not Really: just show what will be done.
  -no-archives  Don't generate the tar ball or iso files [do]
  -archive-name Infix to insert in .tar and .iso filenames. []

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

for x in "$@"; do
   if [[ $x =~ " " ]]; then
      SAVED_COMMAND_LINE="$SAVED_COMMAND_LINE \\\"$x\\\""
   else
      SAVED_COMMAND_LINE="$SAVED_COMMAND_LINE $x"
   fi
done

while [ $# -gt 0 ]; do
   case "$1" in
   -d)                  arg_check 1 $# $1; CVS_DIR="-d $2"; shift;;
   -r*|-D*)             VERSION_TAG="$1";;
   -dir)                arg_check 1 $# $1; OUTPUT_DIR=$2; shift;;
   -models)             arg_check 1 $# $1; MODELS=$2; shift;;
   -archive-name)       arg_check 1 $# $1; ARCHIVE_NAME=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -debug)              DEBUG=1;;
   -n)                  NOT_REALLY=1;;
   -no-archives)        NO_ARCHIVES=1;;
   -h|-help)            usage;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   error_exit "Extraneous command line argument $1.";;
   esac
   shift
done

print_header() {
   fn=$1
   echo
   echo =============================================
   echo $fn
   echo =============================================
}

check_reliable_host() {
   print_header check_reliable_host
   [[ `hostname` = joyce ]] &&
      error_exit "This script does not work on Joyce.  Use Verlaine instead."
}

do_checkout() {
   print_header do_checkout
   if [[ ! $NOT_REALLY ]]; then
      [[ -d $OUTPUT_DIR && ! $FORCE ]] &&
         error_exit "$OUTPUT_DIR exists - won't overwrite -delete it first."
   fi

   run_cmd mkdir $OUTPUT_DIR
   run_cmd echo "$0 $SAVED_COMMAND_LINE" \> $OUTPUT_DIR/make-distro-cmd-used
   run_cmd echo Ran on `hostname` \>\> $OUTPUT_DIR/make-distro-cmd-used
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd cvs $CVS_DIR co -P \"$VERSION_TAG\" -d generic-model PORTAGEshared/generic-model '>&' cvs.log
      run_cmd find generic-model -name CVS \| xargs rm -rf

      run_cmd rm -f generic-model/make-distro.sh
   run_cmd popd
}

get_models() {
   print_header make_models
   run_cmd rsync -rLptv --progress $MODELS/* ./$OUTPUT_DIR/generic-model/
}

make_iso_and_tar() {
   print_header make_iso_and_tar
   echo Generating tar ball and iso file.
   
   #VERSION=v1_0
   VERSION=${OUTPUT_DIR//./_}
   VOLID=PortageGenericModel_${VERSION}
   VOLID=`echo "$VOLID" | perl -pe 's#[/:]#.#g'`
   #echo $VOLID
   ISO_VOLID=PortageGenericModel`echo $VERSION | sed -e 's/v//g' -e 's/_/./g'`
   ISO_VOLID=${ISO_VOLID:0:31}
   if [ -n "$ARCHIVE_NAME" ]; then
      ARCHIVE_FILE=${VOLID}_${ARCHIVE_NAME}
   else
      ARCHIVE_FILE=$VOLID
   fi
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd mkisofs -V $ISO_VOLID -joliet-long -o $ARCHIVE_FILE.iso \
              generic-model '&>' iso.log
      run_cmd mv generic-model PortageGenericModel1.0
      run_cmd tar -cvzf $ARCHIVE_FILE.tar.gz PortageGenericModel1.0 '>&' tar.log
      run_cmd md5sum $ARCHIVE_FILE.* \> $ARCHIVE_FILE.md5
   run_cmd popd
}

check_reliable_host

test $OUTPUT_DIR  || error_exit "Missing mandatory -dir argument"

if [[ ! $VERSION_TAG ]]; then
   error_exit "Missing mandatory -rCVS_TAG or -DCVS_DATE argument"
fi

do_checkout
get_models

if [[ ! $NO_ARCHIVES ]]; then
   make_iso_and_tar
fi

echo
echo ALL DONE!

