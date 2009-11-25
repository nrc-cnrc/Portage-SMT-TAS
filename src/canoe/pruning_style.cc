/**
 * @author Samuel Larkin
 * @file pruning_style.cc
 *
 * $Id$
 *
 * Calculate the pruning value for filtering phrase tables.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include "pruning_style.h"
#include "str_utils.h"
#include <sstream>
#include <iostream>

using namespace Portage;
using namespace std;

// pruningStyle
pruningStyle* pruningStyle::create(const string& type, Uint limit) {
   if (type == "") {
      // Default is fix#limit
      cerr << "Using fixPruning with " << limit << endl;  // SAM DEBUG
      return new fixPruning(limit);
   }
   else if (isPrefix("fix", type)) {
      const size_t hash_pos = type.rfind('#');
      if ( hash_pos != string::npos ) {
          if (!conv(type.substr(hash_pos+1), limit)) {
             error(ETFatal, "Failed to convert %s", type.c_str());
          }
      }
      else {
         error(ETWarn, "You didn't provide a limit, example fix#%d", limit);
      }
      cerr << "Using fixPruning with " << limit << endl;  // SAM DEBUG
      return new fixPruning(limit);
   }
   else if (isPrefix("linear", type)) {
      const size_t hash_pos = type.rfind('#');
      if ( hash_pos != string::npos ) {
          if (!conv(type.substr(hash_pos+1), limit)) {
             error(ETFatal, "Failed to convert %s", type.c_str());
          }
      }
      else {
         error(ETWarn, "You didn't provide a limit, linear#%d", limit);
      }
      cerr << "Using linearPruning with " << limit << endl;  // SAM DEBUG
      return new linearPruning(limit);
   }
   else {
      error(ETFatal, "Invalid pruning style (%s).", type.c_str());
   }

   return NULL;
}

// fixPruning
Uint fixPruning::operator()(Uint number_of_word) const {
   //cerr << "fixPruning(" << number_of_word << ")=" << L << endl;  // SAM DEBUG
   return L;
}

string fixPruning::description() const {
   ostringstream oss;
   oss << "fix pruning at " << L;
   return oss.str();
}


// linearPruning
Uint linearPruning::operator()(Uint number_of_word) const {
   //cerr << "linearPruning(" << number_of_word << ")=" << constant * number_of_word << endl;  // SAM DEBUG
   return constant * number_of_word;
}

string linearPruning::description() const {
   ostringstream oss;
   oss << "linear pruning with Nx" << constant;
   return oss.str();
}
