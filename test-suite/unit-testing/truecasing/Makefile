#!/usr/bin/make -f
# vim:noet:ts=3

#LIB       = PERL5LIB=~/sandboxes/quick.fix/src/truecasing:$$PERL5LIB
#TRUECASER = ~/sandboxes/quick.fix/src/truecasing/truecase.pl
TRUECASER = truecase.pl

VERBOSE = --verbose

MAP = tc-map.en
LM  = tc-lm.en

.NOTPARALLEL:
.SECONDARY:

all: canoe1_latin1
all: canoe1_utf8
all: canoe2_latin1
all: canoe2_utf8
# Note: Currently TPPTs support UTF8 only.
#all: tpt_latin1
all: tpt_utf8
#all: tpt_uBoS_latin1
all: tpt_uBoS_utf8
all: srilm_latin1
all: srilm_utf8
all: viterbi_latin1
all: viterbi_utf8

clean:
	${RM} -r ${LM}_*.tplm* ${MAP}_*.tppt*
	${RM} ref_* src_* ${LM}_* ${MAP}_* vocabMap2tpt.*

canoe1%: src% ref% ${LM}%.gz ${MAP}%.gz
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --lm=${LM}$*.gz --map=${MAP}$*.gz \
	| diff -q - ref$*

canoe2%: src% ref% ${LM}%.gz ${MAP}%
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --lm=${LM}$*.gz --map=${MAP}$* \
	| diff -q - ref$*

tpt%: src% ref% ${LM}%.tplm ${MAP}%.tppt
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --tplm=${LM}$*.tplm --tppt=${MAP}$*.tppt \
	| diff -q - ref$*

tpt_uBoS%: src% ref% ${LM}%.tplm ${MAP}%.tppt
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --tplm=${LM}$*.tplm --tppt=${MAP}$*.tppt --uppercaseBOS \
	| diff -q - ref$*

srilm%: src% ref% ${LM}% ${MAP}%
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --lm=${LM}$* --map=${MAP}$* --srilm \
	| diff -q - ref$*

viterbi%: src% ref% ${LM}% ${MAP}%
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --lm=${LM}$* --map=${MAP}$* --srilm --viterbi \
	| diff -q - ref$*


########################################
# HELPERS

#REF = This is a test .\nÉric lives in Québec .
#SRC = this is a test .\néric lives in québec .

# Make sure the correct latin1 text goes into the *_latin1 files.
REF = This is a test .\n\xC9ric lives in Qu\xE9bec .
SRC = this is a test .\n\xE9ric lives in qu\xE9bec .

# NOTE: The following SRC/REF text, which contains a sentence beginning with a
# word starting with an accented character that is unknown by the truecasing
# model, should work with --uppercaseBOS, but it does not:
#REF = \xC9tude is an accented word .
#SRC = \xE9tude is an accented word .

src_latin1:
	echo -e "${SRC}" > $@

ref_latin1:
	echo -e "${REF}" > $@

${LM}_latin1:
	echo -e "${REF}" | ngram-count -text - -order 3 -lm $@

${MAP}_latin1:
	echo -e\
		"a\tA\t0.0273266569645544\ta\t0.972673343035446\n"\
		"\xE9ric\t\xC9RIC\t0.0203383\t\xC9ric\t0.970669\t\xE9ric\t0.00521756\n"\
		"is\tIS\t5.80701487396775e-05\tIs\t0.0248139752821408\tis\t0.975127954569119\n"\
		"qu\xE9bec\tQU\xC9BEC\t0.0106383\tQu\xE9bec\t0.980439\tqu\xE9bec\t0.00514756\n"\
		"test\tTEST\t0.00020772746157042\tTest\t0.0305359368508517\ttest\t0.969256335687578\n"\
		"this\tTHIS\t5.95558008802638e-05\tThis\t0.170216289237831\tthis\t0.829724154961289\n" \
	> $@

src_utf8 ref_utf8 ${LM}_utf8 ${MAP}_utf8: %_utf8: %_latin1
	cat $< \
	| iconv -f latin1 -t UTF-8 \
	> $@

%.gz: %
	gzip -c $< > $@

%.tplm: %
	arpalm2tplm.sh -n 1 $<

%.tppt: %
	vocabMap2tpt.sh $<
