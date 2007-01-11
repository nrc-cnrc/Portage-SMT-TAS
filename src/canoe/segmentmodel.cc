/**
 * @author Michel Simard
 * @file segmentmodel.cc  Implementation of all Segmentation model related decoder features.
 * 
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / 
 * Copyright 2005, National Research Council of Canada
 */

#include <errors.h>
#include <math.h>
#include "segmentmodel.h"
#include <str_utils.h>
#include <fstream>
#include <iostream>

using namespace Portage;

/************************** SegmentModel ***********************************************************/

SegmentationModel* SegmentationModel::create(const string& name, const string& arg, bool fail)
{ 
   SegmentationModel* m = NULL;

   if (name == "count") {
      m = new SegmentCount();
      cerr << "Using ``count'' segmentation model" << endl;

   } else if (name == "bernoulli") {
      m = new BernoulliSegmentationModel(arg);
      cerr << "Using ``bernoulli'' segmentation model with Q=" << arg << endl;

   } else if (name == "uniform") {
      m = new BernoulliSegmentationModel("0.5");
      cerr << "Using ``uniform'' segmentation model" << endl;

   } else if (name == "none") {
      cerr << "Not using any segmentation model" << endl;

   } else if (fail)
      error(ETFatal, "unknown segmentation model: " + name);

   return m;
}

/************************** SegmentCount ***********************************************************/

double SegmentCount::score(const PartialTranslation& trans)
{
   return -1;
}

Uint SegmentCount::computeRecombHash(const PartialTranslation &pt)
{
   return 0;
}

bool SegmentCount::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   return true;
}

double SegmentCount::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   return -1;
}

double SegmentCount::futureScore(const PartialTranslation &trans)
{
   return 0;
}


/************************** BernoulliSegmentationModel ******************************************/

BernoulliSegmentationModel::BernoulliSegmentationModel(const string &arg) {
  double Q;
  conv(arg, Q);
  boundary = log(max(Q, 1e-6));
  not_boundary = log(max(1-Q, 1e-6));
}

double BernoulliSegmentationModel::score(const PartialTranslation& pt)
{
  int words = pt.lastPhrase->src_words.end - pt.lastPhrase->src_words.start;
  return boundary + (words - 1) * not_boundary;
}

Uint BernoulliSegmentationModel::computeRecombHash(const PartialTranslation &pt)
{
   return 0;
}

bool BernoulliSegmentationModel::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   return true;
}

double BernoulliSegmentationModel::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   int words = phrase_info.src_words.end - phrase_info.src_words.start;
   return boundary + (words - 1) * not_boundary;
}

double BernoulliSegmentationModel::futureScore(const PartialTranslation &trans)
{
   return 0;
}

