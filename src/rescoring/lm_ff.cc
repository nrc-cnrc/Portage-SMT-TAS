/**
 * @author Aaron Tikuisis
 * @file lm_ff.cc  Implementation of feature function ngramFF.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "rescoring_general.h"
#include "lm_ff.h"
#include <canoe_general.h>
#include <lm.h>
#include <voc.h>
#include <file_utils.h>

using namespace Portage;

NgramFF::NgramFF(const string &args)
{
   // The #N, formerly parsed here, is now handled by PLM::Create.
   vocab = new Voc();
   // Ack, this is bad: we're not using limit_vocab!!!  Must do something
   // about this...
   lm = PLM::Create(args, vocab, false, false, 0, LOG_ALMOST_0);
}

NgramFF::~NgramFF()
{
   delete vocab;
   delete lm;
}

double NgramFF::value(Uint k)
{
   // Store the sentence backwards in an array, along with begin-sentence and
   // end-sentence tags.
   const Tokens& tokens = (*nbest)[k].getTokens();

   Uint uSent_size = tokens.size() + 2;
   Uint uSent[uSent_size];
   for (Uint i = 0; i < tokens.size(); i++)
      uSent[tokens.size() - i] = vocab->add(tokens[i].c_str());
   uSent[0] = vocab->index(PLM::SentEnd);
   uSent[tokens.size() + 1] = vocab->index(PLM::SentStart);

   double result = 0;
   for (Uint i = 0; i < tokens.size() + 1; i++)
      result += lm->wordProb(uSent[i], uSent + i + 1, uSent_size - i - 1);
   assert(result > -INFINITY);
   return result;
} // value
