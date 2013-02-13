#!/bin/bash
# @file prep-file-layoyt.sh
# @brief Put the Python into the installed structure for portageLive
# 
# @author Darlene Stewart, based on other portageLive prep-file-layoyt.sh scripts
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: ./prep-file-layout.sh

  Copy the python 2.7 files into the structure for packaging into an RPM or
  installing manually on a translation server.

  The python installation is put in /opt/python-2.7.1 and 
  /opt/PortageII/bin/python is linked to the python /opt/python-2.7.1/bin/pyton.

  Once the layout has been prepared, you can package it into an RPM using
  ../scripts/make-rpm.sh

==EOF==

   exit 1
}

arg_check() {
   if [ $2 -le $1 ]; then
      usage "Missing argument to $3 option."
   fi
}

set -o errexit

FIXED_IP=
while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done
[[ $# -gt 0 ]] && usage "Superfluous argument(s) $*"

PYTHON_DIR=python-2.7.1-install

PYTHON_VERSION=$(basename ${PYTHON_DIR})
PYTHON_VERSION=${PYTHON_VERSION/%-install}

DEST=rpm.build.root/opt/${PYTHON_VERSION}

# Create directory structure
mkdir -p ${DEST}
mkdir -p rpm.build.root/opt/PortageII/bin

if [[ $PYTHON_DIR =~ : ]]; then
   CP_CMD="rsync -avrzlp"
else
   CP_CMD="cp -p -r"
fi

# Copy the python directories
${CP_CMD} ${PYTHON_DIR}/opt/${PYTHON_VERSION}/* $DEST


# Create the link in /opt/PortageII/bin
ln -s /opt/${PYTHON_VERSION}/bin/python rpm.build.root/opt/PortageII/bin/


# Set proper permissions on the directory and file structure
find rpm.build.root -type d | xargs chmod 755
find rpm.build.root -type f | xargs chmod u+rw,g+r,g-w,o+r,o-w

