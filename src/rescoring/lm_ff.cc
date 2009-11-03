/**
 * @author Aaron Tikuisis
 * @file lm_ff.cc  Implementation of feature function NgramFF.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "lm_ff.h"
#include "canoe_general.h"
#include "lm.h"
#include "file_utils.h"

using namespace Portage;

NgramFF::NgramFF(const string& args)
: FeatureFunction(args)
, lm(NULL)
{}

NgramFF::~NgramFF()
{
   if (lm) delete lm;
}

bool NgramFF::parseAndCheckArgs()
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

void NgramFF::init(const Sentences * const src_sents)
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

double NgramFF::value(Uint k)
{
   // Store the sentence backwards in an array, along with begin-sentence and
   // end-sentence tags.
   const Tokens& tokens = (*nbest)[k].getTokens();

   const Uint uSent_size = tokens.size() + 2;
   Uint uSent[uSent_size];
   for (Uint i = 0; i < tokens.size(); i++)
      uSent[tokens.size() - i] = tgt_vocab->add(tokens[i].c_str());
   uSent[0] = tgt_vocab->index(PLM::SentEnd);
   uSent[tokens.size() + 1] = tgt_vocab->index(PLM::SentStart);

   double result = 0;
   for (Uint i = 0; i < tokens.size() + 1; i++)
      result += lm->wordProb(uSent[i], uSent + i + 1, uSent_size - i - 1);
   assert(result > -INFINITY);
   return result;
} // value

void NgramFF::values(Uint k, vector<double>& vals)
{
   // Store the sentence backwards in an array, along with begin-sentence and
   // end-sentence tags.
   const Tokens& tokens = (*nbest)[k].getTokens();

   Uint uSent_size = tokens.size() + 2;
   Uint uSent[uSent_size];
   for (Uint i = 0; i < tokens.size(); i++)
      uSent[tokens.size() - i] = tgt_vocab->add(tokens[i].c_str());
   uSent[0] = tgt_vocab->index(PLM::SentEnd);
   uSent[tokens.size() + 1] = tgt_vocab->index(PLM::SentStart);

   for (Uint i = uSent_size-2; i>0; i--)
     vals.push_back(lm->wordProb(uSent[i], uSent + i + 1, uSent_size - i - 1));

   assert(vals.size() == tokens.size());
} // values
