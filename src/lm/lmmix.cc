/**
 * @author GF
 * @file lmmix.cc - lm mixture model
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "lmmix.h"
#include <file_utils.h>
#include <str_utils.h>
#include <gfmath.h>

using namespace Portage;
using namespace std;

namespace Portage {

LMMix::LMMix(const string& name, VocabFilter* vocab,
             OOVHandling oov_handling, float oov_unigram_prob,
             bool limit_vocab, Uint limit_order) :
   PLM(vocab, oov_handling, oov_unigram_prob)
{
   string s;
   vector<string> toks;
   split(gulpFile(name.c_str(), s), toks);

   if (toks.empty())
      error(ETFatal, "empty lmmix file %s", name.c_str());
      
   if (toks.size() % 2)
      error(ETFatal, "number of lmmix models differs from number of weights in file %s", 
            name.c_str());

   double z = 0.0;
   for (Uint i = 0; i < toks.size()-1; i += 2) {
      models.push_back(PLM::Create(toks[i], vocab, oov_handling, oov_unigram_prob,
                                   limit_vocab, limit_order, NULL));
      if (models.back()->getOrder() > gram_order)
         gram_order = models.back()->getOrder();
      
      double wt = conv<double>(toks[i+1]);
      z += wt;
      wts.push_back(log(wt));
   }
   if (z > 1.0001 || z < 0.9999)
      error(ETWarn, "lmmix model weights not normalized in %s - sum is %g", name.c_str(), z);
   if (models.empty())
      error(ETFatal, "empty lmmix model %s", name.c_str());
}

float LMMix::wordProb(Uint word, const Uint context[], Uint context_length)
{
   double p = 0.0;
   for (Uint i = 0; i < models.size(); ++i)
      p += exp(wts[i] + models[i]->wordProb(word, context, context_length));
   return log(p);
}

void LMMix::clearCache()
{
   for (Uint i = 0; i < models.size(); ++i)
      models[i]->clearCache();
}

LMMix::~LMMix()
{
   for (Uint i = 0; i < models.size(); ++i)
      delete(models[i]);
}

bool LMMix::check_file_exists(const string& lm_filename)
{
   if (!check_if_exists(lm_filename)) return false;

   string s;
   vector<string> toks;
   split(gulpFile(lm_filename.c_str(), s), toks);

   if (toks.empty())
      error(ETFatal, "empty lmmix file %s", lm_filename.c_str());
      
   if (toks.size() % 2)
      error(ETFatal, "number of lmmix models differs from number of weights in file %s", 
            lm_filename.c_str());

   for (Uint i(0); i < toks.size()-1; i += 2) {
      if (!PLM::check_file_exists(toks[i]))
         cerr << "LMMix(" << lm_filename << ") can't access: " << toks[i] << endl;
   }

   return true;
}

};
