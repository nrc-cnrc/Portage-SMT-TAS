/**
 * @author Eric Joanis
 * @file lmrestcost.h  Prototype implementation of better rest costs.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#include "lmrestcost.h"
#include "str_utils.h"
#include "lmtrie.h"
#include "tplm.h"

using namespace Portage;

static const string LMRestCostMagicNumberV1 = "Rest-cost LM v1.0 (NRC)";

static const string help_the_poor_user_s = "\
Syntax for Rest-Cost LM config file:\n\
   " + LMRestCostMagicNumberV1 + "\n\
   rest-costs=RESTCOSTS\n\
   full-lm=FULLLM\n\
<FULLLM> specifies the regular KN-smoothed LM of the full order\n\
<RESTCOSTS> specifies an LM file that contains the rest costs from lower-order\n\
n-grams, to be used when context is not completely known.\n\
\n\
<FULLLM> and <RESTCOSTS> can be any LM format supported by Portage\n\
\n\
To use a rest-cost LM, specify the .rclm folder containing the config file,\n\
or explicitly name the config file.\n\
";
static const char* help_the_poor_user = help_the_poor_user_s.c_str();

LMRestCost::Creator::Creator(
      const string& lm_physical_filename, Uint naming_limit_order)
   : PLM::Creator(lm_physical_filename, naming_limit_order)
{
}

bool LMRestCost::isA(const string& lm_physical_filename) {
   return
      (is_directory(lm_physical_filename) && check_if_exists(lm_physical_filename + "/config")) ||
      matchMagicNumber(lm_physical_filename, LMRestCostMagicNumberV1);
}

bool LMRestCost::Creator::checkFileExists(vector<string>* list)
{
   if (is_directory(lm_physical_filename)) {
      dir = lm_physical_filename;
      configFile = dir + "/config";
   } else {
      configFile = lm_physical_filename;
      dir = DirName(lm_physical_filename);
   }
   if (list) {
      list->push_back(dir);
      list->push_back(configFile);
   }
   if (!is_directory(dir) && check_if_exists(configFile))
      return false;

   string line;
   iSafeMagicStream config(configFile);
   if (!getline(config,line) || line != LMRestCostMagicNumberV1) {
      error(ETWarn, "Rest-cost LM config file %s does not start with magic string '%s'\n%s",
            configFile.c_str(), LMRestCostMagicNumberV1.c_str(), help_the_poor_user);
      return false;
   }

   bool ok = true;
   while (getline(config, line)) {
      trim(line);
      if (line.empty() || line[0] == '#') continue;
      const string::size_type p = line.find('=');
      if (p == string::npos || p+1 == line.size()) {
         error(ETWarn, "Invalid line in rest-cost LM config file %s: %s\n%s",
               configFile.c_str(), line.c_str(), help_the_poor_user);
         ok = false;
      }
      string key = line.substr(0,p);
      trim(key);
      string value = line.substr(p+1);
      trim(value);
      if (key == "rest-costs" || key == "full-lm") {
         string adjustedFileName = adjustRelativePath(dir, value);
         bool file_exists = PLM::checkFileExists(adjustedFileName);
         if (!file_exists) {
            error(ETWarn, "Can't access %s component %s in Rest-cost LM config file %s\n%s",
                  key.c_str(), value.c_str(), configFile.c_str(), help_the_poor_user);
            ok = false;
         }
         // Save the file names for the sake of other methods in this Creator class.
         if (key == "rest-costs")
            restCostsFile = adjustedFileName;
         else
            fullLMFile = adjustedFileName;
      }
   }

   return ok;
}

Uint64 LMRestCost::Creator::totalMemmapSize()
{
   if (checkFileExists(NULL))
      return PLM::totalMemmapSize(restCostsFile) + PLM::totalMemmapSize(fullLMFile);
   else
      return 0;
}

PLM* LMRestCost::Creator::Create(VocabFilter* vocab,
                            OOVHandling oov_handling,
                            float oov_unigram_prob,
                            bool limit_vocab,
                            Uint limit_order,
                            ostream *const os_filtered,
                            bool quiet)
{
   if (!checkFileExists(NULL))
      error(ETFatal, "Problem with Rest-cost LM %s, aborting.", lm_physical_filename.c_str());
   return new LMRestCost(*this, vocab, oov_handling,
                         oov_unigram_prob, limit_vocab, limit_order,
                         os_filtered);
}

LMRestCost::LMRestCost(const Creator& creator, VocabFilter* vocab,
                   OOVHandling oov_handling, float oov_unigram_prob,
                   bool limit_vocab, Uint limit_order,
                   ostream *const os_filtered) :
   PLM(vocab, oov_handling, oov_unigram_prob),
   m(NULL),
   r(NULL),
   myCreator(creator)
{
   // Construct the full-lm component model
   m = PLM::Create(myCreator.fullLMFile, vocab, oov_handling, oov_unigram_prob,
                   limit_vocab, limit_order, os_filtered);
   assert(m);
   // Constrcut the rest-cost component model
   r = PLM::Create(myCreator.restCostsFile, vocab, oov_handling, oov_unigram_prob,
                   limit_vocab, limit_order, os_filtered);
   assert(r);

   if (!dynamic_cast<LMTrie*>(r) && !dynamic_cast<TPLM*>(r))
      error(ETFatal, "The rest-cost component of rest-cost LM %s must be a TPLM, a binlm or an ARPA LM file. %s is not supported.",
            myCreator.lm_physical_filename.c_str(), myCreator.restCostsFile.c_str());

   // We reuse this at every call to wordProb(), so save it in the class itself.
   full_order = getOrder();
}

LMRestCost::~LMRestCost()
{
   delete m;
   delete r;
}

const bool debug_lmrestcost = false;

float LMRestCost::wordProb(Uint word, const Uint context[], Uint context_length)
{
   if (debug_lmrestcost) cerr << "C_len=" << context_length;
   if (context_length + 1 >= full_order) {
      if (debug_lmrestcost) cerr << " is complete." << endl;
      return m->wordProb(word, context, context_length);
   } else {
      float restCost = r->wordProb(word, context, context_length);
      if (debug_lmrestcost) cerr << " rc-depth=" << r->getLatestNgramDepth();
      if (r->getLatestNgramDepth() == context_length+1) {
         if (debug_lmrestcost) cerr << " using rest-cost." << endl;
         hits.hit(context_length+1);
         return restCost;
      } else {
         if (debug_lmrestcost) cerr << " using back-off model." << endl;
         return m->wordProb(word, context, context_length);
      }
   }
}

Uint LMRestCost::minContextSize(const Uint context[], Uint context_length)
{
   return m->minContextSize(context, context_length);
}

void LMRestCost::newSrcSent(const vector<string>& src_sent,
                          Uint external_src_sent_id)
{
   m->newSrcSent(src_sent, external_src_sent_id);
   r->newSrcSent(src_sent, external_src_sent_id);
}

