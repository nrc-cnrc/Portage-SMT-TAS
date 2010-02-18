/**
 * @author Darlene Stewart
 * @file tpt_error.h Exception and error handling in the tpt module.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include "tpt_error.h"
#include "tpt_typedefs.h"

namespace ugdiss {

const char* const efatal = "Error: ";
const char* const ewarn = "Warning: ";

ostream& exit_1(ostream& os)
{
   os << endl;
   exit(1);
   return os;
}

} // namespace ugdiss