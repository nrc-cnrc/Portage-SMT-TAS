# PortageII setup file for tcsh users.
# Source this file in tcsh to set all PortageII environment variables
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
setenv PORTAGE $HOME/PortageII-cur

# Where the generic model is installed. Update this variable if you install the
# generic model in a non-standard location.
setenv PORTAGE_GENERIC_MODEL $PORTAGE/generic-model

# Software overrides
# Uncomment and change the following if you installed these packages in
# a different location:
#set CHINESE_SEGMENTATION_HOME_OVERRIDE=$PORTAGE/third-party/chinese-segmentation
#set PYTHON_HOME_OVERRIDE=$PORTAGE/third-party/python-2.7
#set ICU_HOME_OVERRIDE=$PORTAGE/third-party/icu
#set PERL_HOME_OVERRIDE=$PORTAGE/third-party/perl-5.14
#set JAVA_HOME_OVERRIDE=$PORTAGE/third-party/jdk-1.7
#set MITLM_HOME_OVERRIDE=$PORTAGE/third-party/mitlm


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
#set PRECOMP_PORTAGE_ARCH=x86_64-icu

# END OF USER CONFIGURABLE VARIABLES
# =======================================================================

echo 'PortageII_cur, NRC-CNRC, (c) 2004 - 2018, Her Majesty in Right of Canada' > /dev/stderr

# Setup third-party software in the default location
# Setup architecture-specific binaries last, so they come ahead of everything else
if (! $?PATH) then
   setenv PATH $PORTAGE/third-party/bin:$PORTAGE/third-party/scripts
else
   setenv PATH $PORTAGE/third-party/bin:$PORTAGE/third-party/scripts:$PATH
endif

if (! $?LD_LIBRARY_PATH) then
   setenv LD_LIBRARY_PATH $PORTAGE/third-party/lib
else
   setenv LD_LIBRARY_PATH $PORTAGE/third-party/lib:$LD_LIBRARY_PATH
endif

if (! $?MANPATH) then
   setenv MANPATH $PORTAGE/third-party/share/man:
else
   setenv MANPATH $PORTAGE/third-party/share/man:$MANPATH
endif

# Setup third-party software with specific configuration procedures
foreach config ($PORTAGE/third-party/conf.d/*.tcsh)
   source $config
end

# Setup PortageII itself, making sure it comes ahead of third-party software
setenv PATH $PORTAGE/bin:$PATH
setenv LD_LIBRARY_PATH $PORTAGE/lib:$LD_LIBRARY_PATH

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

# Setup architecture-specific binaries last, so they come ahead of everything else
if ($?PRECOMP_PORTAGE_ARCH) then
   setenv PATH $PORTAGE/bin/${PRECOMP_PORTAGE_ARCH}:$PATH
   setenv LD_LIBRARY_PATH $PORTAGE/lib/${PRECOMP_PORTAGE_ARCH}:$LD_LIBRARY_PATH
   unset PRECOMP_PORTAGE_ARCH
endif

