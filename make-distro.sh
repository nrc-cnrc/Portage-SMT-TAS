#!/bin/bash

# make-distro.sh - Make a CD or a copyable distro of PORTAGEshared.
# 
# PROGRAMMER: Eric Joanis
# 
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

echo 'make-distro.sh, NRC-CNRC, (c) 2007 - 2013, Her Majesty in Right of Canada'

GIT_PATH=balzac.iit.nrc.ca:/home/git

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: make-distro.sh [-h(elp)] [-bin] [-nosrc] [-n]
       [-compile-only] [-compile-host HOST] [-r GIT_TAG]
       [-patch-from OLD_CD_DIR:PREREQ_TOKEN
          [-patch-from OLD_CD_DIR2:PREREQ_TOKEN2 [...]]]
       [-src] [-bin2013]
       [-d GIT_PATH] [-framework FRAMEWORK]
       -dir OUTPUT_DIR

  Make a PORTAGEshared distribution folder, ready to burn on CD or copy to a
  remote site as is.

Arguments:

  -r            What source to use for this distro, as a valid argument to git
                clone's --branch option: a branch or a tag, typically a
                tag having been created first using "git tag v1_X_Y COMMIT;
                git push --tags", e.g.,:
                   git tag PortageII-cur master
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
                or future.
  -framework    Include framework from Git repository FRAMEWORK.

Canned options for specific licensees:

  These options chooose the right combinations of the above for specific
  licensees.  -dir and -r must still be specified.

  -src          Same as: -archive-name src
  -bin2013      Same as: -bin -nosrc -archive-name bin

Distro creation check list:
  - Substitue PortageII-cur for PortageII-2.3
  - Substitue PortageII cur for PortageII 2.3
  - Change src/build/Doxyfile PROJECT_NUMBER = 2.2 for PROJECT_NUMBER = 2.3
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

# Verbosely run a command, echoing was it was.
# If the command has a non-zero exit status, report it and abort.
# Options:
#  -m MODULENAME   print MODULENAME: in front of echoed commands
#  -no-error       ignore errors (i.e., don't abort of command has non-zero
#                  exit status)
NESTING_LEVEL=0
r() {
   NESTING_LEVEL=$((NESTING_LEVEL + 1))
   if [ "$1" = "-no-error" ]; then
      shift
      local RUN_CMD_NO_ERROR=1
   else
      local RUN_CMD_NO_ERROR=
   fi
   if [[ $1 = -m ]]; then
      local MODULE="$2: " ; shift ; shift
   else
      local MODULE=
   fi
   echo "$MODULE$*"
   if [ -z "$NOT_REALLY" ]; then
      eval $*
      rc=$?
   else
      rc=0
   fi
   if [ -z "$RUN_CMD_NO_ERROR" -a "$rc" != 0 ]; then
      echo "Exit status: $rc is not zero - aborting."
      if (( $NESTING_LEVEL > 1 )); then
         NESTING_LEVEL=$((NESTING_LEVEL - 1))
         return $rc
      else
         exit 1
      fi
   fi
   NESTING_LEVEL=$((NESTING_LEVEL - 1))
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
   -patch-from)         arg_check 1 $# $1; PATCH_FROM="$PATCH_FROM $2"; shift;;
   -archive-name)       arg_check 1 $# $1; ARCHIVE_NAME=$2; shift;;
   -src)                ARCHIVE_NAME=src;;
   -bin2013)            INCLUDE_BIN=1; NO_SOURCE=1; ARCHIVE_NAME=bin;;
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
   echo Using `hostname`.  Should be OK.
}

do_checkout() {
   print_header do_checkout
   if [[ ! $NOT_REALLY ]]; then
      [[ -d $OUTPUT_DIR && ! $FORCE ]] &&
         error_exit "$OUTPUT_DIR exists - won't overwrite -delete it first."
   fi

   r mkdir $OUTPUT_DIR
   r echo "$0 $SAVED_COMMAND_LINE" \> $OUTPUT_DIR/make-distro-cmd-used
   r echo Ran on `hostname` \>\> $OUTPUT_DIR/make-distro-cmd-used
   r pushd ./$OUTPUT_DIR
      # Note: "r r git" gives us the echo in both the git.log and make-distro logs.
      r r git clone -v --no-checkout $GIT_PATH/PORTAGEshared.git '>&' git.log
      r pushd PORTAGEshared
         r r git checkout $VERSION_TAG '>>' ../git.log '2>&1'
         r r git remote show origin '>>' ../git.log '2>&1'
         r r git show --abbrev=40 --format=fuller HEAD '>>' ../git.log '2>&1'
      r popd

      r chmod 755 PORTAGEshared/logs
      r chmod 777 PORTAGEshared/logs/accounting
      if [[ $FRAMEWORK ]]; then
         r pushd PORTAGEshared
            r r git clone -v --no-checkout $GIT_PATH/$FRAMEWORK.git framework '>&' ../git.framework.log
            r pushd framework
               r r git checkout $VERSION_TAG '>>' ../../git.framework.log '2>&1'
               r r git remote show origin '>>' ../../git.framework.log '2>&1'
               r r git show --abbrev=40 --format=fuller HEAD '>>' ../../git.framework.log '2>&1'
            r popd
         r popd
      fi
      r 'find PORTAGEshared -name .git\* | xargs rm -rf'
      r 'rm PORTAGEshared/.[a-z]*'

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

      r rm -f PORTAGEshared/src/.log.klocwork*
      r rm -f PORTAGEshared/make-distro.sh
      r rm -f PORTAGEshared/generic-model/make-distro.sh
   r popd
}

get_test_systems() {
   # Some test suites rely on data in $PORTAGE/test-suite/systems
   print_header get_test_systems
   r pushd ./$OUTPUT_DIR
      r rsync -arz balzac.iit.nrc.ca:/home/portage/test-suite/systems/ PORTAGEshared/test-suite/systems
   r popd
}

get_user_manual() {
   # Note: the user manual snapshot is created by loading this link:
   # http://wiki-ilt/PORTAGEshared/scripts/restricted/ywiki.cgi?act=snapshot
   print_header get_user_manual
   r pushd ./$OUTPUT_DIR
      r rsync -arz wiki-ilt:/export/projets/Lizzy/PORTAGEshared/snapshot/ \
                         PORTAGEshared/doc/user-manual
      r find PORTAGEshared/doc/user-manual/uploads -name Layout* \| xargs rm -f
      r find PORTAGEshared/doc/user-manual/uploads -name \*.gif.1 \| xargs rm -f
      r pushd PORTAGEshared/doc/user-manual/pages
         for x in *.html; do
            echo Making images relative in $x and renaming PORTAGE shared '->' PortageII.
            if [[ ! $NOT_REALLY ]]; then
               perl -e 'print '"'"'%s/IMG SRC="http:\/\/wiki-ilt\/PORTAGEshared\/uploads/img src="..\/uploads/'"'"'."\nw\nq\n"' | ed $x
               perl -e 'print '"'"'%s/img src="http:\/\/wiki-ilt\/PORTAGEshared\/uploads/img src="..\/uploads/'"'"'."\nw\nq\n"' | ed $x
               if grep -q 'PORTAGE shared' $x; then
                  perl -e 'print '"'"'%s/PORTAGE shared/PortageII/g'"'"'."\nw\nq\n"' | ed $x
               fi
               if grep -q 'Portage II' $x; then
                  perl -e 'print '"'"'%s/Portage II/PortageII/g'"'"'."\nw\nq\n"' | ed $x
               fi
               if grep -q 'Portage Live' $x; then
                  perl -e 'print '"'"'%s/Portage Live/PortageLive/g'"'"'."\nw\nq\n"' | ed $x
               fi
               if grep -q 'Portage Machine Translation' $x; then
                  perl -e 'print '"'"'%s/Portage Machine Translation/PortageII Machine Translation/g'"'"'."\nw\nq\n"' | ed $x
               fi
            fi
         done
      r popd
      r rm PORTAGEshared/doc/user-manual/uploads/{cameleon_07.gif,images,notices,styles}
   r popd
}

make_pdfs() {
   print_header make_pdfs
   r pushd ./$OUTPUT_DIR/PORTAGEshared

      r pushd ./src
         r r make docs '>&' ../../docs.log
         r cp */*.pdf ../doc/
         r make clean '>&' /dev/null
         r rm -f canoe/uml.eps
         r cp -p adaptation/README ../doc/adaptation.README
         r mkdir -p ../doc/confidence
         r cp -p confidence/README confidence/ce*.ini ../doc/confidence/
         r cp -p rescoring/README ../doc/rescoring.README
      r popd

      r pushd ./test-suite/unit-testing/toy
         r make doc
         r make clean
      r popd

      r pushd ./framework
         r make doc
         r make clean.doc
         r cp tutorial.pdf ../doc
      r popd

   r popd
}

make_doxy() {
   print_header "make_doxy (working in the background)"
   echo Including source code documentation.
   r pushd ./$OUTPUT_DIR/PORTAGEshared/src
      r r make doxy '>&' ../../doxy.log
      r -m DOXY mv htmldoc htmldoc_tmp
      r -m DOXY mkdir htmldoc
      r -m DOXY mv htmldoc_tmp htmldoc/files
      echo '<META HTTP-EQUIV="refresh" CONTENT="0; URL=files/index.html">' > htmldoc/index.html
   r -m DOXY popd
}

make_usage() {
   print_header make_usage
   echo Generating usage information.
   r pushd ./$OUTPUT_DIR/PORTAGEshared
      r r git clone -v --no-checkout $GIT_PATH/PORTAGEshared.git FOR_USAGE '>&' ../git.for_usage.log
      r pushd FOR_USAGE
         r r git checkout $VERSION_TAG '>>' ../../git.for_usage.log '2>&1'
      r popd
      r mv FOR_USAGE/src SRC_FOR_USAGE
      r pushd ./SRC_FOR_USAGE
         r r make ICU= LOG4CXX=NONE CF=-Wno-error -j 3 usage '>&' ../../make_usage.log
      r popd
      r rm -rf SRC_FOR_USAGE FOR_USAGE
   r popd
}

# Examine /etc/redhat-release to determine if we're building for el5 or el6
determine_distro_level() {
   local RELEASE_FILE=/etc/redhat-release
   if [[ -r $RELEASE_FILE ]]; then
      local GREP_OUT=`grep -o ' [0-9]\.[0-9]' $RELEASE_FILE | grep -o '[0-9]' | head -1`
      if [[ $GREP_OUT != "" ]]; then
         echo -n -el$GREP_OUT
      fi
   fi
}

make_bin() {
   print_header "make_bin ICU=$ICU"
   ELFDIR=`arch``determine_distro_level`
   if [[ $ICU = yes ]]; then
      ICU_LIB="ICU=$ICU_ROOT NO_ICU_RPATH=1"
      ELFDIR=$ELFDIR-icu
   else
      ICU_LIB=""
      ELFDIR=$ELFDIR
   fi
   r pushd ./$OUTPUT_DIR/PORTAGEshared
      r pushd ./src
         r r make install -j 4 $ICU_LIB '>&' ../../make_$ELFDIR.log
         r make clean '>&' /dev/null
      r popd
      r pushd ./bin
         r mkdir -p $ELFDIR
         r file \* \| grep ELF \| sed "'s/:.*//'" \| xargs -i{} mv {} $ELFDIR
      r popd
      r pushd ./lib
         r mkdir -p $ELFDIR
         r file \* \| grep ELF \| sed "'s/:.*//'" \| xargs -i{} mv {} $ELFDIR
         if [[ $ICU = yes ]]; then
            LD_LIBRARY_PATH=$ICU_ROOT/lib:$LD_LIBRARY_PATH ldd ../bin/$ELFDIR/canoe
         else
            ldd ../bin/$ELFDIR/canoe ../bin/$ELFDIR/arpalm.encode
         fi |
            egrep -o '/[^ ]*(portage|libtcmalloc|libprofiler|libicu)[^ ]*.so[^ ]*' |
            xargs -i cp {} $ELFDIR
         r rmdir --ignore-fail-on-non-empty $ELFDIR
      r popd
   r popd
}

make_iso_and_tar() {
   print_header make_iso_and_tar
   echo Generating tar ball and iso file.
   VERSION="${VERSION_TAG#-r}"
   VERSION="${VERSION// /.}"
   VOLID=${VERSION}
   VOLID=`echo "$VOLID" | perl -pe 's#[/:]#.#g'`
   #echo $VOLID
   ISO_VOLID=$VERSION
   ISO_VOLID=${ISO_VOLID:0:31}
   if [ -n "$ARCHIVE_NAME" ]; then
      ARCHIVE_FILE=${VOLID}-${ARCHIVE_NAME}
   else
      ARCHIVE_FILE=$VOLID
   fi
   r pushd ./$OUTPUT_DIR
      if [ -n "$PATCH_FROM" ]; then
         PATCH_FILES=*_to_*.patch
      else
         PATCH_FILES=
      fi
      r r mkisofs -V $ISO_VOLID -joliet-long -o $ARCHIVE_FILE.iso \
              PORTAGEshared $PATCH_FILES '>&' iso.log
      r mv PORTAGEshared PortageII-cur
      r r tar -cvzf $ARCHIVE_FILE.tar.gz PortageII-cur '>&' tar.log
      r md5sum $ARCHIVE_FILE.* \> $ARCHIVE_FILE.md5
   r popd
}


check_reliable_host

test $OUTPUT_DIR  || error_exit "Missing mandatory -dir argument"

# Block for manually calling just parts of this script - uncomment "true ||" to
# activate.
if
   #true ||
   false; then
   #do_checkout
   get_user_manual
   exit
fi

if [[ ! $COMPILE_ONLY ]]; then
   if [[ ! $VERSION_TAG ]]; then
      error_exit "Missing mandatory -r GIT_TAG argument"
   fi

   do_checkout
   get_test_systems
   get_user_manual
   make_pdfs
   if [[ ! $NO_SOURCE && ! $NO_DOXY ]]; then
      # We launch doxy in the background because it can't be parallelized and
      # it takes a long time.
      make_doxy &
      MAKE_DOXY_PID=$!
      # Wait a second so that the log is more-or-less linear
      sleep 1
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

   # Change SETUP.bash and SETUP.tcsh to have a default PRECOMP_PORTAGE_ARCH active
   perl -e 'print "%s/#\\(PRECOMP_PORTAGE_ARCH=\\)/\\1/\nw\nq\n"' | ed SETUP.bash
   perl -e 'print "%s/#\\(set PRECOMP_PORTAGE_ARCH=\\)/\\1/\nw\nq\n"' | ed SETUP.tcsh
fi

if [[ $MAKE_DOXY_PID ]]; then
   print_header "Wait for background make_doxy to finish"
   # wait for the background make_doxy process
   r wait $MAKE_DOXY_PID
fi

if [[ $MAKE_DOXY_PID ]]; then
   print_header "Wait for background make_doxy to finish"
   # wait for the background make_doxy process
   r wait $MAKE_DOXY_PID
fi

if [[ $COMPILE_HOST ]]; then
   print_header "Logging on to $COMPILE_HOST to compile code."
   r ssh $COMPILE_HOST cd `pwd` \\\; $0 -compile-only -dir $OUTPUT_DIR $ICU_OPT
fi

if [[ ! $COMPILE_ONLY ]]; then

   print_header "Cleaning up source and source-doc"
   if [[ $NO_SOURCE ]]; then
      r pushd ./$OUTPUT_DIR/PORTAGEshared
         r rm ./doc/code-documentation.html
         r rm -rf src
      r popd
   fi

   if [[ $PATCH_FROM ]]; then
      print_header "Preparing patches"
      for PATCH_ARG in $PATCH_FROM; do
         OLD_DIR=${PATCH_ARG%:*}
         PATCHLEVEL_TOKEN=${PATCH_ARG##*:}
         r echo Prereq: $PATCHLEVEL_TOKEN \
                 \> $OUTPUT_DIR/${OLD_DIR}_to_${OUTPUT_DIR}.patch
         r -no-error LC_ALL=C TZ=UTC0 diff -Naur \
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

print_header "All done successfully"
