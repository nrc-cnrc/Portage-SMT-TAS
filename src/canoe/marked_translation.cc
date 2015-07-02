/**
 * @author Samuel Larkin
 * @file marked_translation.cc
 * Implementation of MarkedTranslation.
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

#include "marked_translation.h"

using namespace Portage;

bool MarkedTranslation::operator==(const MarkedTranslation &b) const
{
   return src_words == b.src_words
          && markString == b.markString
          && log_prob == b.log_prob;
} // operator==

ostream& MarkedTranslation::toJSON(ostream& out) const {
   out << "{";
   out << to_JSON("src_words", src_words);
   out << ',';
   out << to_JSON("log_prob", log_prob);
   out << ',';
   out << to_JSON("markString", markString);
   out << ',';
   out << to_JSON("class_name", class_name);
   out << "}";
   return out;
}

namespace Portage {
   ostream& operator<<(ostream& out, const MarkedTranslation& o) {
      return o.toJSON(out);
   }
};

