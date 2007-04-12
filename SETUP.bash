# PORTAGEshared setup file for bash users.
# Source this file in bash to set all Portage environment variables
#
# Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2007, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the PORTAGEshared
# package.
# Change this variable to indicate where this package is actually located.
export PORTAGE=$HOME/PORTAGEshared

# If your boost installation is in a non-standard location, fix and uncomment
# the following line:
#export CPLUS_INCLUDE_PATH=/full/path/to/boost/include

# Binary distributions only: PRECOMP_PORTAGE_ARCH must be i686 (32 bits) or
# x86_64 (64 bits), depending on your architecture.  We only provide
# pre-compilied executables for those two architectures, though they may be
# compatible with other Linux 32 and 64 bit architectures.
#PRECOMP_PORTAGE_ARCH=`arch`

# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'PORTAGEshared, NRC-CNRC, (c) 2004 - 2007, Her Majesty in Right of Canada'

if [ ${PATH:-UNDEF} = "UNDEF" ] ; then
   export PATH=$PORTAGE/bin
else
   export PATH=$PATH:$PORTAGE/bin
fi

if [ ${LD_LIBRARY_PATH:-UNDEF} = "UNDEF" ] ; then
   export LD_LIBRARY_PATH=$PORTAGE/lib
else
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PORTAGE/lib
fi

if [ ${PERL5LIB:-UNDEF} = "UNDEF" ] ; then
   export PERL5LIB=$PORTAGE/lib:$PORTAGE/lib/perl5/site_perl
else
   export PERL5LIB=$PERL5LIB:$PORTAGE/lib:$PORTAGE/lib/perl5/site_perl
fi

if [ ${CPLUS_INCLUDE_PATH:-UNDEF} = "UNDEF" ] ; then
   export CPLUS_INCLUDE_PATH=$PORTAGE/include
else
   export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$PORTAGE/include
fi

if [ ${PRECOMP_PORTAGE_ARCH:-UNDEF} != "UNDEF" ]; then
   export PATH=$PATH:$PORTAGE/bin/$PRECOMP_PORTAGE_ARCH
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PORTAGE/lib/$PRECOMP_PORTAGE_ARCH
   unset PRECOMP_PORTAGE_ARCH
fi
