/**
 * @author Nicola Ueffing
 * $Id$
 * @file nbest_posterior_ff.cc
 * 
 * N-best Rescoring Module
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada 
 * 
 * Contains the implementation of all NBest Posterior feature functions:
 *   nbestWordPostLev_ff, nbestWordPostSrc_ff, nbestWordPostTrg_ff
 *   nbestPhrasePostSrc_ff, nbestPhrasePostTrg_ff
 *   nbestNgramPost_ff, nbestSentLenPost_ff
 * which are feature functions based on word / phrase / sentence-length
 * posterior probabilities which have been calculated over N-best lists
 */

#include <fstream>
#include "nbest_posterior_ff.h"

using namespace Portage;

nbestPosteriorBase_ff::nbestPosteriorBase_ff(const string& args, NBestPosterior* const nbconf) 
: FeatureFunction(args)
, nbconf(nbconf)
, ff_scale(1.0f)
{
   assert(nbconf);
}

nbestPosteriorBase_ff::~nbestPosteriorBase_ff() 
{
   assert(nbconf);
   if (nbconf) delete nbconf;
}

bool nbestPosteriorBase_ff::loadModelsImpl()
{
   nbconf->setFFProperties(ff_wts_file, ff_prefix, ff_scale);
   return true;
}

bool nbestPosteriorBase_ff::parseAndCheckArgs()
{
   string my_args = argument;        // Make a copy to strip parsed parts
   string::size_type pos = 0;

   // Scale
   pos = my_args.find("#");
   const string s = my_args.substr(0, pos);
   if (!s.empty()) {
      ff_scale = atof(s.c_str());
   }
   else {
      error(ETFatal, "You must provide a scale factor");
      return false;
   }

   // Feature function weight file
   if (pos != string::npos) {
      my_args = my_args.substr(pos+1);
      pos = my_args.find("#");
      ff_wts_file = my_args.substr(0, pos);
   }
   else {
      error(ETFatal, "You must provide a weight file");
      return false;
   }

   // Friendly feature_function_tool -check reminder
   // This is not a syntax error, just a friendly warning
   static bool beenWarned = false;
   if (ff_wts_file == "<ffval-wts>") {
      if (!beenWarned) {
         error(ETWarn, "Don't forget to give a canoe.ini to gen-features-parallel.sh");
         beenWarned = true;
      }
   }
   else {
      if (!check_if_exists(ff_wts_file)){
         error(ETFatal, "File is not accessible: %s", ff_wts_file.c_str());
         return false;
      }
   }

   // Prefix
   if (pos != string::npos)
      ff_prefix = my_args.substr(pos+1);

   return true;
}

void nbestPosteriorBase_ff::source(Uint s, const Nbest * const nbest) {
   FeatureFunction::source(s, nbest);
   nbconf->clearAll();
   nbconf->setNB(*nbest);
   nbconf->computePosterior(s);
}

double
nbestPosteriorBase_ff::computeValue(const Translation& trans) {
   nbconf->init(trans.getTokens());
   if (nbconf->getNBsize()==0)
      return INFINITY;
   else
      return nbconf->sentPosteriorOne();
}

vector<double>
nbestPosteriorBase_ff::computeValues(const Translation& trans) {
   nbconf->init(trans.getTokens());
   if (nbconf->getNBsize()==0)
      return vector<double>(1,INFINITY);
   else
      return nbconf->wordPosteriorsOne();
}


/**************************** WORD LEV ****************************/

nbestWordPostLev_ff::nbestWordPostLev_ff(const string& args)
: nbestPosteriorBase_ff(args, new NBestWordPostLev)
{}

double
nbestWordPostLev_ff::computeValue(const Translation& trans) {
   nbconf->init(trans.getTokens());
   return nbconf->sentPosteriorOne();
}

vector<double>
nbestWordPostLev_ff::computeValues(const Translation& trans) {
   nbconf->init(trans.getTokens());
   return nbconf->wordPosteriorsOne();
}


/**************************** WORD SRC ****************************/

nbestWordPostSrc_ff::nbestWordPostSrc_ff(const string& args)
: nbestPosteriorBase_ff(args, new NBestWordPostSrc)
{}

double
nbestWordPostSrc_ff::computeValue(const Translation& trans) {
   nbconf->setAlig(*trans.alignment);
   return nbestPosteriorBase_ff::computeValue(trans);
}

vector<double>
nbestWordPostSrc_ff::computeValues(const Translation& trans) {
   nbconf->setAlig(*trans.alignment);
   return nbestPosteriorBase_ff::computeValues(trans);
}


/**************************** WORD TRG ****************************/

nbestWordPostTrg_ff::nbestWordPostTrg_ff(const string& args) 
: nbestPosteriorBase_ff(args, new NBestWordPostTrg)
{}


/**************************** PHRASE SRC ****************************/

nbestPhrasePostSrc_ff::nbestPhrasePostSrc_ff(const string& args) 
: nbestPosteriorBase_ff(args, new NBestPhrasePostSrc) 
{}

double
nbestPhrasePostSrc_ff::computeValue(const Translation& trans) {
   nbconf->setAlig(*trans.alignment);
   return nbestPosteriorBase_ff::computeValue(trans);
}

vector<double>
nbestPhrasePostSrc_ff::computeValues(const Translation& trans) {
   nbconf->setAlig(*trans.alignment);
   return nbestPosteriorBase_ff::computeValues(trans);
}


/**************************** PHRASE TRG ****************************/

nbestPhrasePostTrg_ff::nbestPhrasePostTrg_ff(const string& args) 
: nbestPosteriorBase_ff(args, new NBestPhrasePostTrg)
{}

double
nbestPhrasePostTrg_ff::computeValue(const Translation& trans) {
   nbconf->setAlig(*trans.alignment);
   return nbestPosteriorBase_ff::computeValue(trans);
}

vector<double>
nbestPhrasePostTrg_ff::computeValues(const Translation& trans) {
   nbconf->setAlig(*trans.alignment);
   return nbestPosteriorBase_ff::computeValues(trans);
}


/**************************** N-GRAMS****************************/

nbestNgramPost_ff::nbestNgramPost_ff(const string& args) 
: nbestPosteriorBase_ff(args.substr(args.find("#")+1, args.size()-args.find("#")), new NBestNgramPost) 
, maxN(atoi(args.substr(0, args.find("#")).c_str()))
{}

bool nbestNgramPost_ff::loadModelsImpl()
{
   nbconf->setMaxN(maxN);
   nbconf->setFFProperties(ff_wts_file, ff_prefix, ff_scale);
   return true;
}


/**************************** SENTENCE LENGTH ****************************/

nbestSentLenPost_ff::nbestSentLenPost_ff(const string& args) 
: nbestPosteriorBase_ff(args, new NBestSentLenPost) 
{}

