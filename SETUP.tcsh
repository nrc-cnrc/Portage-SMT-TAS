# PORTAGEshared setup file for tcsh users.
# Source this file in tcsh to set all Portage environment variables
#
# Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2008, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the PORTAGEshared
# package.
# Change this variable to indicate where this package is actually located.
setenv PORTAGE $HOME/PORTAGEshared

# Binary distributions only: PRECOMP_PORTAGE_ARCH must be i686 (32 bits) or
# x86_64 (64 bits), depending on your architecture.  We only provide
# pre-compilied executables for those two architectures, though they may be
# compatible with other Linux 32 and 64 bit architectures.
#set PRECOMP_PORTAGE_ARCH=`arch`

# We also provide executables compiled with and without ICU, in order to make
# that dependency be optional.  Uncomment the following line instead of the
# previous one if you have installed and wish to use ICU.
#set PRECOMP_PORTAGE_ARCH=`arch`-icu


# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'PORTAGEshared, NRC-CNRC, (c) 2004 - 2008, Her Majesty in Right of Canada'

if (! $?PATH) then
   setenv PATH ${PORTAGE}/bin
else
   setenv PATH "${PATH}:${PORTAGE}/bin"
endif

if (! $?LD_LIBRARY_PATH) then
   setenv LD_LIBRARY_PATH ${PORTAGE}/lib
else
   setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${PORTAGE}/lib"
endif

if (! $?PERL5LIB) then
   setenv PERL5LIB ${PORTAGE}/lib:${PORTAGE}/lib/perl5/site_perl
else
   setenv PERL5LIB "${PERL5LIB}:${PORTAGE}/lib:${PORTAGE}/lib/perl5/site_perl"
endif

if (! $?CPLUS_INCLUDE_PATH) then
   setenv CPLUS_INCLUDE_PATH ${PORTAGE}/include
else
   setenv CPLUS_INCLUDE_PATH ${CPLUS_INCLUDE_PATH}:${PORTAGE}/include
endif

if ($?PRECOMP_PORTAGE_ARCH) then
   setenv PATH "${PATH}:${PORTAGE}/bin/$PRECOMP_PORTAGE_ARCH"
   setenv LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${PORTAGE}/lib/$PRECOMP_PORTAGE_ARCH"
   unset PRECOMP_PORTAGE_ARCH
endif
