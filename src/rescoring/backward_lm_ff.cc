/**
 * @author Samuel Larkin
 * @file backward_lm_ff.cc  Implementation of feature function BackwardLmFF.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "backward_lm_ff.h"
#include "lm.h"
#include "canoe_general.h"
#include "file_utils.h"

using namespace Portage;

BackwardLmFF::BackwardLmFF(const string& args)
: FeatureFunction(args)
, lm(NULL)
, converter(NULL)
{}

BackwardLmFF::~BackwardLmFF()
{
   if (lm) delete lm;
   if (converter) delete converter;
}

bool BackwardLmFF::parseAndCheckArgs()
{
   // The optional order argument will be taken care of when loading the model
   // by PLM.
   if (argument.empty()) {
      error(ETWarn, "You must provide a lm-file name to NgramFF");
      return false;
   }
   if (!check_if_exists(argument.substr(0, argument.find("#")))){
      error(ETWarn, "File is not accessible: %s", argument.c_str());
      return false;
   }
   return true;
}

void BackwardLmFF::init(const Sentences * const src_sents)
{
   assert(src_sents != NULL);
   assert(!src_sents->empty());

   FeatureFunction::init(src_sents);

   // We load the language model only on the first time init is called
   if (lm == NULL) {
      // nbest_posterior will fail here if it uses ngramff.
      assert(tgt_vocab != NULL);
      // we filter only if the global and the per sentence vocab was done prior.
      const bool limitVocab(tgt_vocab != NULL);
      // Here, we give a lm limit of 0 meaning no limit and if the user wants
      // to apply a limit the args would look like NgramFF:lm-file#order and in
      // this case PLM::Create will take care of it. :D
      lm = PLM::Create(argument, tgt_vocab, PLM::SimpleAutoVoc, LOG_ALMOST_0,
                       limitVocab, 0, NULL);
   }
}

void BackwardLmFF::addTgtVocab(VocabFilter* tgt_vocab)
{
   FeatureFunction::addTgtVocab(tgt_vocab);

   assert(converter == NULL);

   converter = new Voc::indexConverter(*tgt_vocab);
}

double BackwardLmFF::value(Uint k)
{
   assert(converter != NULL);
   vector<Uint> uSent;
   uSent.reserve(200);

   // Converter the string translation to the required Uint translation
   uSent.push_back((*converter)(PLM::SentEnd));
   split((*nbest)[k].c_str(), uSent, *converter);
   uSent.push_back((*converter)(PLM::SentStart));

   // Score the translation
   const Uint uSent_size(uSent.size());
   double result = 0;
   for (Uint i = 0; i < uSent_size - 1; i++)
      result += lm->wordProb(uSent[i], &uSent[i + 1], uSent_size - i - 1);

   assert(result > -INFINITY);
   return result;
} // value

void BackwardLmFF::values(Uint k, vector<double>& vals)
{
   assert(converter != NULL);
   vector<Uint> uSent;
   uSent.reserve(200);

   // Converter the string translation to the required Uint translation
   uSent.push_back((*converter)(PLM::SentEnd));
   split((*nbest)[k].c_str(), uSent, *converter);
   uSent.push_back((*converter)(PLM::SentStart));

   // Score the translation
   const Uint uSent_size(uSent.size());
   for (Uint i = uSent_size-2; i>0; i--)
     vals.push_back(lm->wordProb(uSent[i], &uSent[i + 1], uSent_size - i - 1));

   assert(vals.size() == uSent_size-2);
} // values
