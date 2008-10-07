/**
 * @author Nicola Ueffing
 * @file bleu_postedit_ff.h
 *
 * $Id$
 *
 * N-best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 *
 * Contains the declaration of the BLEU feature function for statistical post-editing.
 * Note that this is only useful for automatic post-editing in which source and target 
 * language are the same (well, maybe also for Catalan -> Castilian translation ;-)
 */

#ifndef BLEU_POSTEDIT_FF_H
#define BLEU_POSTEDIT_FF_H

#include "featurefunction.h"
#include <bleu.h>
#include <vector>
#include <string>

using namespace Portage;

namespace Portage
{
  class BleuPostedit : public FeatureFunction {

  private:
    int smoothbleu; ///< Mandatory smoothing type for BLEU (see eval module for documentation)
    vector<Tokens> src;
    BLEUstats bleustats; 

  public:
    BleuPostedit(const string& args);
    virtual Uint requires() { 
      return  FF_NEEDS_TGT_TOKENS | FF_NEEDS_SRC_TOKENS;
    }
    virtual void source(Uint s, const Nbest * const nbest);
    virtual bool parseAndCheckArgs();
    
    virtual double value(Uint k) { return computeValue((*nbest)[k]); }
    virtual double computeValue(const Translation& trans);

    virtual FeatureFunction::FF_COMPLEXITY cost() const {
      return LOW;
    }
   }; // BleuPostedit

} // Portage

#endif // BLEU_POSTEDIT_FF_H
