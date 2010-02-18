#!/bin/bash
# This file is derivative work from Ulrich Germann's Tightly Packed Tries
# package (TPTs and related software).
#
# Original Copyright:
# Copyright 2005-2009 Ulrich Germann; all rights reserved.
# Under licence to NRC.
#
# Copyright for modifications:
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008-2010, Her Majesty in Right of Canada



GNG=${1:?}

if [ ! -f ${GNG}/1gms/vocab_cs.gz ]; then
    echo "usage: $0 <path to google ngram data dir>"
    echo "ERROR: could not find ${GNG}/1gms/vocab_cs.gz"
    exit 1;
fi

tmpdir=tmp.$$
jobfile=encode.jobs
mkdir ${tmpdir} && cd ${tmpdir}

gng.make-tokenindex ${GNG}/1gms/vocab_cs.gz gng.tdx
echo "gng.encode ${GNG}/1gms/vocab.gz 1gm-0000.bin" > ${jobfile}
for f in ${GNG}/[2-5]gms/*[0-9][0-9][0-9][0-9].gz; do
    binfile=`basename $f .gz`.bin
    echo "gng.encode $f $binfile" >> ${jobfile}
done
run-parallel.sh -N gng.encode $jobfile 30
gng.assemble
mv gng.tdx gng.dat ..
cd .. && rm -rf $tmpdir
