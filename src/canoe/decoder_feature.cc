/**
 * @author Eric Joanis
 * @file decoder_feature.cc  Abstract parent class for all features used
 *                           in the decoder, except LMs and TMs.
 *
 * $Id$
 *
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches du Canada / 
 * Copyright 2006, National Research Council of Canada
 */

#include "decoder_feature.h"
#include "segmentmodel.h"
#include "distortionmodel.h"
#include "length_feature.h"
#include <errors.h>

using namespace Portage;

DecoderFeature* DecoderFeature::create(BasicModelGenerator* bmg,
                                       const string& group, const string& name,
                                       const string& args, bool fail)
{
   DecoderFeature* f = NULL;

   if ( group == "SegmentationModel" ) {
      f = SegmentationModel::create(name, args, fail);
   } else if ( group == "DistortionModel" ) {
      f = DistortionModel::create(name, args, fail);
   } else if ( group == "LengthFeature" ) {
      f = new LengthFeature();
   } else if ( fail ) {
      error(ETFatal, "unknown decoder feature: " + group);
   }

   if ( f && f->description.empty() ) {
      f->description = group;
      if ( name != "" ) f->description += ":" + name;
      if ( args != "" ) f->description += ":" + args;
   }

   return f;
}
