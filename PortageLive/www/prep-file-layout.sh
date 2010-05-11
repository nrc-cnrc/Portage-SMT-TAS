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

Usage: ./prep-file-layout.sh [-ip FIXED_IP_ADDRESS]

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

set -o errexit

FIXED_IP=
while [ $# -gt 0 ]; do
   case "$1" in
   -ip)                 arg_check 1 $# $1; FIXED_IP=$2; shift;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done
[[ $# -gt 0 ]] && usage "Superfluous argument(s) $*"

# Set to the IP address if it is going to fixed, leave blank for a machine
# using DHCP or a VM.
if [[ $FIXED_IP ]]; then
   SOAP_DEST=rpm.build.root/var/www/html
else
   SOAP_DEST=rpm.build.root/opt/Portage/www
fi

# Create directory structure
mkdir -p rpm.build.root/var/www/html/plive   # working directory
mkdir -p rpm.build.root/var/www/html/cgi-bin # cgi scripts
mkdir -p $SOAP_DEST/secure                   # directory for SOAP stuff

# Copy the CGI scripts
cp cgi/*.cgi rpm.build.root/var/www/html/cgi-bin

# Copy the php and SOAP files
cp soap/{client.php,index.html,PortageLiveAPI.*} $SOAP_DEST
cp soap/secure/{client.php,index.html,PortageLiveAPI.*} $SOAP_DEST/secure

# For fixed IP, replace the token by the given IP
if [[ $FIXED_IP ]]; then
   sed "s/__REPLACE_THIS_WITH_YOUR_IP__/$FIXED_IP/" \
      < soap/PortageLiveAPI.wsdl > $SOAP_DEST/PortageLiveAPI.wsdl
   sed "s/__REPLACE_THIS_WITH_YOUR_IP__/$FIXED_IP/" \
      < soap/secure/PortageLiveAPI.wsdl > $SOAP_DEST/secure/PortageLiveAPI.wsdl
fi

# Set proper permissions on the directory and file structure
find rpm.build.root -type d | xargs chmod 755
find rpm.build.root -type f | xargs chmod 644
