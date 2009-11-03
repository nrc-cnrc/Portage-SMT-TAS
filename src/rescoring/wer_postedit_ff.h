/**
 * @author Nicola Ueffing
 * @file wer_postedit_ff.h
 *
 * $Id$
 *
 * N-best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 *
 * Contains the declaration of the WER feature function for statistical post-editing.
 * Note that this is only useful for automatic post-editing in which source and target 
 * language are the same (well, maybe also for Catalan -> Castilian translation ;-)
 */

#ifndef WER_POSTEDIT_FF_H
#define WER_POSTEDIT_FF_H

#include "featurefunction.h"
#include <wer.h>
#include <vector>
#include <string>

using namespace Portage;

namespace Portage
{
  class WerPostedit : public FeatureFunction {

  private:
    vector<Tokens> src;
    float          srclen;

  public:
    WerPostedit(const string& args);
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
   }; // WerPostedit

} // Portage

#endif // WER_POSTEDIT_FF_H
