/**
 * @author Nicola Ueffing
 * @file ibm1del.cc  Implementation of IBM1DeletionBase
 *
 * $Id$
 * 
 * N-best Rescoring Module
 * 
 * Contains the implementation of IBM1Deletion, which is a feature function
 * defined as follows:
 * \f$ \frac{1}{J} \sum\limits_{j=1}^{J} \delta ( \max\limits_{e\in \mbox{tgt\_vocab}} p(e|f_j)) \f$
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada 
 */

#include "featurefunction.h"
#include "ibm1del.h"
#include <ttablewithmax.h>
#include <vector>
#include <string>

using namespace std;
using namespace Portage;

IBM1DeletionBase::IBM1DeletionBase(const string& args)
: table( args.substr(0,args.find("#")) )
, thr(0.1f)
{
   const string::size_type pos = args.find("#");
   if (pos != string::npos)
      thr = atof(args.substr(pos+1,args.size()-pos).c_str());
}

double 
IBM1DeletionBase::computeValue(const Tokens& src, const Tokens& tgt) {
   double ratioDel = 0.0f;

   for (Uint j = 0; j < src.size(); j++) {
      double curMax = 0;
      for (Uint i = 0; i < tgt.size(); i++) 
         curMax = max( curMax, table.getProb(src[j], tgt[i]) );
      if (curMax < thr)
         ratioDel++;
   } // for j
   return ratioDel/double(src.size());
} // IBM1DeletionBase
