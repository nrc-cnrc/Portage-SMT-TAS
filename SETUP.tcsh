# Portage 1.4 setup file for tcsh users.
# Source this file in tcsh to set all Portage environment variables
#
# Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2010, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the Portage 1.4
# package.
# Change this variable to indicate where this package is actually located.
setenv PORTAGE $HOME/Portage1.4

# Extra dynamic libraries.  If you had to install dynamic libraries in custom
# locations to get Portage 1.4 running, list the PATHs where the .so files
# are located in this variable.  Multiple directories should be separated by
# colons.  The sample value shown here would apply if you installed g++ 4.2.0
# in /opt/gcc-4.2.0 on a 64 bit machine.
#set EXTRA_DYNLIB_PATH=/opt/gcc-4.2.0/lib64:/opt/gcc-4.2.0/lib

# Binary distributions only: PRECOMP_PORTAGE_ARCH must be i686 (32 bits) or
# x86_64 (64 bits), depending on your architecture.  We only provide
# pre-compilied executables for those two architectures, though they may be
# compatible with other Linux 32 and 64 bit architectures.
#set PRECOMP_PORTAGE_ARCH=`arch`

# Binary distributions only: we also provide executables compiled with and
# without ICU, in order to make that dependency be optional.  Uncomment the
# following line instead of the previous one if you have installed and wish to
# use ICU.
#set PRECOMP_PORTAGE_ARCH=`arch`-icu

# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'Portage 1.4, NRC-CNRC, (c) 2004 - 2010, Her Majesty in Right of Canada' > /dev/stderr

if (! $?PATH) then
   setenv PATH $PORTAGE/bin
else
   setenv PATH $PORTAGE/bin:$PATH
endif

if (! $?LD_LIBRARY_PATH) then
   setenv LD_LIBRARY_PATH $PORTAGE/lib
else
   setenv LD_LIBRARY_PATH $PORTAGE/lib:$LD_LIBRARY_PATH
endif

if (! $?PERL5LIB) then
   setenv PERL5LIB $PORTAGE/lib
else
   setenv PERL5LIB $PORTAGE/lib:$PERL5LIB
endif

if (! $?CPLUS_INCLUDE_PATH) then
   setenv CPLUS_INCLUDE_PATH $PORTAGE/include
else
   setenv CPLUS_INCLUDE_PATH $PORTAGE/include:$CPLUS_INCLUDE_PATH
endif

if ($?PRECOMP_PORTAGE_ARCH) then
   setenv PATH $PORTAGE/bin/${PRECOMP_PORTAGE_ARCH}:$PATH
   setenv LD_LIBRARY_PATH $PORTAGE/lib/${PRECOMP_PORTAGE_ARCH}:$LD_LIBRARY_PATH
   unset PRECOMP_PORTAGE_ARCH
endif

if ($?EXTRA_DYNLIB_PATH) then
   setenv LD_LIBRARY_PATH ${EXTRA_DYNLIB_PATH}:$LD_LIBRARY_PATH
   unset EXTRA_DYNLIB_PATH
endif
