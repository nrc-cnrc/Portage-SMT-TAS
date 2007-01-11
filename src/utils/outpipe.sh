#!/bin/bash -f

# Aaron Tikuisis
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

## 
## Usage: outpipe.sh outfile
## 
## If outfile is -, directs standard input to standard output.  Otherwise, directs
## standard input into the file outfile.
## 

if [ $# -ne 1 -o "$1" == "-h" ]; then
    cat $0 | grep "^##" | cut -c4-
    exit 1
fi

if [ "$1" == "-" ]; then
    cat
else
    cat > $1
fi
