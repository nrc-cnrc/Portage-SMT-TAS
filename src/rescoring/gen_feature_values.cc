/**
 * @author George Foster
 * @file gen_feature_values.cc 
 * @brief Program that generates and outputs the value for a feature function
 * given a source and its nbest.
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

#include <exception_dump.h>
#include <arg_reader.h>
#include <logging.h>
#include <file_utils.h>
#include <featurefunction_set.h>
#include <rescore_io.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <printCopyright.h>

using namespace Portage;
using namespace std;

/// Program gen_feature_values usage.
static char help_message[] = "\n\
gen_feature_values [-v][-w][-a AF][-o OF][-n N][-min SINDEX][-max EINDEX]\n\
                   FEATURE ARG SRC NBEST\n\
\n\
  Generate values for FEATURE on a given SRC text and NBEST lists.\n\
  Values are written to stdout. Use rescore_train -H and -h for information\n\
  on features and file formats.\n\
\n\
Options:\n\
\n\
  -a   Read in phrase alignment file AF.\n\
  -o   Output feature file OF.\n\
  -n   Print features only for the N best sentences.\n\
  -w   Print feature values for each target word rather than one per sentence.\n\
       NOTE: Works only for features like word and phrase posterior probs.\n\
  -v   Write progress reports to cerr.\n\
  -min Start index to process\n\
  -max End index to process\n\
";

// globals

static bool verbose = false;
static string name;
static string argument;
static string src_file;
static string nbest_file;
static string alignment_file;
static string out_file = "-";
static Uint   printN = 0;
static bool   printWordVals = false;
static Uint   minSindex = numeric_limits<Uint>::min();
static Uint   maxSindex = numeric_limits<Uint>::max();

static void getArgs(int argc, const char *const argv[]);

// main

int MAIN(argc, argv)
{
   printCopyright(2005, "gen_feature_values");
   // Do this here until we use argProcessor for this program.
   Logging::init();

   getArgs(argc, argv);

   // Prepare the output stream.
   oMagicStream outstr(out_file);

   // Prepare the source sentences
   Sentences  src_sents;
   const Uint S  = RescoreIO::readSource(src_file, src_sents);
   const Uint KS = countFileLines(nbest_file);
   if (S == 0 && KS == 0) {
      error(ETWarn, "empty input files: %s, %s", src_file.c_str(), nbest_file.c_str());
      // This is not an error but we shall stop here.
      exit(0);
   }

   if ((S == 0 && KS > 0) or (S > 0 && KS == 0)) {
      error(ETFatal, "Problem with the input sizes (NB: %d, S: %d)", KS, S);
   }

   const Uint K = KS / S;
   if (K * S != KS) {
      error(ETFatal, "Nbest list's size(%d) is not a multiple of source's size(%d).", KS, S);
   }

   // Prepare the feature function
   // EJJ 11Jul2006
   // Since the ff gets deleted right at the end of the program, when we're
   // about to exit and have the OS clean up for us anyway, it's much faster if
   // we just exist without deleting the pointer.  So use a null deleter for
   // the ff!
   ptr_FF  ff(FeatureFunctionSet::create(name, argument, NULL, false, true));
   if (!ff)
      error(ETFatal, "unknown feature: %s", name.c_str());

   // If we need the tgtVocab, insert ff into an ffset, so that we can call
   // FeatureFunctionSet::createTgtVocab().
   FeatureFunctionSet ffset;
   if (ff->requires() & FF_NEEDS_TGT_VOCAB) {
      ffset.ff_infos.push_back(FeatureFunctionSet::ff_info(name+":"+argument, name, ff));
      ffset.createTgtVocab(src_sents, FileReader::create<Translation>(nbest_file, K));
      ff->addTgtVocab(ffset.tgt_vocab);
   }

   // Give the whole thing to the ff
   ff->init(&src_sents);


   // Prepare the alignment file
   iMagicStream astr;
   const bool bNeedAligment(ff->requires() & FF_NEEDS_ALIGNMENT);
   if (bNeedAligment) {
      if (verbose) cerr << "This feature requires alignment" << endl;
      if (!alignment_file.empty()) {
         astr.open(alignment_file.c_str());
         if (!astr) error(ETFatal, "unable to open alignment file %s", alignment_file.c_str());
      }
      else {
         error(ETFatal, "%s requires the alignment file", name.c_str());
      }
   }

   outstr << setprecision(10);
   NbestReader  pfr(FileReader::create<Translation>(nbest_file, K));
   vector<double> vals;
   vals.reserve(K);
   Uint s(0);
   for (; pfr->pollable(); ++s) {
      // READING NBEST
      Nbest nbest;
      pfr->poll(nbest);
      const Uint K(nbest.size());

      // READING ALIGNMENT
      vector<PhraseAlignment> alignments(K);
      Uint k(0);
      for (; bNeedAligment && k < K && alignments[k].read(astr); ++k) {
         nbest[k].phraseAlignment = alignments[k];
      }
      if (bNeedAligment && (k != K ))
         error(ETFatal, "unexpected end of nbests file after %d lines (expected %dx%d=%d lines)", s*K+k, S, K, S*K);

      if (minSindex <= s && s < maxSindex) {
         // Specify source and nbest to the ff and print values.
         ff->source(s, &nbest);
         Uint maxPrintN = K;
         if (printN>0)
            maxPrintN = min(printN, K);
         if (! printWordVals)
            for (Uint k = 0; k < maxPrintN; ++k)
               outstr << ff->value(k) << endl;
         else
            for (Uint k = 0; k < maxPrintN; ++k) {
               vals.clear();
               ff->values(k, vals);
               copy(vals.begin(), vals.end(), ostream_iterator<double>(outstr, " "));
               outstr << endl;
            }
      }
   }
   //cerr << "at end" << endl;
} END_MAIN

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "a:", "n:", "o:", "w", "min:", "max:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, 4, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("a", alignment_file);
   arg_reader.testAndSet("n", printN);
   arg_reader.testAndSet("w", printWordVals);
   arg_reader.testAndSet("o", out_file);
   arg_reader.testAndSet("min", minSindex);
   arg_reader.testAndSet("max", maxSindex);

   arg_reader.testAndSet(0, "feature", name);
   arg_reader.testAndSet(1, "arg", argument);
   arg_reader.testAndSet(2, "src", src_file);
   arg_reader.testAndSet(3, "nbest", nbest_file);
}
