#!/bin/bash
# @file prep-file-layoyt.sh
# @brief Put the CGI and SOAP files into the installed structure.
#
# @author Eric Joanis, based on Samuel Larkin's Makefile
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
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

  By default, the SOAP files are put in a staging location, /opt/PortageII/www,
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

ROOT_DIR="rpm.build.root"
HTML_DIR="${ROOT_DIR}/var/www/html"

# Set to the IP address if it is going to fixed, leave blank for a machine
# using DHCP or a VM.
if [[ $FIXED_IP ]]; then
   SOAP_DEST=${HTML_DIR}
else
   SOAP_DEST=${ROOT_DIR}/opt/PortageII/www
fi

# Create directory structure
mkdir -p ${HTML_DIR}/plive   # working directory
mkdir -p ${HTML_DIR}/images  # to hold all images
mkdir -p ${HTML_DIR}/secure  # to hold all images
mkdir -p ${ROOT_DIR}/var/www/cgi-bin      # cgi scripts
mkdir -p $SOAP_DEST/secure                   # directory for SOAP stuff

# Copy the CGI scripts
cp cgi/*.{pm,cgi} ${ROOT_DIR}/var/www/cgi-bin

# Copy the css
cp cgi/*.css ${HTML_DIR}

# Copy the images needed by the CGI scripts
cp images/*.{gif,jpg,png} ${HTML_DIR}/images

# Copy the icon needed by the CGI scripts
cp images/*.ico ${HTML_DIR}

ln -s ../images ${HTML_DIR}/secure/
ln -s ../favicon.ico ${HTML_DIR}/secure/

# Copy the php and SOAP files
cp soap/{index.html,PortageLiveAPI.*,wsdl-viewer.xsl,test.php,soap.php} $SOAP_DEST
# Copy them into secure/ as well, for use with ssl/https.
cp soap/{index.html,PortageLiveAPI.php,wsdl-viewer.xsl,test.php,soap.php} $SOAP_DEST/secure
perl -ple 's/(http)(:\/\/__REPLACE_THIS_WITH_YOUR_IP__)/$1s$2/g' \
   < soap/PortageLiveAPI.wsdl \
   > $SOAP_DEST/secure/PortageLiveAPI.wsdl

# For fixed IP, replace the token by the given IP
if [[ $FIXED_IP ]]; then
   sed "s/__REPLACE_THIS_WITH_YOUR_IP__/$FIXED_IP/g" \
      < soap/PortageLiveAPI.wsdl \
      > $SOAP_DEST/PortageLiveAPI.wsdl
   perl -ple 's/(http)(:\/\/__REPLACE_THIS_WITH_YOUR_IP__)/$1s$2/g' \
      < soap/PortageLiveAPI.wsdl \
   | sed "s/__REPLACE_THIS_WITH_YOUR_IP__/$FIXED_IP/g" \
      > $SOAP_DEST/secure/PortageLiveAPI.wsdl
fi

# Generate the XML copy of the WSDL, as human-readable documentation
for wsdl in `find $SOAP_DEST -name PortageLiveAPI.wsdl`; do
    soap/gen-xml.pl < $wsdl > $wsdl.xml
done

# Set proper permissions on the directory and file structure
find ${ROOT_DIR} -type d | xargs chmod 755
find ${ROOT_DIR} -type f | xargs chmod 644
find -name \*.cgi | xargs chmod o+x
chmod o+w ${HTML_DIR}/plive

# Create a tarball of portagelive's www.
cd ${ROOT_DIR} && tar zchvf ../portagelive.www.tgz * && cd ..

