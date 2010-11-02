#!/usr/bin/make -f
# vim:noet:ts=3

#TRUECASER = perl -I ../../../src/truecasing ../../../src/truecasing/truecase.pl
TRUECASER = truecase.pl

UCBOS = --ucBOSEncoding $*
VERBOSE = --verbose

MAP = tc-map.en
LM  = tc-lm.en

QUIET = -q

.NOTPARALLEL:
.SECONDARY:

all: canoe1_cp1252
all: canoe1_utf8
all: canoe2_cp1252
all: canoe2_utf8
all: canoe_ucbos_cp1252
all: canoe_ucbos_utf8
all: tpt_cp1252
all: tpt_utf8
all: tpt_ucbos_cp1252
all: tpt_ucbos_utf8

clean:
	${RM} -r ${LM}_*.tplm* ${MAP}_*.tppt*
	${RM} ref_* src_* ${LM}_* ${MAP}_* vocabMap2tpt.*

canoe1_%: src_% ref_% ${LM}_% ${MAP}_%
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --lm=${LM}_$* --map=${MAP}_$* \
	| diff ${QUIET} - ref_$*

canoe2_%: src_% ref_% ${LM}_%.gz ${MAP}_%.gz
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --lm=${LM}_$*.gz --map=${MAP}_$*.gz \
	| diff ${QUIET} - ref_$*

canoe_ucbos_%: src_ucbos_% ref_ucbos_% ${LM}_%.gz ${MAP}_%.gz
	${LIB} ${TRUECASER} ${VERBOSE} ${UCBOS} --text=$< --lm=${LM}_$*.gz --map=${MAP}_$*.gz \
	| diff ${QUIET} - ref_ucbos_$*

tpt_%: src_% ref_% ${LM}_%.tplm ${MAP}_%.tppt
	${LIB} ${TRUECASER} ${VERBOSE} --text=$< --tplm=${LM}_$*.tplm --tppt=${MAP}_$*.tppt \
	| diff ${QUIET} - ref_$*

tpt_ucbos_%: src_ucbos_% ref_ucbos_% ${LM}_%.tplm ${MAP}_%.tppt
	${LIB} ${TRUECASER} ${VERBOSE} ${UCBOS} --text=$< --tplm=${LM}_$*.tplm --tppt=${MAP}_$*.tppt \
	| diff ${QUIET} - ref_ucbos_$*

########################################
# HELPERS

#REF_UCBOS = This is a test .\nÉric lives in Québec .\nÉtude is accented .
#SRC_UCBOS = this is a test . \néric lives in québec . \nétude is accented . 

# Make sure the correct windows-1252 (CP1252) text goes into the *_cp1252 files.
REF_UCBOS = This is a test .\n\xC9ric lives in Qu\xE9bec .\n\xC9tude is accented .
SRC_UCBOS = this is a test . \n\xE9ric lives in qu\xE9bec . \n\xC9tude is accented . 
REF = This is a test .\n\xC9ric lives in Qu\xE9bec .
SRC = this is a test . \n\xE9ric lives in qu\xE9bec . 

src_cp1252:
	env echo -e "${SRC}" > $@

ref_cp1252:
	env echo -e "${REF}" > $@

src_ucbos_cp1252:
	env echo -e "${SRC_UCBOS}" > $@

ref_ucbos_cp1252:
	env echo -e "${REF_UCBOS}" > $@

${LM}_cp1252:
	env echo -e "${REF}" | ngram-count -text - -order 3 -lm $@

${MAP}_cp1252:
	env echo -e\
		"a\tA\t0.0273266569645544\ta\t0.972673343035446\n"\
		"\xE9ric\t\xC9RIC\t0.0203383\t\xC9ric\t0.970669\t\xE9ric\t0.00521756\n"\
		"is\tIS\t5.80701487396775e-05\tIs\t0.0248139752821408\tis\t0.975127954569119\n"\
		"qu\xE9bec\tQU\xC9BEC\t0.0106383\tQu\xE9bec\t0.980439\tqu\xE9bec\t0.00514756\n"\
		"test\tTEST\t0.00020772746157042\tTest\t0.0305359368508517\ttest\t0.969256335687578\n"\
		"this\tTHIS\t5.95558008802638e-05\tThis\t0.170216289237831\tthis\t0.829724154961289\n" \
	> $@

src_utf8 ref_utf8 src_ucbos_utf8 ref_ucbos_utf8 ${LM}_utf8 ${MAP}_utf8: %_utf8: %_cp1252
	cat $< \
	| iconv -f windows-1252 -t UTF-8 \
	> $@

%.gz: %
	gzip -c $< > $@

%.tplm: %
	arpalm2tplm.sh -n 1 $<

%.tppt: %
	vocabMap2tpt.sh $<
