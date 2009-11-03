/**
 * @author Samuel Larkin
 * @file marked_translation.h
 * Structure to hold the information for one translation for one  mark-up.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef __MARKED_TRANSLATION__H__
#define __MARKED_TRANSLATION__H__

#include "canoe_general.h"
#include <vector>
#include <string>

namespace Portage {
   using namespace std;

   /**
    * translation marked in the decoder input.
    */
   struct MarkedTranslation
   {
      Range src_words;            ///< marked source range
      vector<string> markString;  ///< translation provided on input
      double log_prob;            ///< probability provided on input
      string class_name;          ///< if set, indicate the rule's classname

      /**
       * Test equality of two MarkedTranslation objects.
       * @param b other MarkedTranslation
       * @return true iff *this and b and identical
       */
      bool operator==(const MarkedTranslation &b) const;
   }; // MarkedTranslation
}; // ends namespace Portage 

#endif // ends __MARKED_TRANSLATION__H__

