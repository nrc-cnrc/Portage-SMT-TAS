/**
 * @author George Foster
 * @file dmstruct.cc  Represent distortion model counts
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */


#include "dmstruct.h"

using namespace Portage;

std::ostream& Portage::operator<<(std::ostream& os, const DistortionCount& dc) {
   os << dc.val(0);
   for (Uint i = 1; i < dc.size(); ++i)
      os << ' ' << dc.val(i);
   return os;
}
