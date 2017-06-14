#!/bin/bash

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
while [ $# -gt 0 ]; do
   case "$1" in
   -with-ce)            ;;
   -decode-only)        ;;
   -xtags)              ;;
   -nl=*)               ;;
   -dir=*)              ;;
   -v|-verbose)         ;;
   -d|-debug)           ;;
   -h|-help)            ;;
   --)                  shift; break;;
   -*)                  ! echo "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

rev
