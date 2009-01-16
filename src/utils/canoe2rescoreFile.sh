#!/bin/bash
# $Id$

# @file canoe2rescoreFile.sh 
# @brief ffvals with weigths are written to stdout.
# 
# PROGRAMMER: Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada


echo 'canoe2rescoreFile.sh, NRC-CNRC, (c) 2006 - 2009, Her Majesty in Right of Canada' >&2

if [ $# -ne 1 ] || [ "$1" == "-h" ]; then
    echo "Usage: $0  <canoe file>"
    echo "ffvals with weigths are written to stdout"
else
     grep weight $1 | \
	awk '{if ($1=="[weight-d]") wd = $2; \
		if ($1=="[weight-t]") wt = $2; \
		if ($1=="[weight-l]") wl = $2; \
		if ($1=="[weight-w]") ww = $2; \
	    }END{ \
		n=0; \
		num=split(wd,s,":"); for (i=1;i<=num;i++) printf("FileFF:ffvals,%i %s\n",++n,s[i]); \
		num=split(ww,s,":"); for (i=1;i<=num;i++) printf("FileFF:ffvals,%i %s\n",++n,s[i]); \
		num=split(wl,s,":"); for (i=1;i<=num;i++) printf("FileFF:ffvals,%i %s\n",++n,s[i]); \
		num=split(wt,s,":"); for (i=1;i<=num;i++) printf("FileFF:ffvals,%i %s\n",++n,s[i]); \
	    }'
fi
