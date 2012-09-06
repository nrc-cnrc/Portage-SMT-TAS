# Portage 1.5 setup file for tcsh users.
# Source this file in tcsh to set all Portage environment variables
#
# Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006 - 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006 - 2012, Her Majesty in Right of Canada


# =======================================================================
# USER CONFIGURABLE VARIABLES

# The PORTAGE environment variable points the the root of the Portage 1.5
# package.
# Change this variable to indicate where this package is actually located.
setenv PORTAGE $HOME/Portage1.5.0

# Extra dynamic libraries.  If you had to install dynamic libraries in custom
# locations to get Portage 1.5 running, list the PATHs where the .so files
# are located in this variable.  Multiple directories should be separated by
# colons.  The sample value shown here would apply if you installed g++ 4.6.0
# in /opt/gcc-4.6.0 on a 64 bit machine.
#set EXTRA_DYNLIB_PATH=/opt/gcc-4.6.0/lib64:/opt/gcc-4.6.0/lib

# Extra program PATHs.  If you installed Python, Perl, your language modelling
# toolkit, or other required programs in non-standard locations, you can add
# them to your PATH globally on your system, or add them to the
# EXTRA_PROGRAM_PATH variable here.
#set EXTRA_PROGRAM_PATH=/path/to/python2.7/bin:/path/to/other/dependency/bin

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

echo 'Portage 1.5.0, NRC-CNRC, (c) 2004 - 2012, Her Majesty in Right of Canada' > /dev/stderr

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

if (! $?PYTHONPATH) then
   setenv PYTHONPATH $PORTAGE/lib
else
   setenv PYTHONPATH $PORTAGE/lib:$PYTHONPATH
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

if ($?EXTRA_PROGRAM_PATH) then
   setenv PATH ${EXTRA_PROGRAM_PATH}:$PATH
   unset EXTRA_PROGRAM_PATH
endif
