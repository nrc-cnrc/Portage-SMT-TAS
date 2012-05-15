/**
 * @author George Foster
 * @file phrase_oracle.cc 
 * @brief Program that derives an oracle joint phrase table from a source text
 * and its reference translations, using an Och-style phrase induction
 * algorithm.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <map>
#include <voc.h>
#include <file_utils.h>
#include <arg_reader.h>
#include <printCopyright.h>
#include "ibm.h"
#include "word_align.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
phrase_oracle [-Hvx][-a 'meth args'][-c cf][-ibm n][-m maxlen][-min minlen]\n\
              [-d ldiff][-p pt]\n\
              ibm.src_given_tgt ibm.tgt_given_src src ref1 ref2 .. refn\n\
\n\
Derive an oracle joint phrase table from a source text and its reference\n\
translations, using an Och-style phrase induction algorithm. Optionally\n\
filter the table to contain only entries found in a static phrasetable.\n\
Result is written to stdout.\n\
\n\
Options:\n\
\n\
-H   List available word-alignment methods and quit.\n\
-v   Write progress reports to cerr. Use -vv to get more.\n\
-x   Write tgt phrases first in output table. Use this if tgt language code\n\
     (eg 'en', 'fr', etc) comes before the src language code in lexicographic\n\
     order, to maintain the standard convention for joint phrasetables. This\n\
     also causes -p to expect tgt phrases first. [write src first]\n\
-a   Main word-alignment method and optional args. Use -H for help.\n\
     [IBMOchAligner 2]\n\
-c   Interpolate primary phrases with Cartesian product over phrase pairs,\n\
     using a coefficient of cf on the cartesian pairs (and 1-cf on primary\n\
     pairs) [0]\n\
-ibm Specify which IBM model to use for word alignment: 1 or 2 [2]\n\
-m   Maximum phrase length (applies to src and refs only) [8]\n\
-min Minimum phrase length (applies to src and refs only) [1]\n\
-d   Max permissible difference in number of words between source and target\n\
     phrases. [4]\n\
-p   Restrict output phrases to ones that occur in <pt>, which may be either\n\
     joint or conditional, but must have src phrases in left column (unless -x\n\
     given).\n\
\n\
";

// globals

static Uint verbose = 0;
static bool tgt_first = false;
static string align_spec = "IBMOchAligner 1";
static float cart_coeff = 0.0;
static Uint ibm_num = 2;
static Uint maxlen = 8;
static Uint minlen = 1;
static Uint maxdiff = 4;
static string extern_phrasetable_fname;

static string ibm_src_given_tgt_fname;
static string ibm_tgt_given_src_fname;
static string srcfname;
static vector<string> refnames;

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "phrase_oracle");
   getArgs(argc, argv);

   if (maxlen == 0) maxlen = 10000000;
   if (minlen == 0)  {
     minlen = 1;
     cerr << "minimal phrase length has to be at least 1 -> changing this!" << endl;
   }
   if ( minlen > maxlen ) {
     cerr << "Minimal phrase length is greater than the maximal one! This doesn't make sense -> exit!!!" << endl
          << "min: " << minlen << " max: " << maxlen << endl;
     exit(1);
   }

   IBM1* ibm_src_given_tgt;
   IBM1* ibm_tgt_given_src;

   if (verbose) cerr << "loading models..." << endl;
   if (ibm_num == 1) {
      ibm_src_given_tgt = new IBM1(ibm_src_given_tgt_fname);
      ibm_tgt_given_src = new IBM1(ibm_tgt_given_src_fname);
   } else {
      ibm_src_given_tgt = new IBM2(ibm_src_given_tgt_fname);
      ibm_tgt_given_src = new IBM2(ibm_tgt_given_src_fname);
   }

   WordAlignerFactory factory(ibm_src_given_tgt, ibm_tgt_given_src, verbose, false, false);
   vector<string> toks;
   split(align_spec, toks, " \n\t", 2);
   toks.resize(2);
   WordAligner* main_aligner = factory.createAligner(toks[0], toks[1]);
   assert(main_aligner);
   WordAligner* cart_aligner = factory.createAligner("CartesianAligner", "");
   assert(cart_aligner);

   if (verbose) cerr << "models loaded" << endl;

   // build phrase table from contents of src and ref files

   PhraseTableGen<float> pt;
   for (Uint i = 0; i < refnames.size(); ++i) {

      iSafeMagicStream src(srcfname);
      iSafeMagicStream ref(refnames[i]);

      Uint line_no = 0;
      string src_line, ref_line;
      vector<string> src_toks, ref_toks;
      vector< vector<Uint> > src_sets;

      while (getline(src, src_line)) {
         if (!getline(ref, ref_line)) {
            error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
                  srcfname.c_str(), refnames[i].c_str());
            break;
         }
         ++line_no;

         if (verbose > 1) cerr << "--- " << line_no << " ---" << endl;

         src_toks.clear(); ref_toks.clear();
         split(src_line, src_toks);
         split(ref_line, ref_toks);

         if (verbose > 1)
            cerr << src_line << endl << ref_line << endl;

         main_aligner->align(src_toks, ref_toks, src_sets);
         factory.addPhrases(src_toks, ref_toks, src_sets, maxlen, maxlen, maxdiff,
                         minlen, minlen, pt, (float(1.0)-cart_coeff));

         if (cart_coeff != 0) {
            cart_aligner->align(src_toks, ref_toks, src_sets);
            factory.addPhrases(src_toks, ref_toks, src_sets, maxlen, maxlen, maxdiff,
                            minlen, minlen, pt, cart_coeff);
         }

         if (verbose == 1 && line_no % 1000 == 0)
            cerr << "line: " << line_no << endl;
      }

      if (getline(ref, ref_line))
         error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
               srcfname.c_str(), refnames[i].c_str());
   }

   // Filter - keep only phrase pairs that are in external phrase table, then
   // write table to stdout.

   if (verbose)
      cerr << (extern_phrasetable_fname == "" ? "not " : "") << "filtering phrase table" << endl;

   PhraseTableGen<float> pt_filt;

   if (extern_phrasetable_fname != "") {
      iSafeMagicStream ifs(extern_phrasetable_fname);

      vector<string>::const_iterator b1, e1, b2, e2, v;
      vector<string> toks;
      string line;
      while (getline(ifs, line)) {
         if (!tgt_first) PhraseTableBase::extractTokens(line, toks, b1, e1, b2, e2, v);
         else PhraseTableBase::extractTokens(line, toks, b2, e2, b1, e1, v);
         float freq;
         if (pt.exists(b1, e1, b2, e2, freq))
            pt_filt.addPhrasePair(b1, e1, b2, e2, freq);
      }
      pt_filt.dump_joint_freqs(cout, 0, tgt_first);
   } else
      pt.dump_joint_freqs(cout, 0, tgt_first);
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "vv", "x", "c:", "a:", "ibm:", "m:", "min:", "d:", "p:"};

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, -1, help_message, "-h", true,
                        WordAlignerFactory::help().c_str(), "-H");
   arg_reader.read(argc-1, argv+1);

   if (arg_reader.getSwitch("v")) verbose = 1;
   if (arg_reader.getSwitch("vv")) verbose = 2;

   arg_reader.testAndSet("x", tgt_first);
   arg_reader.testAndSet("a", align_spec);
   arg_reader.testAndSet("c", cart_coeff);
   arg_reader.testAndSet("ibm", ibm_num);
   arg_reader.testAndSet("m", maxlen);
   arg_reader.testAndSet("min", minlen);
   arg_reader.testAndSet("d", maxdiff);
   arg_reader.testAndSet("p", extern_phrasetable_fname);

   arg_reader.testAndSet(0, "ibm_src_given_tgt", ibm_src_given_tgt_fname);
   arg_reader.testAndSet(1, "ibm_tgt_given_src", ibm_tgt_given_src_fname);

   arg_reader.testAndSet(2, "src", srcfname);
   arg_reader.getVars(3, refnames);
}
