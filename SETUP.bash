# PORTAGEshared setup file for bash users.
# Source this file in bash to set all Portage environment variables
#
# Eric Joanis
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Conseil national de recherches Canada / National Research Council Canada


# =======================================================================
# USER CONFIGURABLE VARIABLE

# The PORTAGE environment variable points the the root of the PORTAGEshared
# package.
# Change this variable to indicate where this package is actually located.
export PORTAGE=$HOME/PORTAGEshared

# END OF USER CONFIGURABLE VARIABLE
# =======================================================================

echo 'PORTAGEshared, Copyright (c) 2004 - 2006, Conseil national de recherches Canada / National Research Council Canada'

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

