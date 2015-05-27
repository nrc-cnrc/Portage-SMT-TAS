/**
 * @author Eric Joanis
 * @file decoder_feature.cc  Abstract parent class for all features used
 *                           in the decoder, except LMs and TMs.
 *
 * $Id$
 *
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "decoder_feature.h"
#include "segmentmodel.h"
#include "distortionmodel.h"
#include "ibm_feature.h"
#include "length_feature.h"
#include "levenshtein_feature.h"
#include "ngrammatch_feature.h"
#include "rule_feature.h"
#include "unal_feature.h"
#include "sparsemodel.h"
#include "bilm_model.h"
#include "nnjm.h"
#include "basicmodel.h"
#include "errors.h"

using namespace Portage;

DecoderFeature* DecoderFeature::create(BasicModelGenerator* bmg,
                                       const string& group,
                                       const string& args, bool fail, Uint verbose)
{
   DecoderFeature* f = NULL;

   if ( group == "SegmentationModel" ) {
      f = SegmentationModel::create(args, fail);
   } else if ( group == "DistortionModel" ) {
      f = DistortionModel::create(args, bmg->c, fail);
   } else if ( group == "IBM1FwdFeature" ) {
      f = new IBM1FwdFeature(bmg, args);
   } else if ( group == "LengthFeature" ) {
      f = new LengthFeature();
   } else if ( group == "LevFeature" ) {
      f = new LevenshteinFeature(bmg);
   } else if ( group == "NgramMatchFeature" ) {
      f = new NgramMatchFeature(args);
   } else if ( group == RuleFeature::name ) {
      f = new RuleFeature(bmg, args);
   } else if ( group == "UnalFeature" ) {
      f = new UnalFeature(args);
   } else if ( group == "SparseModel" ) {
      f = new SparseModel(args, DirName(bmg->c->configFile), verbose, bmg->getOpenVoc(),
                          true, bmg->c->sparseModelAllowNonLocalWts, false, true);
   } else if ( group == "BiLMModel" ) {
      f = new BiLMModel(bmg, args);
   } else if ( group == "NNJM" ) {
      f = new NNJM(bmg, args);
   } else {
      if ( fail )
         error(ETFatal, "unknown decoder feature: " + group);
      else
         return NULL;
   }

   // We MUST return a valid pointer or else it's an error.
   if (f == NULL) {
      error(ETFatal, "Invalid %s when initializing decoder feature with %s",
         group.c_str(), args.c_str());
   }
   
   if ( f && f->description.empty() ) {
      f->description = group;
      if ( args != "" ) f->description += ":" + args;
   }

   return f;
}

Uint64 DecoderFeature::totalMemmapSize(const string& group, const string& args)
{
   if ( group == "BiLMModel" )
      return BiLMModel::totalMemmapSize(args);
   else
      return 0;
}
