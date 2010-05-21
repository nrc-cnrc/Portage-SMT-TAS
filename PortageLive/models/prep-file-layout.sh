#!/bin/bash
# $Id$
# @file prep-file-layoyt.sh
# @brief Put the CGI and SOAP files into the installed structure.
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

Usage: ./prep-file-layout.sh source

  TODO:
  Copy the CGI and SOAP files into the structure for packaging into an RPM or
  installing manually on a translation server.

  By default, the SOAP files are put in a staging location, /opt/Portage/www,
  and are then copied to the web server structure at each boot of the Virtual
  Appliance, with the IP address inserted in the right place.

  If you specify -ip FIXED_IP_ADDRESS, the SOAP files will be placed in their
  proper installed location, with the IP address hard-coded.  Only use this
  method for physical machines with static IPs.

  Once the layout has been prepared, you can package it into an RPM using
  ../scripts/make-rpm.sh

Option:

  -ip  Hard code the IP address for the translation server.  [Don't]

==EOF==

   exit 1
}

arg_check() {
   if [ $2 -le $1 ]; then
      usage "Missing argument to $3 option."
   fi
}

# error_exit "some error message" "optionnally a second line of error message"
# will exit with an error status, print the specified error message(s) on
# STDERR.
error_exit() {
   {
      PROG_NAME=`basename $0`
      echo -n "$PROG_NAME fatal error: "
      for msg in "$@"; do
         echo $msg
      done
      echo "Use -h for help."
   } >&2
   exit 1
}

set -o errexit

while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0  && error_exit "Missing source directory argument"
SOURCE=$1; shift
[[ $# -gt 0 ]] && usage "Superfluous argument(s) $*"

DESTINATION=rpm.build.root/opt/Portage/models/context/
mkdir -p $DESTINATION
scp -r $SOURCE/models/portageLive/* $DESTINATION

# Set proper permissions on the directory and file structure
find rpm.build.root -type d | xargs chmod 755
find rpm.build.root -type f | xargs chmod 644
find -type f -name \*.sh | xargs chmod 755
chmod 755 $DESTINATION/plugins/*

exit


######################################## ######################################## ########################################
# Set to the IP address if it is going to fixed, leave blank for a machine
# using DHCP or a VM.
if [[ $FIXED_IP ]]; then
   SOAP_DEST=rpm.build.root/var/www/html
else
   SOAP_DEST=rpm.build.root/opt/Portage/www
fi

# Create directory structure
mkdir -p rpm.build.root/var/www/html/plive   # working directory
mkdir -p rpm.build.root/var/www/html/images  # to hold all images
mkdir -p rpm.build.root/var/www/cgi-bin      # cgi scripts
mkdir -p $SOAP_DEST/secure                   # directory for SOAP stuff

# Copy the CGI scripts
cp cgi/*.cgi rpm.build.root/var/www/cgi-bin

# Copy the images needed by the CGI scripts
cp images/*.gif images/*.jpg rpm.build.root/var/www/html/images

# Copy the php and SOAP files
cp soap/{index.html,PortageLiveAPI.*,test.php} $SOAP_DEST
cp soap/secure/{index.html,PortageLiveAPI.*,test.php} $SOAP_DEST/secure

# For fixed IP, replace the token by the given IP
if [[ $FIXED_IP ]]; then
   sed "s/__REPLACE_THIS_WITH_YOUR_IP__/$FIXED_IP/" \
      < soap/PortageLiveAPI.wsdl > $SOAP_DEST/PortageLiveAPI.wsdl
   sed "s/__REPLACE_THIS_WITH_YOUR_IP__/$FIXED_IP/" \
      < soap/secure/PortageLiveAPI.wsdl > $SOAP_DEST/secure/PortageLiveAPI.wsdl
fi

