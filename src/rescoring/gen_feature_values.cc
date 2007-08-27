/**
 * @author George Foster
 * @file gen_feature_values.cc  Program that generates and outputs the value
 * for a feature function given a source and its nbest.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <exception_dump.h>
#include <arg_reader.h>
#include <file_utils.h>
#include <featurefunction.h>
#include <rescore_io.h>
#include <basic_data_structure.h>
#include <fileReader.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <printCopyright.h>

using namespace Portage;
using namespace std;

/// Program gen_feature_values usage.
static char help_message[] = "\n\
gen_feature_values [-v][-a F][-o F][-n N] feature arg src nbest\n\
\n\
Generate values for a feature on a given source text and nbest lists.\n\
Values are written to stdout. Use rescore_train -H and -h for information\n\
on features and file formats.\n\
\n\
Options:\n\
\n\
-a  Read in phrase alignment file F.\n\
-o  Output feature file F.\n\
-n  Print features only for the N best sentences.\n\
-v  Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string name;
static string argument;
static string src_file;
static string nbest_file;
static string alignment_file;
static string out_file;
static Uint   printN=0;

static void getArgs(int argc, const char *const argv[]);

// main

int MAIN(argc, argv)
{
   printCopyright(2005, "gen_feature_values");
   // Do this here until we use argProcessor for this program.
   Logging::init();

   getArgs(argc, argv);

   // EJJ 11Jul2006
   // Since the ff gets deleted right at the end of the program, when we're
   // about to exit and have the OS clean up for us anyway, it's much faster if
   // we just exist without deleting the pointer.  So use a null deleter for
   // the ff!
   ptr_FF  ff(FeatureFunctionSet::create(name, argument, NULL, false, true));
   if (!ff)
      error(ETFatal, "unknown feature: %s", name.c_str());


   Sentences  src_sents;
   const Uint S  = RescoreIO::readSource(src_file, src_sents);
   if (S == 0)
      error(ETFatal, "empty source file: %s", src_file.c_str());
   const Uint KS = countFileLines(nbest_file);
   const Uint K = KS / S;


   iMagicStream astr;
   if (!alignment_file.empty()) {
      astr.open(alignment_file.c_str());
      if (!astr) error(ETFatal, "unable to open alignment file %s", alignment_file.c_str());
   }
       
   oMagicStream outstr("-");
   if (!out_file.empty())
     outstr.open(out_file.c_str());

   outstr << setprecision(10);
   NbestReader  pfr(FileReader::create<Translation>(nbest_file, S, K));
   Uint s(0);
   for (; pfr->pollable(); ++s) {
      // READING NBEST
      Nbest nbest;
      pfr->poll(nbest);
      const Uint K(nbest.size());
      
      // READING ALIGNMENT
      vector<Alignment> alignments(K);
      Uint k(0);
      for (; !alignment_file.empty() && k < K && alignments[k].read(astr); ++k) {
          nbest[k].alignment = &alignments[k];
      }
      if (!alignment_file.empty() && (k != K )) error(ETFatal, "unexpected end of nbests file after %d lines (expected %dx%d=%d lines)", s*K+k, S, K, S*K);
                                                         
      ff->init(&src_sents, K);   // Give the whole thing to the ff
                              
    
     // Specify source and nbest to the ff and print values.
     ff->source(s, &nbest);
     Uint maxPrintN = K;
     if (printN>0)
       maxPrintN = min(printN, K);
     for (Uint k = 0; k < maxPrintN; ++k)
       outstr << ff->value(k) << endl;
   }
   //cerr << "at end" << endl;
} END_MAIN

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* const switches[] = {"v", "a:", "n:", "o:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, 4, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("a", alignment_file);
   arg_reader.testAndSet("n", printN);
   arg_reader.testAndSet("o", out_file);

   arg_reader.testAndSet(0, "feature", name);
   arg_reader.testAndSet(1, "arg", argument);
   arg_reader.testAndSet(2, "src", src_file);
   arg_reader.testAndSet(3, "nbest", nbest_file);

}
