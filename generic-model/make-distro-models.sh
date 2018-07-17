#!/bin/bash

# make-distro-models.sh - Make a copyable distro of Portage Generic Model.
# 
# PROGRAMMER: Darlene Stewart based on Eric Joanis' PORTAGEshared/make-distro.sh
# 
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, 2016, 2018, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, 2016, 2018, Her Majesty in Right of Canada

# Creating the PortageGenericModel-2.1 distro: in $PORTAGE/models/generic-model/v2.0/distro_v2.1/:
# ~/sandboxes/PORTAGEshared.dev/generic-model/make-distro-models.sh -r gpsc -models ../dvd_v2.1_models -dir v2.1_models -cur 2.1 &> log.v2.1_models

echo 'make-distro-models.sh, NRC-CNRC, (c) 2012-2018, Her Majesty in Right of Canada'

GIT_PATH=$PORTAGE_GIT_ROOT

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: make-distro-models.sh [-h(elp)] [-n] [-d GIT_PATH] -r GIT_TAG
       -models MODELS -dir OUTPUT_DIR -cur VERSION

  Make a generic-model background models distribution folder, ready to burn on
  a DVD or copy to a remote site as is.

Arguments:

  -r            What source to use for this distro, as a valid argument to git
                clone's --branch option: a branch or a tag, typically a
                tag having been created first using "git tag v1_X_Y COMMIT;
                git push --tags", e.g.,:
                   git tag PortageII-cur master
                   git push --tags
                run in both PORTAGEshared and portage.framework.
                
  -models       Include models from directory MODELS.

  -dir          The distro will be created in OUTPUT_DIR, which must not
                already exist.

  -cur          Current version number, e.g., 2.1

Options:

  -h(elp):      print this help message
  -d            Git server host and dirname [$GIT_PATH]
  -n            Not Really: just show what will be done.
  -no-archives  Don't generate the tar ball [do]
  -archive-name Infix to insert in .tar filenames. []
  -v(erbose)    Increment the verbosity level by 1 (may be repeated)
  -debug        Print debugging information

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
   -d)                  arg_check 1 $# $1; GIT_PATH="$2"; shift;;
   -r)                  arg_check 1 $# $1; VERSION_TAG="$2"; shift;;
   -dir)                arg_check 1 $# $1; OUTPUT_DIR=$2; shift;;
   -cur)                arg_check 1 $# $1; CUR_VERSION=$2; shift;;
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

[[ $CUR_VERSION ]] || error_exit "Missing mandatory -cur VERSION argument"
[[ $OUTPUT_DIR ]]  || error_exit "Missing mandatory -dir OUTPUT_DIR argument"
[[ $VERSION_TAG ]] || error_exit "Missing mandatory -r GIT_TAG argument"

print_header() {
   fn=$1
   echo
   echo =============================================
   echo $fn
   echo =============================================
   date
}

check_reliable_host() {
   print_header check_reliable_host
   echo Using `hostname`.
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
      run_cmd git clone -n $GIT_PATH/PORTAGEshared.git '>&' git-clone.log
      run_cmd pushd PORTAGEshared
         run_cmd git checkout $VERSION_TAG -- generic-model/models
         run_cmd mv generic-model/models ../generic-model
      run_cmd popd
      run_cmd rm -rf PORTAGEshared
   run_cmd popd
}

get_models() {
   print_header make_models
   run_cmd rsync -rLptv --progress $MODELS/* ./$OUTPUT_DIR/generic-model/
}

make_tar() {
   print_header make_tar
   echo Generating tar ball file.
   
   VOLID=PortageGenericModel-$CUR_VERSION
   if [ -n "$ARCHIVE_NAME" ]; then
      ARCHIVE_FILE=${VOLID}-${ARCHIVE_NAME}
   else
      ARCHIVE_FILE=$VOLID
   fi
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd mv generic-model $VOLID
      run_cmd chmod -R u=rwX,g+rX,g-w,o+rX,o-w $VOLID
      run_cmd tar -cvzf $ARCHIVE_FILE.tar.gz $VOLID '>&' tar.log
      run_cmd md5sum $ARCHIVE_FILE.* \> $ARCHIVE_FILE.md5
   run_cmd popd
}

check_reliable_host

[[ $OUTPUT_DIR ]] || error_exit "Missing mandatory -dir argument"
[[ $MODELS ]] || error_exit "Missing mandatory -models argument"
[[ $VERSION_TAG ]] || error_exit "Missing mandatory -r GIT_TAG argument"

do_checkout
get_models

if [[ ! $NO_ARCHIVES ]]; then
   make_tar
fi

echo
echo ALL DONE!

