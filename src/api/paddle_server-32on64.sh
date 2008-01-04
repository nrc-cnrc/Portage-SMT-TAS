#!/bin/bash

# $Id$
#
# paddle_server-32on64.sh - specific configuration to launch paddle_server
#                           in 32 bit mode on a 64 bit machine in Gatineau
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada


# Setup the default environment to the 32 bit stuff rather than the 64 bit stuff
export PORTAGE=/export/projets/portage/i686-g3.4.4
source /export/projets/portage/SETUP-specific.bash $PORTAGE

# Setup the specific environment to $HOME (or edit this line to use a specific
# demo build instead
source /export/projets/portage/SETUP-specific.bash $HOME

# Launch paddle_server from the directory where this script is located
echo ""
date
PADDLE_SERVER=`dirname $0`/paddle_server
echo "$PADDLE_SERVER $*"
exec $PADDLE_SERVER $*
