#!/bin/bash

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
