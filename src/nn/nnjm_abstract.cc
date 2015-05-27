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

#include "nnjm_abstract.h"
//#include "nnjm_pywrap.h"
#include "nnjm_native.h"

using namespace Portage;

/*
NNJMAbstract* NNJMs::new_PyWrap(const std::string& modelfile, Uint swin_size, Uint ng_size, Uint format) {
   return new NNJMPyWrap(modelfile, swin_size, ng_size, format);
}
*/

NNJMAbstract* NNJMs::new_Native(const std::string& modelfile, bool selfnorm, bool useLookup) {
   return new NNJMNative(modelfile, selfnorm, useLookup);
}
