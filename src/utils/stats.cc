// $Id$
/**
 * @author Evan Stratford
 * @file stats.cc
 * @brief 
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include "stats.h"
#include <sstream>

using namespace Portage;

void CountStat::write(ostream& out) const {
   print(out, "count", c);
}

void TotalStat::write(ostream& out) const {
   print(out, "total", c);
}

void AvgVarStat::write(ostream& out) const {
   print(out, "avg", m);
   print(out, "var", v/(n-1));
}

void AvgVarTotalStat::write(ostream& out) const {
   AvgVarStat::write(out);
   print(out, "total", total);
}

void HistogramStat::write(ostream& out) const {
   map<int, Uint>::const_iterator iter;
   for (iter = h.begin(); iter != h.end(); ++iter) {
      ostringstream tmp;
      tmp << iter->first;
      print(out, tmp.str(), iter->second);
   }
}

