/**
 * @author Aaron Tikuisis / George Foster / Samuel Larkin / Eric Joanis
 * @file featurefunction_set.cc  K-Best Rescoring Module - Sets of Feature
 *                               functions
 *
 * $Id$
 *
 * This class was moved out of featurefunction.h to remove spurious
 * dependencies that slowed down compiling.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005 - 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005 - 2008, Her Majesty in Right of Canada
*/

#include "featurefunction_set.h"
#include "file_ff.h"
#include "feature_function_grammar.h"
#include <errors.h>
#include <str_utils.h>
#include <file_utils.h>
#include <vocab_filter.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#ifndef NO_COMPUTED_FF
// Include files for feature functions
#include "lm_ff.h"
#include "ibm_ff.h"
#include "ibm1del.h"
#include "cache_lm_ff.h"
#include "ibm1wtrans.h"
#include "nbest_posterior_ff.h"
#include "consensus.h"
#include "bleurisk.h"
#include "bleu_postedit_ff.h"
#include "wer_postedit_ff.h"
#include "per_postedit_ff.h"
#include "mismatch_ff.h"
#include "backward_lm_ff.h"
#endif

using namespace Portage;

//---------------------------------------------------------------------------
// FeatureFunctionSet
//---------------------------------------------------------------------------

Logging::logger Logger_FeatureFunctionSet(Logging::getLogger("debug.rescoring.FeatureFunctionSet"));

FeatureFunctionSet::FeatureFunctionSet(bool training, Uint seed) :
   training(training),
   seed(seed),
   tgt_vocab(NULL)
{
}

FeatureFunctionSet::~FeatureFunctionSet()
{
   if (tgt_vocab) delete tgt_vocab;
   ff_infos.clear();
}

//   NbestReader  pfr(FileReader::create<Translation>(arg.nbest_file, S, arg.K));
void FeatureFunctionSet::createTgtVocab(const Sentences& source_sentences, NbestReader nbReader)
{
   LOG_VERBOSE1(Logger_FeatureFunctionSet, "Initializing FeatureFunctionSet");
   // Make sure there is at least a feature
   // Init should be called after read
   assert(!source_sentences.empty());
   assert(!ff_infos.empty());
   assert(nbReader.get()!=NULL);

   Uint required = FF_NEEDS_NOTHING;
   for (Uint m(0); m < M(); ++m) {
      required |= ff_infos[m].function->requires();    // Check what feature functions need
   }

   if (required & FF_NEEDS_TGT_VOCAB) {
      LOG_VERBOSE1(Logger_FeatureFunctionSet, "Creating target vocabulary");
      if (tgt_vocab == NULL) {
         tgt_vocab = new VocabFilter(source_sentences.size());
      }

      for (Uint s(0); s<source_sentences.size(); ++s)
         tgt_vocab->addSentence(source_sentences[s].getTokens(), s);

      Uint s(0);
      for (; nbReader->pollable(); ++s) {
         Nbest nbest;
         nbReader->poll(nbest);
         // Here we limit the number of per sentence voacb since they can be quite big
         const Uint sent_no(s % VocabFilter::maxSourceSentence4filtering);
         typedef Nbest::const_iterator SIT;
         for (SIT sent(nbest.begin()); sent!=nbest.end(); ++sent) {
            tgt_vocab->addSentence(sent->getTokens(), sent_no);
         }
         for (Uint m(0); m < M(); ++m)
            ff_infos[m].function->preprocess(tgt_vocab, s, nbest);
      }
      assert(s==source_sentences.size());
   }
}

void FeatureFunctionSet::initFFMatrix(const Sentences& src_sents)
{
   LOG_VERBOSE1(Logger_FeatureFunctionSet, "Initializing Feature Function Matrix");
   for (Uint m(0); m < M(); ++m) {
      if (ff_infos[m].function->requires() & FF_NEEDS_TGT_VOCAB) {
         assert(tgt_vocab != NULL);
         ff_infos[m].function->addTgtVocab(tgt_vocab);
      }
      ff_infos[m].function->init(&src_sents);
   }
}

void FeatureFunctionSet::computeFFMatrix(uMatrix& H, Uint s, Nbest &nbest)
{
   const Uint K(nbest.size());

   Uint nEmpty(0);
   Uint nContiguous(0);
   for (Uint i(0); i<nbest.size(); ++i) {
      if (nbest[i].empty()) {
	 ++nEmpty;
	 ++nContiguous;
      }
      else
	 nContiguous = 0;
   }
   const Uint empty = nEmpty;

   if (empty == K) {
       // error(ETFatal, "The Nbest list for %d only contains empty lines", s);
       cerr << "The Nbest list for " << s << " only contains empty lines" << endl;
       H.resize(0, M(), false);
       nbest.resize(0);
       return;
   }

   if (nContiguous != nEmpty)
      error(ETFatal, "The Nbest list for %d contains some non-contiguous empty lines", s);

   H.resize(K-empty, M(), false);

   Uint required = FF_NEEDS_NOTHING;
   for (Uint m(0); m < M(); ++m) {
      required |= ff_infos[m].function->requires();    // Check what feature functions need
      ff_infos[m].function->source(s, &nbest);
   }

   Uint l(0);
   for (Uint k = 0; k < K; ++k) {
      if (!nbest[k].empty()) {
         if (required & FF_NEEDS_TGT_TOKENS ) // Target tokenization
            nbest[k].getTokens();
         if ((required & FF_NEEDS_ALIGNMENT) && !nbest[k].alignment) // Alignment
            error(ETFatal, "Alignment needed and not found in nbest[%d][%d]\n", s, k);

         for (Uint m = 0; m < M(); ++m)
            H(l, m) = ff_infos[m].function->value(k);
         ++l;
      }
   }
   // Unfortunately if the last set of nbest contains empty lines we must read
   // them because there is a check for consistency that will later fail if not
   // read.
   for (Uint m = 0; m < M(); ++m)
      ff_infos[m].function->value(K-1);

   Nbest::iterator last = remove_if(nbest.begin(), nbest.end(), mem_fun_ref(&Translation::empty));
   assert(Uint(nbest.end()-last) == empty);
   assert(Uint(last-nbest.begin()) == K-empty);
   nbest.resize(K-empty);
   assert(l==nbest.size());
}

// Read description of a feature set from a file.
Uint FeatureFunctionSet::read(const string& filename, bool verbose,
                              const char* fileff_prefix,
                              bool isDynamic, bool useNullDeleter, bool loadModels)
{
   using namespace boost::spirit;

   ff_infos.clear();

   iSafeMagicStream istr(filename.c_str());

   string line;
   vector<string> toks;

   // read contents of file
   Uint index(0);
   feature_function_grammar  gram(training);
   while (getline(istr, line)) {
      if (line.empty()) continue;
      trim(line);
      // Commented out feature function
      if (line[0] == '#') {
         ff_infos.push_back(line);
         error(ETWarn, "Not loading %s", line.c_str());
         continue;
      }

      if (!parse(line.c_str(), gram, space_p).full) {
         error(ETFatal, "Unable to parse the following feature: %s", line.c_str());
      }

      if (verbose)
         cerr << "initializing feature " << gram.name << " with arg: [" << gram.arg << "]" << endl;

      ptr_FF  function(create(gram.name, gram.arg, fileff_prefix, isDynamic, useNullDeleter, loadModels));
      if (!(function))
         error(ETFatal, "unknown feature: %s", gram.name.c_str());

      gram.rnd->seed(++index*17600951 + seed); // Make sure they are not seeded identically
      ff_infos.push_back(ff_info(gram.ff, gram.name, function, gram.weight, gram.rnd));
      gram.clear();  // Get ready for the next one
   }

   return ff_infos.size();
}

void FeatureFunctionSet::getRandomWeights(uVector& v) const
{
   assert(v.size() == M());
   assert(training);
   for (Uint m(0); m<M(); ++m) {
      assert(ff_infos[m].rnd_gen.get() != NULL);
      v(m) = ff_infos[m].rnd_gen->get();
   }
}

void FeatureFunctionSet::getWeights(uVector& v) const
{
   if (v.size() != M())
      v.resize(M());

   for (Uint m(0); m<M(); ++m)
      v(m) = ff_infos[m].weight;
}

void FeatureFunctionSet::setWeights(const uVector& v)
{
   assert(v.size() == M());
   for (Uint m(0); m<M(); ++m)
      ff_infos[m].weight = v(m);
}

void FeatureFunctionSet::write(const string& filename)
{
   ofstream ostr(filename.c_str());
   if (!ostr)
      error(ETFatal, "unable to open file %s", filename.c_str());
   ostr << setprecision(10);

   ff_infos.write(ostr);
}

bool FeatureFunctionSet::complete()
{
   bool bRetour(true);

   typedef vector<ff_info>::iterator IT;
   for (IT it(ff_infos.begin()); it!=ff_infos.end(); ++it) {
      bRetour &= it->function->done();
   }

   return bRetour;
}

Uint FeatureFunctionSet::requires()
{
   Uint bRetour(0);

   typedef vector<ff_info>::iterator IT;
   for (IT it(ff_infos.begin()); it!=ff_infos.end(); ++it) {
      bRetour |= it->function->requires();
   }

   return bRetour;
}

bool FeatureFunctionSet::check()
{
   bool bRetour(true);

   typedef vector<ff_info>::iterator IT;
   for (IT it(ff_infos.begin()); it!=ff_infos.end(); ++it) {
      const bool v = it->function->parseAndCheckArgs();
      bRetour = bRetour && v;
      if (!v) {
         cerr << "Bad ff syntax or invalid filename for " << it->name;
         cerr << " with: " << it->fullDescription << endl;
      }
   }

   return bRetour;
}

template<typename T>
struct null_deleter
{
   void operator()(T const *) const {}
};


ptr_FF FeatureFunctionSet::create(const string& name,
                                  const string& arg,
                                  const char* fileff_prefix,
                                  bool isDynamic,
                                  bool useNullDeleter,
                                  bool loadModels)
{
   FeatureFunction* ff;

   if (name == "FileFF") {
      const string fileff_arg = fileff_prefix ? fileff_prefix + arg : arg;
      // TODO what happens when we have /path/ff.file.arg, where does the prefix fit?
      // now it will yield fileff_prefix/path/ff.file.arg

      if (isDynamic)
         ff = new FileDFF(fileff_arg);
      else
         ff = new FileFF(fileff_arg);

#ifndef NO_COMPUTED_FF
   } else if (name == "LengthFF") {
      ff = new LengthFF();
   } else if (name == "RatioFF") {
      ff = new RatioFF();
   } else if (name == "NgramFF") {
      ff = new NgramFF(arg);
   } else if (name == "IBM1TgtGivenSrc") {
      ff = new IBM1TgtGivenSrc(arg);
   } else if (name == "IBM1SrcGivenTgt") {
      ff = new IBM1SrcGivenTgt(arg);
   } else if (name == "IBM2TgtGivenSrc") {
      ff = new IBM2TgtGivenSrc(arg);
   } else if (name == "IBM2SrcGivenTgt") {
      ff = new IBM2SrcGivenTgt(arg);
   } else if (name == "IBM1AaronTgtGivenSrc" || name == "IBM1WTransTgtGivenSrc") {
      ff = new IBM1WTransTgtGivenSrc(arg);
   } else if (name == "IBM1AaronSrcGivenTgt" || name == "IBM1WTransSrcGivenTgt") {
      ff = new IBM1WTransSrcGivenTgt(arg);
   } else if (name == "IBM1DeletionTgtGivenSrc") {
      ff = new IBM1DeletionTgtGivenSrc(arg);
   } else if (name == "IBM1DeletionSrcGivenTgt") {
      ff = new IBM1DeletionSrcGivenTgt(arg);
   } else if (name == "IBM1DocTgtGivenSrc") {
      ff = new IBM1DocTgtGivenSrc(arg);
   } else if (name == "nbestWordPostLev") {
      ff = new nbestWordPostLev_ff(arg);
   } else if (name == "nbestWordPostSrc") {
      ff = new nbestWordPostSrc_ff(arg);
   } else if (name == "nbestWordPostTrg") {
      ff = new nbestWordPostTrg_ff(arg);
   } else if (name == "nbestPhrasePostSrc") {
      ff = new nbestPhrasePostSrc_ff(arg);
   } else if (name == "nbestPhrasePostTrg") {
      ff = new nbestPhrasePostTrg_ff(arg);
   } else if (name == "nbestNgramPost") {
      ff = new nbestNgramPost_ff(arg);
   } else if (name == "nbestSentLenPost") {
      ff = new nbestSentLenPost_ff(arg);
   } else if (name == "Consensus") {
      ff = new ConsensusWer(arg);
   } else if (name == "ConsensusWin") {
      ff = new ConsensusWin(arg);
   } else if (name == "BLEUrisk") {
      ff = new RiskBleu(arg);
   } else if (name == "ParMismatch") {
      ff = new ParMismatchFF(arg);
   } else if (name == "QuotMismatch") {
      ff = new QuotMismatchFF(arg);
   } else if (name == "CacheLM") {
      ff = new CacheLM(arg);
   } else if (name == "BleuPostedit") {
      ff = new BleuPostedit(arg);
   } else if (name == "WerPostedit") {
      ff = new WerPostedit(arg);
   } else if (name == "PerPostedit") {
      ff = new PerPostedit(arg);
   } else if (name == "BackwardLM") {
      ff = new BackwardLmFF(arg);
#endif
   } else {
      error(ETFatal, "Invalid feature function: %s:%s",
            name.c_str(), arg.c_str());
      return ptr_FF(static_cast<FeatureFunction*>(NULL),
                    null_deleter<FeatureFunction>());
   }
   assert(ff != NULL);

   if (loadModels)
      if (!ff->loadModels())
         error(ETFatal, "Failed to load models for %s%s%s.",
               name.c_str(), (arg.empty() ? "" : ":"), arg.c_str());

   if ( useNullDeleter ) {
      return ptr_FF(ff, null_deleter<FeatureFunction>());
   } else {
      return ptr_FF(ff);
   }
}

const string& FeatureFunctionSet::help()
{
   static string help_str = "\
Features available:\n\
\n\
 LengthFF - number of characters\n\
 RatioFF - ratio of words in the translation over words in the source\n\
 NgramFF:lm-file[#order] - log prob according to lm-file\n\
 IBM1TgtGivenSrc:ibm1.tgt_given_src - IBM1 forward probability\n\
 IBM1SrcGivenTgt:ibm1.src_given_tgt - IBM1 backward probability\n\
 IBM2TgtGivenSrc:ibm2.tgt_given_src - IBM2 forward probability\n\
 IBM2SrcGivenTgt:ibm2.src_given_tgt - IBM2 backward probability\n\
 IBM1WTransTgtGivenSrc:ibm1.tgt_given_src - IBM1 check if all words translated\n\
 IBM1WTransSrcGivenTgt:ibm1.src_given_tgt - IBM1 check if no words inserted\n\
 IBM1DeletionTgtGivenSrc:ibm1.tgt_given_src[#thr] - ratio of del words (p<thr) [0.1]\n\
 IBM1DeletionSrcGivenTgt:ibm1.src_given_tgt[#thr] - ratio of del words (p<thr) [0.1]\n\
 IBM1DocTgtGivenSrc:ibm1.tgt_given_src#docids - p(tgt-hyp|src-doc)\n\
 nbestWordPostLev:scale#<ffval-wts>[#<pfx>] - Levenshtein-based word posteriors\n\
 nbestWordPostSrc:scale#<ffval-wts>[#<pfx>] - source-pos-based word posteriors\n\
 nbestWordPostTrg:scale#<ffval-wts>[#<pfx>] - target-pos-based word posteriors\n\
 nbestPhrasePostSrc:scale#<ffval-wts>[#<pfx>] - source-pos-based phrase post.\n\
 nbestPhrasePostTrg:scale#<ffval-wts>[#<pfx>] - target-pos-based phrase post.\n\
 nbestNgramPost:N#scale#<ffval-wts>[#<pfx>] - N-gram posteriors\n\
 nbestSentLenPost:scale#<ffval-wts>[#<pfx>] - sentence length posteriors\n\
 Consensus - WER-based consensus over N-best list *VERY EXPENSIVE FEATURE*\n\
 ConsensusWin - approx. WER in Consensus by window over position *VERY EXPENSIVE*\n\
 BLEUrisk:len#smoothBLEU#scale#<ffval-wts>[#<pfx>] - risk using BLEU loss function\n\
 ParMismatch - number of mismatched parentheses within the hypothesis\n\
 QuotMismatch:st - mismatched quotes, for src/tgt lang <s>/<t> (eg 'ce')\n\
 CacheLM:docids - cache LM over docs defined in docids file\n\
 WerPostedit - WER score over source sentence for statstitical post-editing\n\
 PerPostedit - PER score over source sentence for statstitical post-editing\n\
 BleuPostedit:smooth - BLEU score over source sentence for stat.post-editing\n\
 BackwardLM:lm-file[#order] - log prob according to a backward language model\n\
 FileFF:file[,column] - pre-computed feature\n\
 SCRIPT:\"./yourscript <some arguments>\" - user specified script\n\
\n\
More details:\n\
\n\
* The features called 'nbest...Post...' are posterior probabilities for words,\n\
  phrases, etc. Their <ffval-wts> argument is a file specifying the features\n\
  determined by the decoder (ffvals) and their weights. Given cow.sh output\n\
  canoe.ini.cow, this can be produced as follows:\n\
     > configtool rescore-model:ffvals canoe.ini.cow\n\
     FileFF:ffvals,1 0.1\n\
     FileFF:ffvals,2 -0.15\n\
     FileFF:ffvals,3 0.05\n\
     FileFF:ffvals,4 0.2\n\
  Here the actual decoder feature values must be in the file <pfx>ffvals, where\n\
  <pfx> is the optional last argument to the nbest...Post... feature.\n\
\n\
* The BLEUrisk feature calculates the risk of the current hypothesis using BLEUK\n\
  as loss function (see Kumar & Byrne, HLT/NAACL 2004).\n\
  It has up to 5 arguments:\n\
  - no. of hyps. to consider in risk calculation (recommendation: 100 -> speed up)\n\
  - BLEU smoothing technique, (recommendation:1 or 2, see bleumain -h for help)\n\
  - scaling factor for sentence probabilities\n\
  - <ffval-wts> and <pfx>, see posterior probabilities above\n\
\n\
* The 'FileFF' feature reads from a file of pre-computed values, optionally\n\
  picking out a particular column; the program gen_feature_values can be used\n\
  to create files like this.\n\
\n\
* The SCRIPT ff allows the user to try a quick and dirty feature function.\n\
  Your script MUST output its values to standard out.\n\
  The script command line must be in quotes and you can use the five\n\
  following tag:\n\
  <ffval-wts> <src> <nbest> <pfx> <NP>\n\
\n\
* To create a template model for rescoring output produced using canoe config\n\
  file canoe.ini.cow, do:\n\
     configtool rescore-model:ffvals canoe.ini.cow > rescore-model\n\
     rescore_train -H 2>&1 | egrep '^ [A-Za-z]' | cut -c2- >> rescore-model\n\
  then edit rescore-model to select desired features and initial weights.\n\
";
   return help_str;
}

