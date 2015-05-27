/**
 * @author Colin Cherry
 * @file nnjm_abstract.cc  Interface to evaluate an nnjm
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#ifndef NNJM_ABS_H
#define NNJM_ABS_H

#include <string>
#include <vector>
#include "portage_defs.h"

namespace Portage
{

class NNJMAbstract {
public:
   virtual ~NNJMAbstract() {}

   typedef vector<Uint>::const_iterator VUI;
   virtual double logprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w, Uint src_pos) = 0;
   virtual double logprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w) = 0;
   virtual void newSrcSent(const vector<Uint>& src_pad, Uint srcWindow) {};
};

class NNJMs {
private:
   NNJMs(); // Private constructor to dodge construction
public:
   //static NNJMAbstract* new_PyWrap(const std::string& modelfile, Uint swin_size, Uint ng_size, Uint format=0);
   static NNJMAbstract* new_Native(const std::string& modelfile, bool selfnorm, bool useLookup);
};
   
}
#endif // NNJM_ABS_H
