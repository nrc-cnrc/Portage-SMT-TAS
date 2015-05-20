/**
 * @author George Foster
 * @file colours.cc
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 *
 */

#include "colours.h"

using namespace Portage;

static const ColourInfo::Colour colour_data[] = {
   ColourInfo::Colour(0,117,220, "0,117,220"),
   ColourInfo::Colour(240,163,255, "240,163,255"),
   ColourInfo::Colour(153,63,0, "153,63,0"),
   ColourInfo::Colour(76,0,92, "76,0,92"),
   ColourInfo::Colour(0,92,49, "0,92,49"),
   ColourInfo::Colour(43,206,72, "43,206,72"),
   ColourInfo::Colour(255,204,153, "255,204,153"),
   ColourInfo::Colour(128,128,128, "128,128,128"),
   ColourInfo::Colour(157,204,0, "157,204,0"),
   ColourInfo::Colour(143,124,0, "143,124,0"),
   ColourInfo::Colour(194,0,136, "194,0,136"),
   ColourInfo::Colour(0,51,128, "0,51,128"),
   ColourInfo::Colour(255,164,5, "255,164,5"),
   ColourInfo::Colour(255,168,187, "255,168,187"),
   ColourInfo::Colour(66,102,0, "66,102,0"),
   ColourInfo::Colour(255,0,16, "255,0,16"),
   ColourInfo::Colour(94,241,242, "94,241,242"),
   ColourInfo::Colour(0,153,143, "0,153,143"),
   ColourInfo::Colour(224,255,102, "224,255,102"),
   ColourInfo::Colour(116,10,255, "116,10,255"),
   ColourInfo::Colour(153,0,0, "153,0,0"),
   ColourInfo::Colour(255,255,0, "255,255,0"),
   ColourInfo::Colour(255,80,5, "255,80,5"),
   ColourInfo::Colour(25,25,25, "25,25,25"),
   ColourInfo::Colour(148,255,181, "148,255,181"),
};

const vector<ColourInfo::Colour> 
ColourInfo::colours(colour_data, colour_data + ARRAY_SIZE(colour_data));
