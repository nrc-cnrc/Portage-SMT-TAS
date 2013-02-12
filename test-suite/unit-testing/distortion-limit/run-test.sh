#!/bin/bash

make gitignore

i=1
for DIST_TYPE in "" -dist-limit-ext -dist-phrase-swap "-dist-limit-ext -dist-phrase-swap" -dist-limit-simple "-dist-limit-simple -dist-phrase-swap"; do
   echo "DIST_TYPE($i)=$DIST_TYPE"
   i=$((i+1))
done

echo $'DL\tDT(1)\tDT(2)\tDT(3)\tDT(4)\tDT(5)\tDT(6)'
for DL in 0 1 2 3 4 5 6 7 -1; do
   echo -n DL=$DL
   for DIST_TYPE in "" -dist-limit-ext -dist-phrase-swap "-dist-limit-ext -dist-phrase-swap" -dist-limit-simple "-dist-limit-simple -dist-phrase-swap"; do
      echo > out.0000.10000best
      canoe -f canoe.ini -input in -nbest out:10000 -b 1e-80 -rs 100000 \
         -distortion-limit $DL $DIST_TYPE >& /dev/null
      echo -n $'\t'
      echo -n `grep -v '^$' out.0000.10000best | wc -l`
      #echo $DIST_TYPE
   done
   echo ""
done | tee /dev/stderr > out

cat > ref <<EOF
DL=0	14	14	114	114	14	161
DL=1	14	14	114	114	82	217
DL=2	64	87	131	147	317	444
DL=3	239	344	300	412	791	986
DL=4	700	1059	806	1094	1871	1871
DL=5	1743	2872	1759	2873	3944	3944
DL=6	3844	6503	3876	6504	7688	7688
DL=7	7688	7688	7688	7688	7688	7688
DL=-1	7688	7688	7688	7688	7688	7688
EOF

echo diff-round.pl ref out
if diff-round.pl ref out; then
   echo All tests PASSED.
   exit 0
else
   echo FAILED.
   exit 1
fi

exit

for DIST_TYPE in "" -dist-limit-ext -dist-phrase-swap "-dist-limit-ext -dist-phrase-swap"; do
   echo DIST_TYPE=$DIST_TYPE
   for DL in 0 1 2 3 4 5 6 7 -1; do
      canoe -f canoe.ini -input in -nbest out:10000 -b 1e-30 -rs 100000 \
         -distortion-limit $DL $DIST_TYPE >& /dev/null
      echo -n DL=$DL$'\t'
      grep -v '^$' out.0000.10000best | wc -l
   done
done
