#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0  <canoe file>"
    echo "ffvals with weights are written to stdout"
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
