/**
 * @author GF
 * @file lmmix.cc - lm mixture model
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "lmmix.h"
#include "file_utils.h"
#include "str_utils.h"
#include "gfmath.h"

using namespace Portage;
using namespace std;

namespace Portage {

static const char LMMIX_COOKIE_V1_0[] = "sent-level mixture v1.0";

const char* LMMix::sentLevelMixtureCookieV1()
{
   return LMMIX_COOKIE_V1_0;
}


LMMix::Creator::Creator(
      const string& lm_physical_filename, Uint naming_limit_order)
   : PLM::Creator(lm_physical_filename, naming_limit_order)
   , lmmix_relative(true)
   , has_been_parsed(false)
{}

bool LMMix::Creator::checkFileExists(vector<string>* list)
{
   // If we don't have to populated list and we already have parsed this model
   // file then do nothing.
   if (!list && has_been_parsed) return true;

   canonical_sub_models.clear();

   if (!check_if_exists(lm_physical_filename)) return false;

   if (list) list->push_back(lm_physical_filename);

   string lmmix_dir = lmmix_relative ? DirName(lm_physical_filename) : "";

   string line;
   iSafeMagicStream file(lm_physical_filename);

   bool ok = true;
   bool have_seen_relative_path = false;
   Uint num_lines = 0;
   while (getline(file, line)) {
      if (isPrefix(LMMIX_COOKIE_V1_0, line))
         break;
      ++num_lines;
      vector<string> toks;
      if (split(line, toks) != 2)
         error(ETFatal, "syntax error in %s; expected 2 tokens on line %d: %s",
               lm_physical_filename.c_str(), num_lines, line.c_str());

      bool file_exists = PLM::checkFileExists(adjustRelativePath(lmmix_dir, toks[0]));
      if (!file_exists && !have_seen_relative_path && toks[0][0] != '/') {
         // try the alternative relative path
         lmmix_relative = not lmmix_relative;
         lmmix_dir = lmmix_relative ? DirName(lm_physical_filename) : "";
         file_exists = PLM::checkFileExists(adjustRelativePath(lmmix_dir, toks[0]));
         if (file_exists) {
            cerr << "Falling back to path"
                 << (lmmix_relative ? "" : " (relative to current working directory)")
                 << ": " << adjustRelativePath(lmmix_dir, toks[0]) << endl;
            error(ETWarn, "Using component LM paths relative to the %s.",
                  lmmix_relative ? "mixlm file location" : "current working directory");
         }
         else {
            lmmix_relative = not lmmix_relative; // restore default setting
            lmmix_dir = lmmix_relative ? DirName(lm_physical_filename) : "";
         }
      }
      if (toks[0][0] != '/')
         have_seen_relative_path = true;
      if (!file_exists) {
         cerr << "LMMix(" << lm_physical_filename << ") can't access: "
              << toks[0] << endl;
         ok = false;
      }

      canonical_sub_models.push_back(adjustRelativePath(lmmix_dir, toks[0]));

      if (list) PLM::checkFileExists(canonical_sub_models.back(), list);
   }

   if (num_lines == 0) {
      error(ETWarn, "empty lmmix file %s", lm_physical_filename.c_str());
      return false;
   }

   has_been_parsed = ok;

   return ok;
}

Uint64 LMMix::Creator::totalMemmapSize()
{
   if (!checkFileExists(NULL)) return 0;

   Uint64 total_size = 0;
   typedef vector<string>::const_iterator IT;
   for (IT it(canonical_sub_models.begin()); it!=canonical_sub_models.end(); ++it) {
      total_size += PLM::totalMemmapSize(*it);
   }

   return total_size;
}

bool LMMix::Creator::prime(bool full)
{
   if (!checkFileExists(NULL)) return false;

   bool ok = true;
   typedef vector<string>::const_iterator IT;
   for (IT it(canonical_sub_models.begin()); it!=canonical_sub_models.end(); ++it) {
      ok &= PLM::prime(*it);
   }

   return ok;
}

PLM* LMMix::Creator::Create(VocabFilter* vocab,
                            OOVHandling oov_handling,
                            float oov_unigram_prob,
                            bool limit_vocab,
                            Uint limit_order,
                            ostream *const os_filtered,
                            bool quiet)
{
   if (!checkFileExists(NULL))
      error(ETFatal, "Unable to open MIXLM %s or one of its associated files.",
            lm_physical_filename.c_str());
   return new LMMix(lm_physical_filename, vocab, oov_handling, oov_unigram_prob,
                    limit_vocab, limit_order, lmmix_relative, false, NULL);

}




LMMix::LMMix(const string& name, VocabFilter* vocab,
             OOVHandling oov_handling, float oov_unigram_prob,
             bool limit_vocab, Uint limit_order, bool lmmix_relative,
             bool notreally, vector<string>* model_names) :
   PLM(vocab, oov_handling, oov_unigram_prob),
   sent_level_mixture(false)
{
   // read initial block: component models and their global weights
   iSafeMagicStream file(name);
   string line;

   string lmmix_dir = lmmix_relative ? DirName(name) : "";

   while (getline(file, line)) {
      if (isPrefix(LMMIX_COOKIE_V1_0, line)) {
         sent_level_mixture = true;
         break;
      }
      vector<string> toks;
      if (split(line, toks) != 2)
         error(ETFatal, "syntax error in %s; expected 2 tokens in %s",
               name.c_str(), line.c_str());

      if (model_names)
         (*model_names).push_back(toks[0]);

      if (notreally) {
         models.push_back(NULL);
      } else {
         models.push_back(PLM::Create(adjustRelativePath(lmmix_dir, toks[0]),
                                      vocab, oov_handling, oov_unigram_prob,
                                      limit_vocab, limit_order, NULL));
         if (models.back()->getOrder() > gram_order)
            gram_order = models.back()->getOrder();
      }

      gwts.push_back(conv<double>(toks[1]));
   }

   double z = accumulate(gwts.begin(), gwts.end(), 0.0);
   if (z > 1.0001 || z < 0.9999)
      error(ETWarn, "lmmix model weights not normalized in %s - sum is %g", name.c_str(), z);

   if (models.empty())
      error(ETFatal, "empty lmmix model %s", name.c_str());

   // process sent-level weight vectors, if magic cookie found

   if (sent_level_mixture) {

      double g = 0.0;
      if (line.length() > strlen(LMMIX_COOKIE_V1_0))
         g = conv<double>(line.substr(strlen(LMMIX_COOKIE_V1_0)));

      while (getline(file, line)) {
         per_sent_wts.push_back(vector<double>(0));
         if (split(line, per_sent_wts.back()) != gwts.size())
            error(ETFatal, "size of weight vector doesn't match global vector: %s",
                  line.c_str());
         for (Uint i = 0; i < gwts.size(); ++i)
            per_sent_wts.back()[i] = log(g * gwts[i] + (1.0-g) * per_sent_wts.back()[i]);
      }
   }

   // take log of global wts

   for (Uint i = 0; i < gwts.size(); ++i)
      gwts[i] = log(gwts[i]);

   wts = &gwts[0];
}

float LMMix::wordProb(Uint word, const Uint context[], Uint context_length)
{
   static double log0 = log(0.0);
   double p = 0.0;
   for (Uint i = 0; i < models.size(); ++i)
      if (wts[i] != log0)
         p += exp(wts[i] + models[i]->wordProb(word, context, context_length));
   return log(p);
}

Uint LMMix::minContextSize(const Uint context[], Uint context_length)
{
   static double log0 = log(0.0);
   Uint result = 0;
   for (Uint i = 0; i < models.size(); ++i)
      if (wts[i] != log0)
         result = max(result, models[i]->minContextSize(context, context_length));
   return result;
}

void LMMix::newSrcSent(const vector<string>& src_sent,
                       Uint external_src_sent_id)
{
   if (sent_level_mixture) {
      clearCache(); // In this case, changing sentence invalidates the cache.
      if (external_src_sent_id >= per_sent_wts.size())
         error(ETFatal,
               "mixlm src index too large: you may be using the model for the wrong source file!");
      wts = &per_sent_wts[external_src_sent_id][0];
   }

   for (Uint i = 0; i < models.size(); ++i)
      models[i]->newSrcSent(src_sent, external_src_sent_id);
}

LMMix::~LMMix()
{
   for (Uint i = 0; i < models.size(); ++i)
      delete(models[i]);
}

PLM::Hits LMMix::getHits()
{
   PLM::Hits hits;
   typedef vector<PLM*>::iterator LM_IT;
   for (LM_IT it(models.begin()); it!= models.end(); ++it) {
      hits += (*it)->getHits();
   }
   return hits;
}

};
