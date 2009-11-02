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

LMMix::Creator::Creator(
      const string& lm_physical_filename, Uint naming_limit_order)
   : PLM::Creator(lm_physical_filename, naming_limit_order)
{}

bool LMMix::Creator::checkFileExists()
{
   if (!check_if_exists(lm_physical_filename)) return false;

   string line;
   iSafeMagicStream file(lm_physical_filename);

   Uint num_lines = 0;
   while (getline(file, line)) {
      ++num_lines;
      vector<string> toks;
      if (split(line, toks) != 2) 
         error(ETFatal, "syntax error in %s; expected 2 tokens on line %d: %s",
               lm_physical_filename.c_str(), num_lines, line.c_str());

      if (!PLM::checkFileExists(toks[0]))
         cerr << "LMMix(" << lm_physical_filename << ") can't access: "
              << toks[0] << endl;
   }

   if (num_lines == 0) {
      error(ETWarn, "empty lmmix file %s", lm_physical_filename.c_str());
      return false;
   }

   return true;
}

Uint64 LMMix::Creator::totalMemmapSize()
{
   iSafeMagicStream file(lm_physical_filename);
   string line;

   Uint64 total_size = 0;
   while (getline(file, line)) {
      vector<string> toks;
      if (split(line, toks) != 2) 
         error(ETFatal, "syntax error in %s; expected 2 tokens in %s",
               lm_physical_filename.c_str(), line.c_str());

      total_size += PLM::totalMemmapSize(toks[0]);
   }
   return total_size;
}

PLM* LMMix::Creator::Create(VocabFilter* vocab,
                            OOVHandling oov_handling,
                            float oov_unigram_prob,
                            bool limit_vocab,
                            Uint limit_order,
                            ostream *const os_filtered,
                            bool quiet)
{
   return new LMMix(lm_physical_filename, vocab, oov_handling,
                    oov_unigram_prob, limit_vocab, limit_order);

}

LMMix::LMMix(const string& name, VocabFilter* vocab,
             OOVHandling oov_handling, float oov_unigram_prob,
             bool limit_vocab, Uint limit_order)
   : PLM(vocab, oov_handling, oov_unigram_prob)
{
   // read component models and their global weights
   iSafeMagicStream file(name);
   string line;

   double z = 0.0;
   while (getline(file, line)) {
      vector<string> toks;
      if (split(line, toks) != 2) 
         error(ETFatal, "syntax error in %s; expected 2 tokens in %s",
               name.c_str(), line.c_str());

      models.push_back(PLM::Create(toks[0], vocab, oov_handling, oov_unigram_prob,
                                   limit_vocab, limit_order, NULL));
      if (models.back()->getOrder() > gram_order)
         gram_order = models.back()->getOrder();

      const double wt = conv<double>(toks[1]);
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
