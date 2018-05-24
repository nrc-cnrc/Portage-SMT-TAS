# PortageII setup file for bash users.
# Source this file in bash to set all PortageII environment variables
#
# Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2018, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2018, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the PortageII
# package.
# Change this variable to indicate where this package is actually located.
PORTAGE=$HOME/PortageII-cur

# Where the generic model is installed. Update this variable if you install the
# generic model in a non-standard location.
PORTAGE_GENERIC_MODEL=$PORTAGE/generic-model

# Software overrides
# Uncomment and change the following if you installed these packages in
# a different location:
#CHINESE_SEGMENTATION_HOME_OVERRIDE=$PORTAGE/third-party/chinese-segmentation
#PYTHON_HOME_OVERRIDE=$PORTAGE/third-party/miniconda2
#PYTHON_HOME_OVERRIDE=$PORTAGE/third-party/python-2.7
#ICU_HOME_OVERRIDE=$PORTAGE/third-party/icu
#PERL_HOME_OVERRIDE=$PORTAGE/third-party/perl-5.14
#JAVA_HOME_OVERRIDE=$PORTAGE/third-party/jdk-1.7
#MITLM_HOME_OVERRIDE=$PORTAGE/third-party/mitlm
#PHPUNIT_HOME_OVERRIDE=$PORTAGE/third-party/phpunit

# Extra software configuration
# Add scripts called <prog-name>.bash in third-party/conf.d/, following the
# examples there, to configure additional third-party dependencies.

# Binary distributions only: we used to include 32 and 64 bits pre-compiled
# executable, with and without ICU.  We no longer support 32 bits since tuning
# requires more than 4GB of RAM.  We no longer provide non-ICU binaries since
# we distribute ICU libraries with PortageII.
# Nonetheless, PRECOMP_PORTAGE_ARCH continues to be required for binary
# distributions, in case we distribute new pre-compiles variants in the future.
# Uncomment the following line if your distribution is binary:
#PRECOMP_PORTAGE_ARCH=x86_64-el5-icu

# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'PortageII_cur, NRC-CNRC, (c) 2004 - 2018, Her Majesty in Right of Canada' >&2

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
export PORTAGE PORTAGE_GENERIC_MODEL PATH LD_LIBRARY_PATH PERL5LIB PYTHONPATH MANPATH
