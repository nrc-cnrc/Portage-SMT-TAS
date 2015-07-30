/**
 * @author Samuel Larkin
 * @file wordClass.h
 * @brief An adapter class for word classes.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */
#include "wordClass.h"


namespace Portage {

IWordClass* loadClasses(const string& fname) {
   IWordClass* wc = NULL;
   if (isSuffix(".mmMap", fname)) {
      wc = new WordClassTightlyPacked(fname);
   }
   else {
      wc = new WordClass(fname);
   }
   return wc;
}

};
