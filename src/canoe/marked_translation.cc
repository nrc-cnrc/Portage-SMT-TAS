/**
 * @author Samuel Larkin
 * @file marked_translation.cc
 * Implementation of MarkedTranslation.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "marked_translation.h"

using namespace Portage;

bool MarkedTranslation::operator==(const MarkedTranslation &b) const
{
   return src_words == b.src_words
          && markString == b.markString
          && log_prob == b.log_prob;
} // operator==

