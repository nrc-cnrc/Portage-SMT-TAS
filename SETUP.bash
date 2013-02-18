# PortageII setup file for bash users.
# Source this file in bash to set all PortageII environment variables
#
# Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2013, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the PortageII
# package.
# Change this variable to indicate where this package is actually located.
PORTAGE=$HOME/PortageII-2.0

# Extra dynamic libraries.  If you had to install dynamic libraries in custom
# locations to get PortageII running, list the PATHs where the .so files are
# located in this variable.  Multiple directories should be separated by
# colons.  The sample value shown here would apply if you installed g++ 4.6.0
# in /opt/gcc-4.6.0 on a 64 bit machine.
#EXTRA_DYNLIB_PATH=/opt/gcc-4.6.0/lib64:/opt/gcc-4.6.0/lib

# Extra program PATHs.  If you installed Python, Perl, your language modelling
# toolkit, or other required programs in non-standard locations, you can add
# them to your PATH globally on your system, or add them to the
# EXTRA_PROGRAM_PATH variable here.
#EXTRA_PROGRAM_PATH=/path/to/python2.7/bin:/path/to/other/dependency/bin

# Binary distributions only: we used to include 32 and 64 bits pre-compiled
# executable, with and without ICU.  We no longer support 32 bits since tuning
# requires more than 4GB of RAM.  We no longer provide non-ICU binaries since
# we distribute ICU libraries with PortageII.
# Nonetheless, PRECOMP_PORTAGE_ARCH continues to be required for binary
# distributions, in case we distribute new pre-compiles variants in the future.
# Uncomment the following line if your distribution is binary:
#PRECOMP_PORTAGE_ARCH=x86_64-icu

# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'PortageII 2.0, NRC-CNRC, (c) 2004 - 2013, Her Majesty in Right of Canada' >&2

PATH=$PORTAGE/bin${PATH:+:$PATH}
if [[ $EXTRA_PROGRAM_PATH ]]; then
   PATH=$EXTRA_PROGRAM_PATH:$PATH
fi

LD_LIBRARY_PATH=$PORTAGE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

PERL5LIB=$PORTAGE/lib${PERL5LIB:+:$PERL5LIB}

PYTHONPATH=$PORTAGE/lib${PYTHONPATH:+:$PYTHONPATH}

if [[ $PRECOMP_PORTAGE_ARCH ]]; then
   PATH=$PORTAGE/bin/$PRECOMP_PORTAGE_ARCH:$PATH
   LD_LIBRARY_PATH=$PORTAGE/lib/$PRECOMP_PORTAGE_ARCH:$LD_LIBRARY_PATH
   unset PRECOMP_PORTAGE_ARCH
fi

if [[ $EXTRA_DYNLIB_PATH ]]; then
   LD_LIBRARY_PATH=$EXTRA_DYNLIB_PATH:$LD_LIBRARY_PATH
   unset EXTRA_DYNLIB_PATH
fi

export PORTAGE PATH LD_LIBRARY_PATH PERL5LIB PYTHONPATH
