TEMP_FILES=wc_stats.*
TEMP_DIRS=tmx-prepro-test*
include ../Makefile.incl

all: wc_stats.test1 wc_stats.test2

wc_stats.%: %
	export LC_ALL=C; wc_stats tmx-prepro-$*/template/corpora/[^M]* | cut -f 1,11 | grep -v sdev > wc_stats.$*
	diff wc_stats.$* ref/

tmx-prepro-%/README:
	[[ -d ${PORTAGE}/tmx-prepro ]] && cp -ar ${PORTAGE}/tmx-prepro tmx-prepro-$* || git clone ${PORTAGE_GIT_ROOT}/PortageTMXPrepro.git tmx-prepro-$*

test1: %: tmx-prepro-%/README
	cp data/{h2,h3,hyphens,train_termino}.tmx tmx-prepro-$@/template/tmx/
	cd tmx-prepro-$@/template/preparation; make all DEV_SETS=dev1 SIZE.BIG=3 SIZE.SMALL=2 RSAMPLE_SIZE=1
	cd tmx-prepro-$@/template/corpora; make all DEV_SETS=dev1 SIZE.BIG=3 SIZE.SMALL=2 RSAMPLE_SIZE=1

test2: %: tmx-prepro-%/README
	cp data/{train_text,train_termino}.tmx tmx-prepro-$@/template/tmx/
	cd tmx-prepro-$@/template/preparation; make all DEV_SETS=dev1 SIZE.BIG=3 SIZE.SMALL=2 RSAMPLE_SIZE=1 corpus=train_text corpus.extra=train_termino
	cd tmx-prepro-$@/template/corpora; make all DEV_SETS=dev1 SIZE.BIG=3 SIZE.SMALL=2 RSAMPLE_SIZE=1 corpus=train_text corpus.extra=train_termino

