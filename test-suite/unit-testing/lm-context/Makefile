SHELL:=/bin/bash
TEMP_FILES=*lat*.0000 *lat*.0000.state *000best* log.* out.* eval.*
TEMP_DIRS=*.tplm
include ../Makefile.incl
.SECONDARY:

##### test cases for -max-lm-context-size
all: $(foreach S,-1 0 1 2 3 4,check$S)

lat%.0000: canoe.ini cpt lm
	echo A B C | canoe -f canoe.ini -max-lm-context-size  $* -ffvals -v 3 \
	   -lattice lat$* -nbest nb$*:1000 \
	   > out.$* 2> log.$*

check%: lat%.0000
	diff <(li-sort.sh lat$*.0000) <(zcat ref/lat$*.0000.gz | li-sort.sh) -q
	diff <(li-sort.sh nb$*.0000.1000best) <(zcat ref/nb.0000.1000best.gz) -q


##### min-vs-nomin: test -minimize-lm-context-size
# the min-vs-nomin test makes sure that -minimize-lm-context-size makes the
# lattice smaller, by recombining more, but keeps the lattice equivalent,
# modulo pruning. This test case is devised so that no pruning happens at all
# (the stack size was set to be large enough to guarantee that).
# So we check that the n-best list is the same and that the lattices have the
# same total number of paths.

all: min-vs-nomin

# We'd want 3000000 for full testing, but it takes much too long to run
NBESTSIZE=100000
min.lat.0000: lm2.tplm canoe.ini cpt input
	time canoe -f canoe.ini -lmodel-file lm2.tplm -input input -minimize-lm-context-size -ffvals -v 2 -cube-pruning -nbest min:${NBESTSIZE} -trace -lattice min.lat -stack 20000  >& log.$@

nomin.lat.0000: lm2.tplm canoe.ini cpt input
	time canoe -f canoe.ini -lmodel-file lm2.tplm -input input -no-minimize-lm-context-size -ffvals -v 2 -cube-pruning -nbest nomin:${NBESTSIZE} -trace -lattice nomin.lat -stack 20000  >& log.$@

min-vs-nomin: min.lat.0000 nomin.lat.0000
#nbest equivalency:
	diff -q <(paste min.0000.${NBESTSIZE}best* | li-sort.sh) <(paste nomin.0000.${NBESTSIZE}best* | li-sort.sh)
#lattice equivalency test between min.lat.0000 and nomin.lat.0000: not perfect,
#but check at least that the total number of paths is the same:
	diff -w <(perl -e 'print int(0.5+exp('`java -jar ../../../bin/cherrycSMT.jar NumLatticePaths  min.lat.0000`')), "\n"') \
	        <(perl -e 'print int(0.5+exp('`java -jar ../../../bin/cherrycSMT.jar NumLatticePaths  nomin.lat.0000`')), "\n"')
	diff -w <(wc -l < min.lat.0000) <(echo 15139)
	diff -w <(wc -l < nomin.lat.0000) <(echo 31469)


##### Minor test cases that are useful too for -minimize-lm-context-size
all: min min2 min.bad

min.bad: canoe.ini cpt lm input
	! canoe -f canoe.ini -input input -minimize-lm-context-size -ffvals > out.$@ 2> log.$@

min: lm.tplm canoe.ini cpt input
	canoe -f canoe.ini -lmodel-file lm.tplm -input input -minimize-lm-context-size -ffvals > out.$@ 2> log.$@

min2: lm2.tplm canoe.ini cpt input
	canoe -f canoe.ini -lmodel-file lm2.tplm -input input -minimize-lm-context-size -ffvals > out.$@ 2> log.$@


##### Utility code to setup this test suite

create-lm:
	( echo 'a b c d e f'; for s in `seq 1 20`; do echo 'a b c d e f' | tr ' ' '\n' | shuf | tr '\n' ' '; echo; done ) | estimate-ngram -t /dev/stdin -wl lm-new -o 5


##### Code to troubleshoot a TPLM bug discovered while creating this test suite
#all:tplm-bug

eval.%.out: %
	lm_eval -v $* lm-input > $@ 2> log.$@

tplm-diff-%: eval.lm%.out eval.lm%.tplm.out
	diff $+

tplm-bug: tplm-diff-3 tplm-diff-2 tplm-diff-1

%.tplm: %
	arpalm2tplm.sh $< >& log.$@

