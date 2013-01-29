#!/bin/bash

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

GIT_PATH=balzac.iit.nrc.ca:/home/git

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: make-distro.sh [-h(elp)] [-bin] [-nosrc] [-licence PROJECT] [-n]
       [-compile-only] [-compile-host HOST] [-r GIT_TAG]
       [-patch-from OLD_CD_DIR:PREREQ_TOKEN
          [-patch-from OLD_CD_DIR2:PREREQ_TOKEN2 [...]]]
       [-aachen] [-smart-bin] [-smart-src] [-can-univ] [-can-biz]
       [-d GIT_PATH] [-framework FRAMEWORK]
       -dir OUTPUT_DIR

  Make a PORTAGEshared distribution folder, ready to burn on CD or copy to a
  remote site as is.

Arguments:

  -r            What source to use for this distro, as a valid argument to git
                clone's --branch option: a branch or a tag, typically a
                tag having been created first using "git tag v1_X_Y COMMIT;
                git push --tags", e.g.,:
                   git tag v1_5_1 master
                   git push --tags
                run in both PORTAGEshared and portage.framework.

  -dir          The distro will be created in OUTPUT_DIR, which must not
                already exist.

Options:

  -h(elp):      print this help message
  -bin:         include compiled code [don't, unless -nosrc is specified]
  -icu {yes|no|both} if "yes" link with ICU when compiling code; if "no", don't
                link with ICU; if "both", compile and link twice, once with and
                once without ICU. [yes]
  -icu-root PATH_TO_ICU find ICU in PATH_TO_ICU/lib instead of $PORTAGE/lib
                when compiling and linking with ICU.  If PATH_TO_ICU includes
                the special token _ARCH_, that token will be replaced by the
                output of calling arch.
  -d            Git server host and dirname [$GIT_PATH]
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
  -no-usage     Don't generate the usage web page [do]
  -no-archives  Don't generate the tar ball or iso files [do]
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
  -framework    Include framework from Git repository FRAMEWORK.

Canned options for specific licensees:

  These options chooose the right combinations of the above for specific
  licensees.  -dir and -r must still be specified.

  -can-univ     Same as: -licence CanUniv -archive-name CanUniv
  -aachen       Same as: -bin -nosrc -licence BinOnly -compile-host leclerc
                         -archive-name BinOnly
  -smart-bin    Same as: -aachen
  -bin2010      Same as: -bin -nosrc -licence BinOnly -archive-name BinOnly
  -smart-src    Same as: -licence SMART -archive-name SMART
  -altera       Same as: -licence ALTERA -archive-name Altera
  -can-biz	Same as: -licence CanBiz -archive-name CanBiz

Distro creation check list:
  - Change the version number and year in src/utils/portage_info.cc,
    SETUP.{bash,tcsh}, the Wiki, the INSTALL file, all the README files
  - Change the value of current_year in portage_utils.{pm,py} and printCopyright.h
  - Change the year on the Wiki in /export/projets/Lizzy/PORTAGEshared/resources/layout/LayoutSnapshotFrench.html
  - Update the RELEASES file
  - Make a fresh snapshot of the wiki by loading this link:
    http://wiki-ilt/PORTAGEshared/scripts/restricted/ywiki.cgi?act=snapshot

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

ICU=yes
ICU_ROOT=$PORTAGE
while [ $# -gt 0 ]; do
   case "$1" in
   -d)                  arg_check 1 $# $1; GIT_PATH="$2"; shift;;
   -bin)                INCLUDE_BIN=1;;
   -icu)                arg_check 1 $# $1; ICU=$2; ICU_OPT="$1 $2"; shift;;
   -icu-root)           arg_check 1 $# $1; ICU_ROOT=$2; shift;;
   -compile-only)       COMPILE_ONLY=1;;
   -compile-host)       arg_check 1 $# $1; COMPILE_HOST=$2; shift;;
   -nosrc)              NO_SOURCE=1;;
   -r)                  arg_check 1 $# $1; VERSION_TAG="$2"; shift;;
   -dir)                arg_check 1 $# $1; OUTPUT_DIR=$2; shift;;
   -licence|-license)   arg_check 1 $# $1; LICENCE=$2; shift;;
   -patch-from)         arg_check 1 $# $1; PATCH_FROM="$PATCH_FROM $2"; shift;;
   -archive-name)       arg_check 1 $# $1; ARCHIVE_NAME=$2; shift;;
   -can-univ)           LICENCE=CanUniv; ARCHIVE_NAME=CanUniv;;
   -smart-src)          LICENCE=SMART; ARCHIVE_NAME=SMART;;
   -aachen|-smart-bin)  INCLUDE_BIN=1; NO_SOURCE=1; LICENCE=BinOnly
                        COMPILE_HOST=leclerc; ARCHIVE_NAME=BinOnly;;
   -bin2010)            INCLUDE_BIN=1; NO_SOURCE=1; LICENCE=bin2010
                        ARCHIVE_NAME=BinOnly;;
   -altera)             LICENCE=ALTERA; ARCHIVE_NAME=Altera;;
   -can-biz)		LICENCE=CanBiz; ARCHIVE_NAME=CanBiz;;
   -framework)          arg_check 1 $# $1; FRAMEWORK=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -debug)              DEBUG=1;;
   -n)                  NOT_REALLY=1;;
   -no-doxy)            NO_DOXY=2;;
   -no-usage)           NO_USAGE=1;;
   -no-archives)        NO_ARCHIVES=1;;
   -h|-help)            usage;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   error_exit "Extraneous command line argument $1.";;
   esac
   shift
done

if [[ $ICU_ROOT != $PORTAGE ]]; then
   ICU_OPT="$ICU_OPT -icu-root $ICU_ROOT"
   ICU_ROOT=${ICU_ROOT//_ARCH_/`arch`}
fi

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
      run_cmd git clone --branch $VERSION_TAG $GIT_PATH/PORTAGEshared.git '>&' git-clone.log
      run_cmd chmod 755 PORTAGEshared/logs
      run_cmd chmod 777 PORTAGEshared/logs/accounting
      if [[ $FRAMEWORK ]]; then
         run_cmd pushd PORTAGEshared
            run_cmd git clone --branch $VERSION_TAG $GIT_PATH/$FRAMEWORK.git framework '>&' ../git-clone.framework.log
         run_cmd popd
      fi
      run_cmd 'find PORTAGEshared -name .git\* | xargs rm -rf'
      run_cmd 'rm PORTAGEshared/.[a-z]*'

      if [ "$LICENCE" = SMART ]; then
         echo Keeping only SMART licence info.
         run_cmd find PORTAGEshared -maxdepth 1 -name LICENCE\* \| \
                 grep -v -x PORTAGEshared/LICENCE_SMART \| xargs rm -f
      elif [ "$LICENCE" = CanUniv -o \( -z "$LICENCE" -a -z "$NO_SOURCE" \) ]; then
         echo Keeping only Canadian University licence info.
         run_cmd find PORTAGEshared -maxdepth 1 -name LICENCE\* \| \
                 grep -v -x PORTAGEshared/LICENCE \| xargs rm -f
      elif [ "$LICENCE" = BinOnly -o -n "$NO_SOURCE" ]; then
         echo No source: removing all LICENCE files.
         run_cmd find PORTAGEshared -maxdepth 1 -name LICENCE\* \| xargs rm -f
      elif [ "$LICENCE" = ALTERA ]; then
         echo Keeping only the Altera licence info.
         run_cmd find PORTAGEshared -maxdepth 1 -name LICENCE\* \| \
                 grep -v -x PORTAGEshared/LICENCE_ALTERA \| xargs rm -f
      elif [ "$LICENCE" = CanBiz ]; then
         echo Keeping only the Canadian companies licence info.
         run_cmd find PORTAGEshared -maxdepth 1 -name LICENCE\* \| \
                 grep -v -x PORTAGEshared/LICENCE_COMPANY \| xargs rm -f
      else
         error_exit "Invalid -licence specification"
      fi

      echo Removing -Werror from src/build/Makefile.incl.
      if [[ ! $NOT_REALLY ]]; then
         perl -e 'print "%s/ -Werror / /\nw\nq\n"' |
            ed PORTAGEshared/src/build/Makefile.incl
      fi

      echo Removing NRC specific config in src/Makefile.user-conf
      if [[ ! $NOT_REALLY ]]; then
         perl -e 'print "g/AT NRC/d\nw\nq\n"' |
            ed PORTAGEshared/src/Makefile.user-conf
      fi

      run_cmd rm -f PORTAGEshared/src/.log.klocwork*
      run_cmd rm -f PORTAGEshared/make-distro.sh
      run_cmd rm -f PORTAGEshared/generic-model/make-distro.sh
   run_cmd popd
}

get_test_systems() {
   # Some test suites rely on data in $PORTAGE/test-suite/systems
   print_header get_test_systems
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd rsync -arz balzac.iit.nrc.ca:/home/portage/test-suite/systems/ PORTAGEshared/test-suite/systems
   run_cmd popd
}

get_user_manual() {
   # Note: the user manual snapshot is created by loading this link:
   # http://wiki-ilt/PORTAGEshared/scripts/restricted/ywiki.cgi?act=snapshot
   print_header get_user_manual
   run_cmd pushd ./$OUTPUT_DIR
      run_cmd rsync -arz wiki-ilt:/export/projets/Lizzy/PORTAGEshared/snapshot/ \
                         PORTAGEshared/doc/user-manual
      run_cmd find PORTAGEshared/doc/user-manual/uploads -name Layout* \| xargs rm -f
      run_cmd find PORTAGEshared/doc/user-manual/uploads -name \*.gif.1 \| xargs rm -f
      run_cmd pushd PORTAGEshared/doc/user-manual/pages
         for x in *.html; do
            echo Making images relative in $x and renaming PORTAGE shared '->' Portage 1.5.1.
            if [[ ! $NOT_REALLY ]]; then
               perl -e 'print '"'"'%s/IMG SRC="http:\/\/wiki-ilt\/PORTAGEshared\/uploads/img src="..\/uploads/'"'"'."\nw\nq\n"' | ed $x
               perl -e 'print '"'"'%s/img src="http:\/\/wiki-ilt\/PORTAGEshared\/uploads/img src="..\/uploads/'"'"'."\nw\nq\n"' | ed $x
               if grep -q 'PORTAGE shared' $x; then
                  perl -e 'print '"'"'%s/PORTAGE shared/Portage 1.5.1/g'"'"'."\nw\nq\n"' | ed $x
               fi
            fi
         done
      run_cmd popd
      run_cmd rm PORTAGEshared/doc/user-manual/uploads/{cameleon_07.gif,images,notices,styles}
   run_cmd popd
}

make_pdfs() {
   print_header make_pdfs
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared

      run_cmd pushd ./src
         run_cmd make docs '>&' ../../docs.log
         run_cmd cp */*.pdf ../doc/
         run_cmd make clean '>&' /dev/null
         run_cmd rm -f canoe/uml.eps
         run_cmd cp -p adaptation/README ../doc/adaptation.README
         run_cmd mkdir -p ../doc/confidence
         run_cmd cp -p confidence/README confidence/ce*.ini ../doc/confidence/
         run_cmd cp -p rescoring/README ../doc/rescoring.README
      run_cmd popd

      run_cmd pushd ./test-suite/unit-testing/toy
         run_cmd make doc
         run_cmd make clean
      run_cmd popd

      run_cmd pushd ./framework
         run_cmd make doc
         run_cmd make clean.doc
         run_cmd cp tutorial.pdf ../doc
      run_cmd popd

   run_cmd popd
}

make_doxy() {
   print_header make_doxy
   echo Including source code documentation.
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared/src
      run_cmd make doxy '>&' ../../doxy.log
      run_cmd mv htmldoc htmldoc_tmp
      run_cmd mkdir htmldoc
      run_cmd mv htmldoc_tmp htmldoc/files
      echo '<META HTTP-EQUIV="refresh" CONTENT="0; URL=files/index.html">' > htmldoc/index.html
   run_cmd popd
}

make_usage() {
   print_header make_usage
   echo Generating usage information.
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared
      run_cmd git clone --branch $VERSION_TAG $GIT_PATH/PORTAGEshared.git FOR_USAGE '>&' ../git-clone.for_usage.log
      run_cmd mv FOR_USAGE/src SRC_FOR_USAGE
      run_cmd pushd ./SRC_FOR_USAGE
         run_cmd make ICU= LOG4CXX=NONE CF=-Wno-error -j 3 usage '>&' ../../make_usage.log
      run_cmd popd
      run_cmd rm -rf SRC_FOR_USAGE FOR_USAGE
   run_cmd popd
}

make_bin() {
   print_header "make_bin ICU=$ICU"
   ELFDIR=`arch`
   if [[ $ICU = yes ]]; then
      ICU_LIB="ICU=$ICU_ROOT NO_ICU_RPATH=1"
      ELFDIR=$ELFDIR-icu
   else
      ICU_LIB=""
      ELFDIR=$ELFDIR
   fi
   run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared
      run_cmd pushd ./src
         run_cmd make install -j 3 $ICU_LIB '>&' ../../make_$ELFDIR.log
         run_cmd make clean '>&' /dev/null
      run_cmd popd
      run_cmd pushd ./bin
         run_cmd mkdir -p $ELFDIR
         run_cmd file \* \| grep ELF \| sed "'s/:.*//'" \| xargs -i{} mv {} $ELFDIR
      run_cmd popd
      run_cmd pushd ./lib
         run_cmd mkdir -p $ELFDIR
         run_cmd file \* \| grep ELF \| sed "'s/:.*//'" \| xargs -i{} mv {} $ELFDIR
         if [[ $ICU = yes ]]; then
            LD_LIBRARY_PATH=$ICU_ROOT/lib:$LD_LIBRARY_PATH ldd ../bin/$ELFDIR/canoe | egrep -o '/[^ ]*(icu|portage)[^ ]*.so[^ ]*' | xargs -i cp {} $ELFDIR
         else
            ldd ../bin/$ELFDIR/canoe | egrep -o '/[^ ]*portage[^ ]*.so[^ ]*' | xargs -i cp {} $ELFDIR
         fi
         run_cmd rmdir --ignore-fail-on-non-empty $ELFDIR
      run_cmd popd
   run_cmd popd
}

make_iso_and_tar() {
   print_header make_iso_and_tar
   echo Generating tar ball and iso file.
   VERSION="${VERSION_TAG#-r}"
   VERSION="${VERSION// /.}"
   VOLID=Portage_${VERSION}
   VOLID=`echo "$VOLID" | perl -pe 's#[/:]#.#g'`
   #echo $VOLID
   ISO_VOLID=Portage`echo $VERSION | sed -e 's/v//g' -e 's/_/./g'`
   ISO_VOLID=${ISO_VOLID:0:31}
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
      run_cmd mkisofs -V $ISO_VOLID -joliet-long -o $ARCHIVE_FILE.iso \
              PORTAGEshared $PATCH_FILES '&>' iso.log
      run_cmd mv PORTAGEshared Portage1.5.1
      run_cmd tar -cvzf $ARCHIVE_FILE.tar.gz Portage1.5.1 '>&' tar.log
      run_cmd md5sum $ARCHIVE_FILE.* \> $ARCHIVE_FILE.md5
   run_cmd popd
}


check_reliable_host

test $OUTPUT_DIR  || error_exit "Missing mandatory -dir argument"

if [[ ! $COMPILE_ONLY ]]; then
   if [[ ! $VERSION_TAG ]]; then
      error_exit "Missing mandatory -r GIT_TAG argument"
   fi

   do_checkout
   get_test_systems
   get_user_manual
   make_pdfs
   if [[ ! $NO_SOURCE && ! $NO_DOXY ]]; then
      make_doxy
   fi
   if [[ ! $NO_USAGE ]]; then
      make_usage
   fi
fi

if [[ $INCLUDE_BIN || $COMPILE_ONLY ]]; then
   echo ""
   echo Including compiled code.
   if [[ $ICU = both ]]; then
      ICU=yes
      make_bin
      ICU=no
      make_bin
   else
      make_bin
   fi
fi

if [[ $COMPILE_HOST ]]; then
   print_header "Logging on to $COMPILE_HOST to compile code."
   run_cmd ssh $COMPILE_HOST cd `pwd` \\\; $0 -compile-only -dir $OUTPUT_DIR $ICU_OPT
fi

if [[ ! $COMPILE_ONLY ]]; then

   print_header "Cleaning up source and source-doc"
   if [[ $NO_SOURCE ]]; then
      run_cmd pushd ./$OUTPUT_DIR/PORTAGEshared
         run_cmd rm ./doc/code-documentation.html
         run_cmd rm -rf src
      run_cmd popd
   fi

   if [[ $PATCH_FROM ]]; then
      print_header "Preparing patches"
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

   if [[ ! $NO_ARCHIVES ]]; then
      make_iso_and_tar
   fi
fi


