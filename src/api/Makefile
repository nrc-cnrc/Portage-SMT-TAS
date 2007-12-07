# $Id$# @file Makefile
# @file Makefile
# 
# Portage API
# George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology 
# Conseil national de recherches Canada / National Research Council Canada 
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada 
# 
# This is the Makefile for this module.

# =============================================================================
# USER CONFIGURABLE VARIABLE

# Variable:        ACE
# Location:        This variable is not defined here, but in api/Makefile.
#                  It is documented here for information purposes only.
# Possible values: defined to "NONE" disables the use of ACE (and the
#                  compilation of paddle_server and paddle_client, the web
#                  demo), any other value or undefined enables the use of ACE.
# Notes:           Comment out this line to compile the web demo - requires ACE
ACE=NONE

# END OF USER CONFIGURABLE VARIABLE
# =============================================================================

# Note: you need the Portage dependency libraries to be available in the
# library and include search paths.  These paths should be given by
# Makefile.incl.  As a sidenote, if you want to specify these paths via
# environment variables, type the following (in bash):
#  export LIBRARY_PATH=$LIBRARY_PATH:/export/projets/portage/lib
#  export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/export/projets/portage/include

# This Makefile's SOURCES, INCLUDES and OBJECTS variables are more complex than
# for other Makefiles, since we conditionally disable the compilation of
# anything that depends on ACE if it's not available.

NON_ACE_SOURCES=postprocessor.cc portage_api.cc preprocessor.cc
ifneq ($(ACE), NONE)
   SOURCES= $(NON_ACE_SOURCES) net_srv.cc paddle_server.cc paddle_client.cc
else
   SOURCES= $(NON_ACE_SOURCES)
endif

NON_ACE_INCLUDES=portage_api.h portage_env.h postprocessor.h preprocessor.h \
                 translator_if.h
ifneq ($(ACE), NONE)
   INCLUDES= $(NON_ACE_INCLUDES) net_srv.h
else
   INCLUDES= $(NON_ACE_INCLUDES)
endif

NON_ACE_OBJECTS=portage_api.o postprocessor.o preprocessor.o
ifneq ($(ACE), NONE)
   OBJECTS= $(NON_ACE_OBJECTS) net_srv.o
else
   OBJECTS= $(NON_ACE_OBJECTS)
endif

LIBRARY=

PERL_LIBS=

TESTPROGS=

ifneq ($(ACE), NONE)
   PROGRAMS=$(TESTPROGS) paddle_server paddle_client echo_paddle_server

   BINSCRIPTS= paddle_server-32on64.sh paddle_remote_launch.pl
endif

INFOFILES=

DOCUMENTS=

EXTRA_LIBS=$(LOGGING_LIB)

ifneq ($(ACE), NONE)
   DYNLIBS=-lACE
endif

MODULE_CF= -Wno-non-virtual-dtor
   #-Wno-deprecated -Wno-sign-compare -Wno-reorder

MODULE_DEPENDS=canoe tm lm logging

include ../build/Makefile.incl

Override rules for programs:

echo_paddle_server.o:
	true

# Custom target to replace portage_api.o by the stub echo_portage_api.o.
echo_paddle_server: paddle_server.o echo_portage_api.o
	$(CXX) $(CXXFLAGS) -o $@ net_srv.o paddle_server.o echo_portage_api.o postprocessor.o preprocessor.o $(LINKWITH) $(DYNLIBS)

# This custom target for paddle_client doesn't link with any superfluous
# libraries so that it can be as light-weight as possible for the Web server.
paddle_client: paddle_client.o
	$(CXX) $(CXXFLAGS) -o $@ paddle_client.o -lACE

-include Makefile.depend
