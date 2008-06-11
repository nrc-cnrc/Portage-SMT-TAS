/**
 * @author Samuel Larkin based on Bruno Laferriere
 * @file lm_eval.cc  Program for testing language model implementations and
 *                   their funtionalities.
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lm.h"
#include "arg_reader.h"
#include "exception_dump.h"
#include "printCopyright.h"
#include "str_utils.h"
#include "file_utils.h"
#include "portage_defs.h"
//#include <dmalloc.h>
#include <fstream>
#include <vector>
#include <time.h>
#include <cmath>

using namespace std;
using namespace Portage;

static char help_message[] = "\n\
lm_eval lmfile testfile\n\
\n\
Test program for LMText, LMBin and any new LM type added.\n\
The type of LM created depends on the file name <lmfile>.\n\
\n\
Options:\n\
\n\
 -v     verbose output\n\
 -q     quiet output (only print global prob)\n\
 -limit limit vocab (load all input sentences before processing)\n\
 -per-sent-limit  limit vocab on a per input sentence basis (implies -limit)\n\
\n\
";

//global

static string lm_filename;
static string test_filename;
static bool verbose;
static bool quiet;
static bool limit_vocab = false;
static bool per_sent_limit = false;
static const float LOG_ALMOST_0 = -18;
static Uint order = 3;

// Function declarations
/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  the command argument vector
 */
static void getArgs(int argc, const char* const argv[]);
/**
 * Efficient line parser to process/calculate one line on the source input.
 * @param line      line to process
 * @param lm        language model object
 * @param voc       vocabulairy index
 * @param num_toks  total number of tokens found in line
 * @param Noov      number of oovs.
 * @return Returns the logprob for line
 */
static float processOneLine_fast(const string& line, PLM* lm, Voc& voc, Uint& num_toks, Uint& Noov);

int MAIN(argc,argv)
{
   printCopyright(2006, "lm_eval");

   getArgs(argc, argv);

   const Uint precision = cout.precision();
   cout.precision(6);
   if (per_sent_limit) limit_vocab = true;

   time_t start = time(NULL);             //Time count

   const Uint line_count = countFileLines(test_filename);
   if (verbose) fprintf(stderr, "the test file contains %d lines\n", line_count);

   // Creating the vocabulary filter
   VocabFilter vocab(per_sent_limit ? line_count : 0);
   if ( limit_vocab || per_sent_limit) {
      if (per_sent_limit) cerr << "Filtering on per sentence vocab" << endl;
      else cerr << "Filtering on vocab" << endl;

      VocabFilter::addConverter  aConverter(vocab, per_sent_limit);

      string line;
      iSafeMagicStream in(test_filename);
      while (getline(in, line)) {
         if(line.empty()) continue;

         vector<Uint> dummy;
         split(line.c_str(), dummy, aConverter);
         aConverter.next();  // Tells the converter we are processing the next line
      }
      cerr << "Read input (cumul time: "
           << (time(NULL) - start) << " secs)" << endl;
   }

   // Creating the language model object
   time_t start_lm(time(NULL));
   PLM* lm = PLM::Create(lm_filename, &vocab, PLM::SimpleAutoVoc, -INFINITY,
                         limit_vocab, 0, NULL);
   if (lm == NULL) {
      error(ETFatal, "Unable to instanciate lm");
   }

   vocab.freePerSentenceData();
   order = lm->getOrder();
   cerr << "Loaded " << order << "-gram model in "
      << (time(NULL) - start_lm) << "s (cumul time: "
      << (time(NULL) - start) << " secs)" << endl;

   string line;

   iSafeMagicStream testfile(test_filename);
   float docLogProb = 0.0;
   Uint num_toks = 0;
   Uint processed = 0;
   Uint Noov = 0;
   while(getline(testfile, line)) {
      if(line.empty()) continue;
      docLogProb += processOneLine_fast(line, lm, vocab, num_toks, Noov);
      //vector<string> words;
      //split(line,words," ");
      //cout << "----------------------"<< endl << "\x1b[31m" << line << "\x1b[0m\n" << endl;
      //processOneLine(words,lm,vocab);
      ++processed;
   }

   if ( quiet || verbose ) {
      //cout << "Document logProb = " << docLogProb << ", ppx = " << exp(-docLogProb / num_toks) << endl << endl;
      // WARNING: since our lm models are using log_10 instead of ln for probs,
      // we must make sure to use 10^H(p) when calculating the perplexity.
      cout << "Document contains " 
           << line_count << " sentences, " 
           << num_toks << " words, " 
           << Noov << " words ignored" << endl;
      cout << "Document logProb=" << docLogProb 
           << " ppl=" << pow(10, (-docLogProb / (num_toks+line_count-Noov)))
           << " ppl1=" << pow(10, (-docLogProb / (num_toks-Noov))) << endl << endl;
   }

   cerr << "End (Total time: " << (time(NULL) - start) << " secs)" << endl;

   Uint seconds =(Uint)difftime(time(NULL), start); //Number of seconds elapsed

   Uint mins = seconds / 60;
   Uint hours = mins/60;
   mins = mins%60;
   seconds = seconds % 60;

   cerr << "Program done in " << hours << "h" << mins << "m" << seconds << "s" << endl;

   cout.precision(precision);
   testfile.close();

   // For debugging purposes, it's best to delete lm, but we don't normally do it.
   //delete lm;
} END_MAIN



//************************************************Functions*******************************************************//
// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "limit", "q", "per-sent-limit"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("q", quiet);
   if ( quiet ) verbose = false;
   arg_reader.testAndSet("limit", limit_vocab);
   arg_reader.testAndSet("per-sent-limit", per_sent_limit);
   arg_reader.testAndSet(0, "lm_filename", lm_filename);
   arg_reader.testAndSet(1, "test_filename", test_filename);
}


/**
 * Calculates the prob for a word in context.
 * P(word | context)
 * @param word      word
 * @param context   context for word to be evaluated in
 * @param lm        language model object
 * @param voc       vocabulairy object
 * @return Returns the prob of word in context P(word | context)
 */
inline float getProb(Uint word, Uint context[], PLM* lm, const Voc& voc)
{
   const float wordLogProb = lm->wordProb(word, context, order-1);
   if (verbose) cout << "p( " << voc.word(word) << " | "
                     << voc.word(context[0]) << " ...)\t= ";
   if ( !quiet ) cout << wordLogProb << endl;

   return  (( wordLogProb != -INFINITY ) ? wordLogProb : 0.0f);
}   


float processOneLine_fast(const string& line, PLM* lm, Voc& voc, Uint& num_toks, Uint& Noov)
{
   float sentLogProb = 0.0;
   char buf[line.length() + 1];
   Uint context[order - 1];
   for ( Uint i = 0; i < order - 1; ++i ) context[i] = voc.index(PLM::SentStart);
   strcpy(buf, line.c_str());
   char* strtok_state; // state variable for strtok_r
   char* tok = strtok_r(buf, " ", &strtok_state);
   while (tok != NULL) {
      ++num_toks;
      const Uint word = limit_vocab ? voc.index(tok) : voc.add(tok);
      const float wordLogProb = getProb(word, context, lm, voc);
      sentLogProb += wordLogProb;
      if (wordLogProb == 0.0f)
         ++Noov;

      // Shift the context
      for ( Uint i = order - 1; i > 0; --i ) context[i] = context[i-1];
      context[0] = word;

      tok = strtok_r(NULL, " ", &strtok_state);
   }
   // Do the end of sentence
   sentLogProb += getProb(voc.index(PLM::SentEnd), context, lm, voc);

   if (verbose)
      cout << "logProb = " << sentLogProb << endl << endl;

   return sentLogProb;
}
