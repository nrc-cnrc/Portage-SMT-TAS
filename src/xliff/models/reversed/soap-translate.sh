#!/bin/bash
translate.pl -src=en -tgt=fr -xsrc=EN-CA -xtgt=FR-CA -notc     -f=`dirname $0`/canoe.ini.cow "$@"
