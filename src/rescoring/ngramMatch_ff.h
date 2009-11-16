/**
 * @author Michel Simard
 * @file ngramMatch_ff.h  This module implements a ngram-match feature function for rescoring.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef NGRAMMATCH_FF_H
#define NGRAMMATCH_FF_H

#include <featurefunction.h>
#include <ngram.h>
#include <set>

using namespace Portage;
using namespace std;

namespace Portage
{
  template<class T> struct NGramLessThan {
    bool operator()(const NGram<T> &n1, const NGram<T> &n2) const {
      return n1 < n2;
    }
  };

  typedef multiset< NGram< string >, NGramLessThan<string> > NGramSet;

  /// This module implements a ngram-match feature function for rescoring.
  class NGramMatchFF: public FeatureFunction {
  public:
    /// Constructor.
    /// @param args  arguments
    NGramMatchFF(const string &args);
    /// Destructor.
    virtual ~NGramMatchFF();

    virtual Uint requires() { return FF_NEEDS_TGT_TOKENS; }
    virtual FeatureFunction::FF_COMPLEXITY cost() const { return HIGH; }
    virtual bool parseAndCheckArgs();
    virtual bool loadModelsImpl();
    virtual void source(Uint s, const Nbest * const nbest);
    virtual double value(Uint k);

  private:
    Uint ngram_size;
    string ref_file;
    vector<string> ref;
    NGramSet ref_ngrams;
  };

}

#endif // NGRAMMATCH_FF_H
