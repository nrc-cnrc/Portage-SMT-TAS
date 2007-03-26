# PORTAGEshared setup file for tcsh users.
# Source this file in tcsh to set all Portage environment variables
#
# Eric Joanis
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLE

# The PORTAGE environment variable points the the root of the PORTAGEshared
# package.
# Change this variable to indicate where this package is actually located.
setenv PORTAGE $HOME/PORTAGEshared

# END OF USER CONFIGURABLE VARIABLE
# =======================================================================

echo 'PORTAGEshared, NRC-CNRC, (c) 2004 - 2007, Her Majesty in Right of Canada'

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
    setenv CPLUS_INCLUDE_PATH "${CPLUS_INCLUDE_PATH}:${PORTAGE}/include
endif

