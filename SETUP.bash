# PortageII setup file for bash users.
# Source this file in bash to set all PortageII environment variables
#
# Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2015, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the PortageII
# package.
# Change this variable to indicate where this package is actually located.
PORTAGE=$HOME/PortageII-cur

# Software overrides
# Uncomment and change the following if you installed these packages in
# a different location:
#CHINESE_SEGMENTATION_HOME_OVERRIDE=$PORTAGE/third-party/chinese-segmentation
#PYTHON_HOME_OVERRIDE=$PORTAGE/third-party/Python-2.7.10
#ICU_HOME_OVERRIDE=$PORTAGE/third-party/icu

# Extra software configuration

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

echo 'PortageII_cur, NRC-CNRC, (c) 2004 - 2015, Her Majesty in Right of Canada' >&2

# Setup third-party software in the default location
PATH=$PORTAGE/third-party/bin:$PORTAGE/third-party/scripts${PATH:+:$PATH}
LD_LIBRARY_PATH=$PORTAGE/third-party/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
MANPATH=$PORTAGE/third-party/share/man:$MANPATH

# Setup third-party software with specific configuration procedures
for config in $PORTAGE/third-party/conf.d/*.bash; do
   source $config
done

# Setup PortageII itself, making sure it comes ahead of third-party software
PATH=$PORTAGE/bin:$PATH
LD_LIBRARY_PATH=$PORTAGE/lib:$LD_LIBRARY_PATH
PERL5LIB=$PORTAGE/lib${PERL5LIB:+:$PERL5LIB}
PYTHONPATH=$PORTAGE/lib${PYTHONPATH:+:$PYTHONPATH}

# Setup architecture-specific binaries last, so they come ahead of everything else
if [[ $PRECOMP_PORTAGE_ARCH ]]; then
   PATH=$PORTAGE/bin/$PRECOMP_PORTAGE_ARCH:$PATH
   LD_LIBRARY_PATH=$PORTAGE/lib/$PRECOMP_PORTAGE_ARCH:$LD_LIBRARY_PATH
   unset PRECOMP_PORTAGE_ARCH
fi

# Export all the variables we setup in this script
export PORTAGE PATH LD_LIBRARY_PATH PERL5LIB PYTHONPATH MANPATH
