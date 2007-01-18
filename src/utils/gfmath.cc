/**
 * @author George Foster, Eric Joanis
 * @file gfmath.cc  Simple math stuff.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include "gfmath.h"

using namespace Portage;

static const double l2 = log(2.0);

double Portage::log2(double x) 
{
   return log(x) / l2;
}

double Portage::exp2(double x) 
{
   return exp(x * l2);
}

Uint Portage::next_power_of_2(Uint x)
{
   if ( x < 3 ) return x;
   return (Uint)pow(2.0, 1+int(floor(log2(x-1))));
}
