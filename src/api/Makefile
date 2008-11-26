# $Id$# @file Makefile
# @file Makefile
# @brief Portage API Makefile
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology 
# Conseil national de recherches Canada / National Research Council Canada 
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada 


# Note: you need the Portage dependency libraries to be available in the
# library and include search paths.  These paths should be given by
# Makefile.incl.  As a sidenote, if you want to specify these paths via
# environment variables, type the following (in bash):
#  export LIBRARY_PATH=$LIBRARY_PATH:/export/projets/portage/lib
#  export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/export/projets/portage/include

# Redundantly include user configuration variables - we need to know early what
# ACE is set to in this makefile.
include ../Makefile.user-conf

ifeq ($(ACE), NONE)
   # When ACE is not available, we don't use the default INCLUDE and SOURCES,
   # since we remove thos files that depend on ACE.
   SOURCES=$(filter-out net_srv.cc paddle_server.cc paddle_client.cc, \
             $(wildcard *.cc))
   INCLUDES=$(filter-out net_srv.h, $(wildcard *.h))
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
   DYNLIBS=-lACE $(LIBICU)
   ifneq ($(ACE),)
      MODULE_CF=-I$(ACE)/include
      MODULE_LF=-L$(ACE)/lib
   endif
else
   DYNLIBS= $(LIBICU)
endif

MODULE_DEPENDS=canoe tm lm logging

include ../build/Makefile.incl

Override rules for programs:

echo_paddle_server.o:
	true

# Custom target to replace portage_api.o by the stub echo_portage_api.o.
echo_paddle_server: paddle_server.o echo_portage_api.o
	$(CXX) $(LINKFLAGS) -o $@ net_srv.o paddle_server.o echo_portage_api.o $(LINKWITH) $(DYNLIBS)

# This custom target for paddle_client doesn't link with any superfluous
# libraries so that it can be as light-weight as possible for the Web server.
paddle_client: paddle_client.o
	$(CXX) $(LINKFLAGS) -o $@ paddle_client.o -lACE

-include Makefile.depend
