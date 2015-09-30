# PortageII setup file for tcsh users.
# Source this file in tcsh to set all PortageII environment variables
#
# Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2014, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the PortageII
# package.
# Change this variable to indicate where this package is actually located.
setenv PORTAGE $HOME/PortageII-cur

# Extra dynamic libraries.  If you had to install dynamic libraries in custom
# locations to get PortageII running, list the PATHs where the .so files are
# located in this variable.  Multiple directories should be separated by
# colons.  The sample value shown here would apply if you installed g++ 4.6.0
# in /opt/gcc-4.6.0 on a 64 bit machine.
#set EXTRA_DYNLIB_PATH=/opt/gcc-4.6.0/lib64:/opt/gcc-4.6.0/lib

# Extra program PATHs.  If you installed Python, Perl, your language modelling
# toolkit, or other required programs in non-standard locations, you can add
# them to your PATH globally on your system, or add them to the
# EXTRA_PROGRAM_PATH variable here.
#set EXTRA_PROGRAM_PATH=/path/to/python2.7/bin:/path/to/other/dependency/bin

# Binary distributions only: we used to include 32 and 64 bits pre-compiled
# executable, with and without ICU.  We no longer support 32 bits since tuning
# requires more than 4GB of RAM.  We no longer provide non-ICU binaries since
# we distribute ICU libraries with PortageII.
# Nonetheless, PRECOMP_PORTAGE_ARCH continues to be required for binary
# distributions, in case we distribute new pre-compiles variants in the future.
# Uncomment the following line if your distribution is binary:
#set PRECOMP_PORTAGE_ARCH=x86_64-icu

# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'PortageII_cur, NRC-CNRC, (c) 2004 - 2013, Her Majesty in Right of Canada' > /dev/stderr

set TMP_PORTAGE_PATH $PORTAGE/bin:$PORTAGE/third-party/bin
if ($?EXTRA_PROGRAM_PATH) then
   set TMP_PORTAGE_PATH ${TMP_PORTAGE_PATH}:$EXTRA_PROGRAM_PATH
   unset EXTRA_PROGRAM_PATH
endif

if (! $?PATH) then
   setenv PATH $TMP_PORTAGE_PATH
else
   setenv PATH ${TMP_PORTAGE_PATH}:$PATH
endif
unset TMP_PORTAGE_PATH

set TMP_PORTAGE_DYNLIB $PORTAGE/lib:$PORTAGE/third-party/lib
if ($?EXTRA_DYNLIB_PATH) then
   set TMP_PORTAGE_DYNLIB ${TMP_PORTAGE_DYNLIB}:$EXTRA_DYNLIB_PATH
   unset EXTRA_DYNLIB_PATH
endif

if (! $?LD_LIBRARY_PATH) then
   setenv LD_LIBRARY_PATH $TMP_PORTAGE_DYNLIB
else
   setenv LD_LIBRARY_PATH ${TMP_PORTAGE_DYNLIB}:$LD_LIBRARY_PATH
endif
unset TMP_PORTAGE_DYNLIB

if (! $?PERL5LIB) then
   setenv PERL5LIB $PORTAGE/lib
else
   setenv PERL5LIB $PORTAGE/lib:$PERL5LIB
endif

if (! $?PYTHONPATH) then
   setenv PYTHONPATH $PORTAGE/lib
else
   setenv PYTHONPATH $PORTAGE/lib:$PYTHONPATH
endif

if ($?PRECOMP_PORTAGE_ARCH) then
   setenv PATH $PORTAGE/bin/${PRECOMP_PORTAGE_ARCH}:$PATH
   setenv LD_LIBRARY_PATH $PORTAGE/lib/${PRECOMP_PORTAGE_ARCH}:$LD_LIBRARY_PATH
   unset PRECOMP_PORTAGE_ARCH
endif

