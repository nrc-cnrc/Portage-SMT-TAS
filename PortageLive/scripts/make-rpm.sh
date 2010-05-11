#!/bin/bash
# $Id$
# @file make-rpm.sh
# @brief Build the RPM from a prepared file layout
# 
# @author Eric Joanis, based on Samuel Larkin's Makefile
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: $0

  Build an RPM from the file layout in rpm.build.root/, and the RPM control
  specifications control.spec.  Increments the release number in control.spec.
  Edit control.spec and adjust the fields and descriptions as necessary before
  calling this script.

  This script requires VMware's mkpkg script.

  Run this script from inside the www, bin or models directory, after having
  created the rpm.build.root file layout as instructed in each directory.

==EOF==

   exit 1
}

if [[ $# -gt 0 ]]; then usage; fi

set -o errexit

# Increment the Release number in the control.spec
mv control.spec control.spec.old
awk '{if ($1 ~ /Release/) {print $1" " $2+1} else {print $0 }} ' < control.spec.old > control.spec
rm control.spec.old

# Make the rpm itself.  Requires VMware's mkpkg script in your PATH.
time mkpkg -t rpm -c control.spec rpm.build.root/

# Rename the rpm to have the build architecture specified in the control.spec
# because mkpkg always calls it i386, regardless of your specification.
BUILD_ARCH=`egrep BuildArchitectures control.spec  | cut -f2 -d ' '`
RPM_FILE=`ls -v *rpm | tail -1`
mv $RPM_FILE ${RPM_FILE%%_i386.rpm}_$BUILD_ARCH.rpm
