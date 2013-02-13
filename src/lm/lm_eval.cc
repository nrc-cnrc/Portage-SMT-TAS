/**
 * @author Samuel Larkin based on Bruno Laferriere
 * @file lm_eval.cc 
 * @brief Program for testing language model implementations and their
 * funtionalities.
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lm.h"
#include "lmtrie.h"
#include "arg_reader.h"
#include "exception_dump.h"
#include "printCopyright.h"
#include "str_utils.h"
#include "file_utils.h"
#include "portage_defs.h"
#include <vector>
#include <time.h>

using namespace std;
using namespace Portage;

static char help_message[] = "\n\
lm_eval [options] LMFILE TESTFILE\n\
\n\
  General program for using or testing LMs in any format supported by PortageII.\n\
  By default, outputs the log-prob of each word in TESTFILE.\n\
\n\
Options:\n\
\n\
  -v               verbose: output word, sentence and document log-prob, as\n\
                   well as document perplexity and a trace of each word query\n\
  -sent            sentence: only output sentence log-probs.\n\
  -q|-ppl          quiet: only output document log-prob and perplexity\n\
  -ppls            write perplexity (counting eos) instead of logprob for -sent\n\
  -p-unk P_UNK     if LMFILE is a closed-vocabulary LM (i.e., P(<unk>) is not\n\
                   defined), set P(<unk>) = P_UNK. [0]\n\
  -log10-p-unk LOG10_P_UNK  equivalent to -p-unk 10 ** LOG10_P_UNK [-infinity]\n\
\n\
  -voc-type TYPE   specify the OOV handling method [SimpleAutoVoc]:\n\
      ClosedVoc      OOVs get a unigram prob of 0 (or P_UNK if specified).\n\
      SimpleOpenVoc  OOVs get the unigram prob P(<unk>) as indicated in LMFILE\n\
                     (LMFILE must specify a probability for <unk>) but OOVs in\n\
                     the context are not handled.  I.e., if LMFILE provides\n\
                     probability estimates or back-off weights for n-grams with\n\
                     <unk> in the context or for n-grams with n>1, they will be\n\
                     ignored.\n\
      SimpleAutoVoc  Use ClosedVoc or SimpleAutoVoc based on whether LMFILE has\n\
                     a probability estimate for P(<unk>).\n\
      FullOpenVoc    Fully handle OOVs, even in the context of a query. (Slower,\n\
                     only use this if LMFILE provides more than just P(<unk>)).\n\
\n\
  -limit           limit vocab (load all input sentences before processing)\n\
  -per-sent-limit  limit vocab on a per input sentence basis (implies -limit)\n\
  -order ORDER     limit the LM order to ORDER [use LMFILE's true order]\n\
  -final-cleanup   Delete models at the end - use for leak detection.\n\
\n\
Note:\n\
\n\
  To get per-sentence log probs, you can use -sent or gen_feature_values:\n\
    lm_eval -sent LMFILE TESTFILE\n\
  or\n\
    gen_feature_values NgramFF LMFILE TESTFILE TESTFILE\n\
  Caveat: lm_eval ignores OOVs, thus effectively giving them a prob of 1;\n\
  gen_feature_values gives OOVs a very small probability (log prob = -18).\n\
  Specify -p-unk or -log10-p-unk to override this behaviour in lm_eval.\n\
  Both programs will handle OOVs correctly if your language model is\n\
  open-vocabulary (i.e., if it has a probability estimate for <unk>).\n\
\n\
";

//global

static string lm_filename;
static string test_filename;
static bool verbose = false;
static bool quiet = false;
static bool ppls = false;
static bool sent = false;
static double p_unk = 0;
static double log10_p_unk = -INFINITY;
static string voc_type = "";
static bool limit_vocab = false;
static bool per_sent_limit = false;
static const float LOG_ALMOST_0 = -18;
static Uint order = 3;
static Uint limit_order = 0;
static bool final_cleanup = false;

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

   //cout.precision(6);
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
   bool valid_voc_type = false;
   PLM::OOVHandling VocType(voc_type, valid_voc_type);
   assert(valid_voc_type);
   PLM* lm = PLM::Create(lm_filename, &vocab, VocType, log10_p_unk,
                         limit_vocab, limit_order, NULL);
   if (!lm)
      error(ETFatal, "Unable to instanciate lm");

   vocab.freePerSentenceData();
   order = lm->getOrder();
   cerr << "Loaded " << order << "-gram model in "
      << (time(NULL) - start_lm) << "s (cumul time: "
      << (time(NULL) - start) << " secs)" << endl;

   string line;

   iSafeMagicStream testfile(test_filename);
   float docLogProb = 0.0;
   Uint num_toks = 0;
   Uint lineno = 0;
   Uint Noov = 0;
   while(getline(testfile, line)) {
      if(line.empty()) {
         if (sent) cout << (ppls ? "1.0" : "0.0") << endl;
         ++lineno;
         continue;
      }

      vector<string> dummy;
      lm->newSrcSent(dummy, lineno);

      docLogProb += processOneLine_fast(line, lm, vocab, num_toks, Noov);
      //vector<string> words;
      //split(line,words," ");
      //cout << "----------------------"<< endl << "\x1b[31m" << line << "\x1b[0m\n" << endl;
      //processOneLine(words,lm,vocab);
      ++lineno;
   }

   cerr << "End (Total time: " << (time(NULL) - start) << " secs)" << endl;

   if ( quiet || verbose ) {
      // WARNING: since our lm models are using log_10 instead of ln for probs,
      // we must make sure to use 10^H(p) when calculating the perplexity.
      cout << "Document contains " 
           << line_count << " sentences, " 
           << num_toks << " words, " 
           << Noov << " words ignored" << endl;
      cout << "Document logProb=" << docLogProb 
           << " ppl=" << pow(10, (-docLogProb / (num_toks+line_count-Noov)))
           << " ppl1=" << pow(10, (-docLogProb / (num_toks-Noov))) << endl;
      lm->getHits().display(cout);
      cout << endl;
   }

   if ( verbose ) {
      // Ugly to downcast but here we really use particularities of subclasses.
      LMTrie* lmtrie = dynamic_cast<LMTrie*>(lm);
      if (lmtrie != NULL) lmtrie->displayStats();
   }

   // For debugging purposes, it's best to delete lm, but we don't normally do it.
   if ( final_cleanup ) delete lm;
} END_MAIN



//************************************************Functions*******************************************************//
// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* const switches[] = {
      "v", "q", "ppl", "sent", "ppls", "limit", "per-sent-limit", "order:",
      "log10-p-unk:", "p-unk:", "voc-type:",
      "final-cleanup",
   };
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("q", quiet);
   arg_reader.testAndSet("ppl", quiet);
   arg_reader.testAndSet("ppls", ppls);
   if ( quiet ) verbose = false;
   arg_reader.testAndSet("sent", sent);
   arg_reader.testAndSet("log10-p-unk", log10_p_unk);
   arg_reader.testAndSet("p-unk", p_unk);
   arg_reader.testAndSet("order", limit_order);
   arg_reader.testAndSet("voc-type", voc_type);
   arg_reader.testAndSet("limit", limit_vocab);
   arg_reader.testAndSet("per-sent-limit", per_sent_limit);
   arg_reader.testAndSet("final-cleanup", final_cleanup);
   arg_reader.testAndSet(0, "lm_filename", lm_filename);
   arg_reader.testAndSet(1, "test_filename", test_filename);

   if ( p_unk < 0 || p_unk > 1 )
      error(ETFatal, "P_UNK must be between 0 and 1.");
   if ( log10_p_unk > 0 )
      error(ETFatal, "LOG10_P_UNK must be <= 0.");

   if ( p_unk > 0 )
      log10_p_unk = log10(p_unk);

   if ( test_filename == "-" || test_filename == "/dev/stdin" )
      error(ETFatal, "TESTFILE cannot be stdin, since it is read multiple times.");

   if ( !voc_type.empty() ) {
      bool valid = false;
      PLM::OOVHandling VocType(voc_type, valid);
      if ( ! valid )
         error(ETFatal, "%s is not a valid -voc-type value", voc_type.c_str());
   } else {
      voc_type = "SimpleAutoVoc";
   }
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
   if ( verbose) cout << "p( " << voc.word(word) << " | "
                      << voc.word(context[0]) << " ...)\t= ";
   if ( !quiet && !sent ) cout << wordLogProb;
   if ( verbose) cout << " [" << lm->getHits().getLatestHit() << "-gram]";
   if ( !quiet && !sent ) cout << endl;

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
   Uint nt = num_toks;
   Uint noov = Noov;
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
   nt = num_toks - nt; // number of toks in sentence
   noov = Noov - noov; // number of oovs in sentence
   // Do the end of sentence
   sentLogProb += getProb(voc.index(PLM::SentEnd), context, lm, voc);

   if (verbose)
      cout << "logProb = " << sentLogProb << endl << endl;
   else if (sent) {
      if (ppls) 
         cout << pow(10, (-sentLogProb / (nt + 1 - noov))) << endl;
      else
         cout << sentLogProb << endl;
   }
   return sentLogProb;
}
