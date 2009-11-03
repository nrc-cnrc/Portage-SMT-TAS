/**
 * @author George Foster
 * @file tm_io.cc  Implementation of tm_io utilities.
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <str_utils.h>
#include "tm_io.h"


// Required so by doxygen
namespace Portage {
namespace TMIO {

string& joinTokens(const vector<string>& tokens, string& line, bool bracket)
{
   for (vector<string>::const_iterator p = tokens.begin(); p != tokens.end(); ++p) {
      if (bracket) line += "[";
      line += *p;
      if (bracket) line += "]";
      if (p != tokens.end()-1)
	 line += " ";
   }
   return line;
}

} // ends namespace TMIO
} // ends namespace Portage
