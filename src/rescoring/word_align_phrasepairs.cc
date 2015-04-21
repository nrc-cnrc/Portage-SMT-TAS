/**
 * @author George Foster
 * @file word_align_phrasepairs.cc 
 * @brief Generate word alignments for given phrase alignments.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include "ibm.h"
#include "word_align.h"
#include "word_align_io.h"
#include "hmm_aligner.h"
#include "arg_reader.h"
#include "file_utils.h"
#include "parse_pal.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
word_align_phrasepairs [-v][-ibm t][-a 'meth args'][-o 'format'][-p ppnbest]\n\
                       ibm_tgt_given_src ibm_src_given_tgt src nbest pal [wal]\n\
\n\
Use IBM models to produce word alignments for phrase pairs occurring in the\n\
files <src> and <nbest>, as specified by phrase alignment file <pal>. Nbest \n\
lists are assumed to be fixed size. For each nbest line, a corresponding word\n\
alignment is written to <wal> in the format selected by the -o switch.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr\n\
-ibm  Type of ibm models given to the -ibm_* switches: 1, 2, or hmm. This\n\
      may be used to force an HMM ttable to be used as an IBM1, for example.\n\
      [determine type of model automatically from filename]\n\
-a    Use given word alignment method and arguments (gen_phrase_tables -H for\n\
      list) [IBMOchAligner]\n\
-o    Specify output format, one of: \n\
      "WORD_ALIGNMENT_WRITER_FORMATS" [gale]\n\
      The gale format is a special case: it requires -p to be specified.\n\
-p    Specify a postprocessed version of <nbest>, for use with gale output.\n\
";

// globals

static bool verbose = false;
static bool Verbose = false;
static bool force = false;
static string ibmtype;
static string align_method = "IBMOchAligner";
static string output_format = "gale";
static string ppnbestname;
static string ibm_tgt_given_src;
static string ibm_src_given_tgt;
static string srcname;
static string nbestname;
static string palname;
static string walname = "-";

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   // create IBM models if needed
   IBM1* ibm_1 = NULL;         // tgt_given_src
   IBM1* ibm_2 = NULL;         // src_given_tgt
   if (ibm_tgt_given_src != "" && ibm_src_given_tgt != "") {
      if (verbose) cerr << "reading IBM models" << endl;
      IBM1::createIBMModelPair(ibm_1, ibm_2, ibm_tgt_given_src, ibm_src_given_tgt,
                               ibmtype, verbose);
   }

   WordAlignerFactory alfactory(ibm_1, ibm_2, 0, false, false);
   WordAligner* aligner = alfactory.createAligner(align_method);

   WordAlignmentWriter* alwriter = WordAlignmentWriter::create(output_format);
   GALEWriter* galewriter = NULL;
   if (output_format == "gale") 
      galewriter = dynamic_cast<GALEWriter*>(alwriter);

   Uint S = countFileLines(srcname);
   Uint SK = countFileLines(nbestname);
   Uint K = SK / S;
   if (K * S != SK)
      error(ETFatal, "nbest list size is not an integral multiple of number of source lines");

   istream* ppnbest = NULL;
   if (ppnbestname != "")
      ppnbest = new iSafeMagicStream(ppnbestname);

   iSafeMagicStream src(srcname);
   iSafeMagicStream nbest(nbestname);
   iSafeMagicStream pal(palname);
   oSafeMagicStream wal(walname);

   vector<string> src_toks, src_phrase_toks;
   vector<string> tgt_toks, tgt_phrase_toks;
   vector<string> pp_tgt_toks;
   vector<PhrasePair> phrase_pairs;
   vector< vector<Uint> > src_sets;
   vector< vector<Uint> > src_phrase_sets;

   if (verbose) cerr << "writing alignments" << endl;

   Uint src_lineno = 0;
   string line, pal_line;
   while (getline(src, line)) {
      splitZ(line, src_toks);

      for (Uint k = 0; k < K; ++k) {

         assert(getline(nbest, line));
         splitZ(line, tgt_toks);

         if (ppnbest) {
            if (!getline(*ppnbest, line))
               error(ETFatal, "ppnbest file is too short");
            splitZ(line, pp_tgt_toks);
         }

         if (!getline(pal, pal_line)) error(ETFatal, "pal file is too short");

         if (tgt_toks.size()) { // nbest line not empty
            if (!parsePhraseAlign(pal_line, phrase_pairs))
               error(ETFatal, "format error at line %d in %s", 
		     src_lineno*K + k+1, palname.c_str());
	    sortBySource(phrase_pairs);
	    src_sets.clear();

            vector<PhrasePair>::iterator p;
            for (p = phrase_pairs.begin(); p != phrase_pairs.end(); ++p) {
               src_phrase_toks.assign(src_toks.begin() + p->src_pos.first, 
				      src_toks.begin() + p->src_pos.second);
               tgt_phrase_toks.assign(tgt_toks.begin() + p->tgt_pos.first, 
				      tgt_toks.begin() + p->tgt_pos.second);

	       if (src_phrase_toks.size() == 1 && tgt_phrase_toks.size() == 1) {
		  src_phrase_sets.resize(1);  // save some time for 1-1 phrase pairs
		  src_phrase_sets[0].assign(1, 0);
	       } else
		  aligner->align(src_phrase_toks, tgt_phrase_toks, src_phrase_sets);

	       // revise alignment: remove explicit null links and make
	       // positions absolute

	       src_phrase_sets.resize(src_phrase_toks.size());
	       for (Uint i = 0; i < src_phrase_sets.size(); ++i)
		  for (Uint j = 0; j < src_phrase_sets[i].size(); ++j) {
		     src_phrase_sets[i][j] += p->tgt_pos.first;
		     if (src_phrase_sets[i][j] == tgt_toks.size())
			src_phrase_sets[i].erase(src_phrase_sets[i].begin()+j);
		  }

	       src_sets.insert(src_sets.end(), 
			       src_phrase_sets.begin(), src_phrase_sets.end());

	    }

	    // write output alignments
            if (galewriter) {
               galewriter->sentence_id = src_lineno+1;
               galewriter->postproc_toks2 = &pp_tgt_toks;
               (*galewriter)(wal, src_toks, tgt_toks, src_sets);
	    } else
               (*alwriter)(wal, src_toks, tgt_toks, src_sets);
		       
	 } else
	    wal << endl;
      }
      ++src_lineno;
   }
	       
   if (getline(pal, line))
      error(ETFatal, "pal file is too long");
   
   if (verbose) cerr << endl << "done" << endl;

   delete alwriter;
   delete ppnbest;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "V", "f", "ibm:", "a:", "o:", "p:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 5, 6, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("V", Verbose);
   arg_reader.testAndSet("f", force);
   arg_reader.testAndSet("ibm", ibmtype);
   arg_reader.testAndSet("a", align_method);
   arg_reader.testAndSet("o", output_format);
   arg_reader.testAndSet("p", ppnbestname);

   if (output_format == "gale" && ppnbestname == "")
      error(ETFatal, "-p argument must be given with gale output");

   arg_reader.testAndSet(0, "ibm_tgt_given_src", ibm_tgt_given_src);
   arg_reader.testAndSet(1, "ibm_src_given_tgt", ibm_src_given_tgt);
   arg_reader.testAndSet(2, "src", srcname);
   arg_reader.testAndSet(3, "nbest", nbestname);
   arg_reader.testAndSet(4, "pal", palname);
   arg_reader.testAndSet(5, "wal", walname);
}
