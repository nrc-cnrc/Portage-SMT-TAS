/**
 * @author George Foster
 * @file colours.h
 *
 * A list of colours that are useful for colour annotation, eg for alignments.
 * Colours that are close together in the list are supposed to be easy to
 * distinguish.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 *
 */

#ifndef COLOURS_H
#define COLOURS_H

#include "portage_defs.h"
#include <vector>

using namespace std;

namespace Portage
{

class ColourInfo {

   Uint c;                      // current colour

public:

   ColourInfo() : c(0) {}

   // RGB and RGB-string representations of a colour.

   struct Colour {
      Uint r;
      Uint g;
      Uint b;
      const char* rgb_str;       // "r,g,b"
      Colour(Uint r, Uint g, Uint b, const char* rgb_str) : 
         r(r), g(g), b(b), rgb_str(rgb_str) {}
   };
   static const vector<Colour> colours; // list of colours

   /**
    * Get the next color in the list (assumes list not empty).
    */
   const Colour& next() 
   {
      if (c == colours.size()) c = 0;
      return colours[c++];
   }

   /**
    * Reset list position.
    */
   void restart() {c = 0;}

};
}

#endif // COLOURS_H
