/**
 * @author Michel Simard
 * @file levenshtein_ff.h  This module implements a ngram-match feature function for rescoring.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef LEVENSHTEIN_FF_H
#define LEVENSHTEIN_FF_H

#include <featurefunction.h>
#include <levenshtein.h>

using namespace Portage;
using namespace std;

namespace Portage
{
  /// This module implements a levenshtein distance feature function for rescoring.
  class LevenshteinFF: public FeatureFunction {
  public:
    /// Constructor.
    /// @param args  arguments
    LevenshteinFF(const string &args);
    /// Destructor.
    virtual ~LevenshteinFF();

    virtual Uint requires() { return FF_NEEDS_TGT_TOKENS; }
    virtual FeatureFunction::FF_COMPLEXITY cost() const { return HIGH; }
    virtual bool parseAndCheckArgs();
    virtual bool loadModelsImpl();
    virtual void source(Uint s, const Nbest * const nbest);
    virtual double value(Uint k);

  private:
    Levenshtein<string> levenshtein;
    string ref_file;
    vector<string> ref;
    Tokens ref_tokens;
  };

}

#endif // LEVENSHTEIN_FF_H
