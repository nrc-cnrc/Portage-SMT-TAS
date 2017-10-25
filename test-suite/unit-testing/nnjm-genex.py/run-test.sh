#!/bin/bash

set -e

make clean
make gitignore
mkdir -p out

# Do the right thing if the source and alignment are empty

nnjm-genex.py -voc out/voc1 input/ex1.{f,e,wal} > out/ex1.out
diff -q {out,ref}/ex1.out
diff -q {out,ref}/voc1.src
diff -q {out,ref}/voc1.tgt
diff -q {out,ref}/voc1.out
echo "PASS: empty source and alignment"

# Standard usage, no tags files

nnjm-genex.py -voc out/voc2 input/ex2.{f,e,wal} > out/ex2.out
diff -q {out,ref}/ex2.out 
diff -q {out,ref}/voc2.src
diff -q {out,ref}/voc2.tgt
diff -q {out,ref}/voc2.out
echo "PASS: no tags files"

# Small voc to force tag usage

nnjm-genex.py -voc out/voc3 \
   -isv 5  -itv 5 -ov 10 -stag input/ex3.f.tag -ttag input/ex3.e.tag \
   input/ex3.{f,e,wal} > out/ex3.out
diff -q {out,ref}/ex3.out
diff -q {out,ref}/voc3.src
diff -q {out,ref}/voc3.tgt
diff -q {out,ref}/voc3.out
echo "PASS: tags with small voc sizes"

# Read-voc mode

nnjm-genex.py -r -voc ref/voc3 -stag input/ex3.f.tag -ttag input/ex3.e.tag \
   input/ex3.{f,e,wal} > out/ex4.out
diff -q {out,ref}/ex4.out
echo "PASS: read voc mode"
