# @file Makefile Creates multiple kinds of nbest and make sure they are all equivalent
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

DIR ?= src_out
PORTAGE ?= /home/portage/i686-g3.4.6
CORPUS_DIR ?= ../../regress-small-voc
TRAIN ?= ${CORPUS_DIR}/lc/europarl.fr-en.fr.lowercase ${CORPUS_DIR}/lc/europarl.fr-en.en.lowercase
SRC ?= ${CORPUS_DIR}/lc/test2000.fr.lowercase
LM ?= ${CORPUS_DIR}/europarl.en.srilm

all: DIFF

clean:
	${RM} 1best.* nbest* lattice* ffvals* pal* para.* $(DIR)/* 
	${RM} run-parallel-logs-*
	
distclean: clean
	${RM} ibm1.fr_given_en.gz ibm2.fr_given_en.gz ibm1.en_given_fr.gz ibm2.en_given_fr.gz
	${RM} ibm2.fr_given_en.pos.gz ibm2.en_given_fr.pos.gz phrases-GT-KN.fr2en.gz
	${RM} canoe.ini
	${RM} log.gen* log.train*
	${RM} -r $(DIR)

############################################################
## SETUP
ibm2.fr_given_en.gz:
	LANG=en_US.ISO-8859-1; \
	train_ibm -vr -s       \
	  ibm1.fr_given_en.gz  \
	  ibm2.fr_given_en.gz  \
          ${TRAIN}
	  >& log.train_ibm.fr_given_en

ibm2.en_given_fr.gz:
	LANG=en_US.ISO-8859-1; \
	train_ibm -v  -s       \
	  ibm1.en_given_fr.gz  \
	  ibm2.en_given_fr.gz  \
          ${TRAIN}
	  >& log.train_ibm.en_given_fr

phrases-GT-KN.fr2en.gz: ibm2.fr_given_en.gz ibm2.en_given_fr.gz
	gen_phrase_tables -v -w1 -m8 -1 fr -2 en -ibm 2 -z \
	  -s GTSmoother -s KNSmoother   \
	  -multipr fwd                  \
	  -o phrases-GT-KN              \
	  ibm2.en_given_fr.gz           \
	  ibm2.fr_given_en.gz           \
          ${TRAIN}
	  >& log.gen_multi_prob_phrase_tables

canoe.ini:
	echo -e "[ttable-multi-prob] \n \
	phrases-GT-KN.fr2en.gz\n \
	[lmodel-file]\n \
	${LM}\n \
	[weight-d] 0.3200787604\n \
	[weight-w] 0.3201824725\n \
	[weight-l] 1\n \
	[weight-t] 0.4:1\n \
	[ttable-limit] 30\n \
	[ttable-prune-type] backward-weights\n\
	[distortion-limit] 7\n \
	[distortion-model] WordDisplacement\n \
	[segmentation-model] none#whatever\n" > $@

SETUP: canoe.ini phrases-GT-KN.fr2en.gz
	[ -e $(DIR) ] || mkdir $(DIR);


UNZIP: nbest $(DIR)/nbest
$(DIR)/nbest: canoe.ini
	canoe \
	  -append \
	  -f canoe.ini \
	  -lattice $(DIR)/lattice \
	  -nbest $(DIR)/nbest:88 \
	  -ffvals \
	  -palign \
	  < $(SRC) > 1best.append
nbest: canoe.ini
	canoe \
	  -f canoe.ini \
	  -lattice $(DIR)/lattice \
	  -nbest $(DIR)/nbest:88 \
	  -ffvals \
	  -palign \
	  < $(SRC) > 1best.default
	cat $(DIR)/nbest.????.88best.ffvals > ffvals
	cat $(DIR)/nbest.????.88best.pal > pal
	cat $(DIR)/lattice.???? > lattice
	cat $(DIR)/lattice.????.state > lattice.state
	cat $(DIR)/nbest.????.88best > $@

ZIP: nbest.gz $(DIR)/nbest.gz
$(DIR)/nbest.gz: canoe.ini
	canoe \
	  -append \
	  -f canoe.ini \
	  -lattice $(DIR)/lattice.gz \
	  -nbest $(DIR)/nbest.gz:88 \
	  -ffvals \
	  -palign \
	  < $(SRC) > 1best.append-gz
nbest.gz: canoe.ini
	canoe \
	  -f canoe.ini \
	  -lattice $(DIR)/lattice.gz \
	  -nbest $(DIR)/nbest.gz:88 \
	  -ffvals \
	  -palign \
	  < $(SRC) > 1best.default-gz
	cat $(DIR)/nbest.????.88best.ffvals.gz > ffvals.gz
	cat $(DIR)/nbest.????.88best.pal.gz > pal.gz
	cat $(DIR)/lattice.????.gz > lattice.gz
	cat $(DIR)/lattice.????.state.gz > lattice.state.gz
	cat $(DIR)/nbest.????.88best.gz > $@

para.nbest.gz: canoe.ini
	canoe-parallel.sh canoe \
	  -append \
	  -f canoe.ini \
	  -nbest para.nbest.gz:88 \
	  -ffvals \
	  -palign \
	  < $(SRC) > 1best.append.para

DIFF: ZIP UNZIP para.nbest.gz
	@echo "The following files must be identical:"
	@zdiff -sq ffvals ffvals.gz
	@zdiff -sq ffvals $(DIR)/nbest.88best.ffvals
	@zdiff -sq ffvals $(DIR)/nbest.88best.ffvals.gz
	@zdiff -sq pal pal.gz
	@zdiff -sq pal $(DIR)/nbest.88best.pal
	@zdiff -sq pal $(DIR)/nbest.88best.pal.gz
	@zdiff -sq lattice lattice.gz
	@zdiff -sq lattice $(DIR)/lattice
	@zdiff -sq lattice $(DIR)/lattice.gz
	@zdiff -sq lattice.state lattice.state.gz
	@zdiff -sq lattice.state $(DIR)/lattice.state
	@zdiff -sq lattice.state $(DIR)/lattice.state.gz
	@zdiff -sq nbest nbest.gz
	@zdiff -sq nbest $(DIR)/nbest.88best
	@zdiff -sq nbest $(DIR)/nbest.88best.gz
	@zdiff -sq 1best.default 1best.append
	@zdiff -sq 1best.default 1best.default-gz
	@zdiff -sq 1best.default 1best.append-gz
	@echo "Comparing the parallel run with based run"
	@zdiff -sq nbest para.nbestnbest.gz
	@zdiff -sq ffvals para.nbestffvals.gz
	@zdiff -sq pal para.nbestpal.gz
	@zdiff -sq 1best.default 1best.append.para

DIFF_ORIG: nbest
	@zdiff -sq nbest original.nbest.gz
