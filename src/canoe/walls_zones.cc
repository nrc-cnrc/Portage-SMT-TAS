/**
 * @author Eric Joanis
 * @file walls_zones.cc  Wall and zone decoder feature implementation file.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#include "walls_zones.h"
#include "basicmodel.h"

using namespace Portage;

// Alright, I'm being lazy. A lot more of the implementations could (and maybe
// should) be here. Create() is here because it won't compile without seeing
// the rest of walls_zones.h first. The rest doesn't absolutely have to be here
// and I don't feel like moving it here.
// The compilation overhead is not significant because walls_zones.h is only
// included by this file and by distortionmodel.cc

WallOrZone* WallOrZone::Create(string featureName, string arg, bool fail) {
   // Wall and zone types are Strict, WordStrict and Loose.
   // Wall and zone classes are any string that is then matched to the
   // name="NAME" attribute of each wall or zone.
   string type, nameAttribute;
   splitNameAndArg(arg, type, nameAttribute);
   WallOrZone* m = NULL;
   if (featureName == "Walls") {
      if (type == "" || type == "Strict")
         m = new StrictWallsFeature();
      else if (type == "WordStrict")
         m = new WordStrictWallsFeature();
      else if (type == "Loose")
         m = new LooseWallsFeature();
      else if (fail)
         error(ETFatal, "Unknown Walls type: %s; valid types are Strict, WordStrict and Loose", type.c_str());
   }
   else if (featureName == "Zones") {
      if (type == "" || type == "Strict")
         m = new StrictZonesFeature();
      else if (type == "" || type == "WordStrict")
         m = new WordStrictZonesFeature();
      else if (type == "" || type == "Loose")
         m = new LooseZonesFeature();
      else if (fail)
         error(ETFatal, "Unknown Zones type: %s; valid types are Strict, WordStrict and Loose", type.c_str());
   }
   else if (featureName == "LocalWalls") {
      if (type == "" || type == "Strict")
         m = new StrictLocalWallsFeature();
      else if (type == "" || type == "WordStrict")
         m = new WordStrictLocalWallsFeature();
      else if (type == "" || type == "Loose")
         m = new LooseLocalWallsFeature();
      else if (fail)
         error(ETFatal, "Unknown LocalWalls type: %s; valid types are Strict, WordStrict and Loose", type.c_str());
   }
   else
      assert(false && "It is a programming error to call WallOrZone::Create() with featureName not Walls or Zones");

   if (m) m->name = nameAttribute;
   return m;
}

double StrictZonesFeature::futureScore(const PartialTranslation &pt)
{
   assert(p_info && p_info->model);
   double result = 0.0;
   const UintSet &cov(pt.sourceWordsNotCovered);
   Uint first_non_covered_word = (cov.empty() ? sentLength : cov[0].start);
   for (Uint i = 0; i < zones.size(); ++i) {
      Range zone(zones[i].range);
      if (zone.start > first_non_covered_word &&     // there are non-cov'd word before zone
          !isSubset(zone, cov) &&                    // zone was started
          !isDisjoint(zone, cov))                    // but not completed
      {
         // Check that we can complete the zone without violating the
         // distortion limit by checking if adding a phrase covering the
         // right-edge of the zone would respect the distortion limit.

         // This is a heuristic test I'm fairly confident in, but I did not
         // prove whether it is a necessary and/or sufficient condition to
         // prevent dead-end hypotheses entering the stack when using hard
         // zones.
         //  - when using the strict DL, I'm confident this test is exact
         //  - when using extended DL, a weird beast I wish I never
         //    implemented, this test will reject some cases that could have
         //    been completed.
         //  - when using simple DL, again, I'm confident this test is exact
         //  - when using ITG DL, there are no guarantees of any kind! But
         //    Colin pointed out combining hard zones and ITG DL is crazy
         //    stuff, and who knows in how many other ways things might break.
         UintSet zone_non_cov;
         intersectRange(zone_non_cov, cov, zone);
         assert(!zone_non_cov.empty());
         assert(zone_non_cov != zone);
         if (zone_non_cov.back().end == zone.end) {
            if (!p_info->model->respectsDistortionLimit(pt, zone_non_cov.back()))
               result -= 1.0;
         }
      }
   }
   return result;
}
