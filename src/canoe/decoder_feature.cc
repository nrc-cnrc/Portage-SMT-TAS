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
#include "rule_feature.h"
#include "errors.h"

using namespace Portage;

DecoderFeature* DecoderFeature::create(BasicModelGenerator* bmg,
                                       const string& group, const string& name,
                                       const string& args, bool fail)
{
   DecoderFeature* f = NULL;

   if ( group == "SegmentationModel" ) {
      f = SegmentationModel::create(name, fail);
   } else if ( group == "DistortionModel" ) {
      f = DistortionModel::create(name, fail);
   } else if ( group == "IBM1FwdFeature" ) {
      f = new IBM1FwdFeature(bmg, args);
   } else if ( group == "LengthFeature" ) {
      f = new LengthFeature();
   } else if ( group == RuleFeature::name ) {
      f = new RuleFeature(bmg, args);
   } else if ( fail ) {
      error(ETFatal, "unknown decoder feature: " + group);
   }

   // We MUST return a valid pointer or else it's an error.
   if (f == NULL) {
      error(ETFatal, "Invalid %s when initializing decoder feature with %s %s",
         group.c_str(), name.c_str(), args.c_str());
   }
   
   if ( f && f->description.empty() ) {
      f->description = group;
      if ( name != "" ) f->description += ":" + name;
      if ( args != "" ) f->description += ":" + args;
   }

   return f;
}
