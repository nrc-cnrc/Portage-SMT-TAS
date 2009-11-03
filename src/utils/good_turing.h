/**
 * @author George Foster
 * @file good_turing.h  Good-Turing frequency smoothing.
 * 
 * 
 * COMMENTS: 
 *
 * For words (or ngrams, etc) occuring with frequency f in a corpus, the
 * Good-Turing estimate of their true frequency is f* = (f+1) N_(f+1) / N_f,
 * where N_f is the number of words with frequency f. This can be justified as
 * leave-one-out cross-validation, among other things.
 *
 * A practical difficulty with the above formula is that the N_f's are noisy,
 * particulary for large f. The solution used here is to smooth them using a
 * weighted least-squares fit to a log(N_f) vs log(f) curve. The more reliable
 * values of N_f for low f are emphasized by making the weight on (the error
 * associated with) each data point proportional to N_f.
 *
 * Another problem is that the 0* estimate is undefined because N_0 isn't known.
 * However, the total mass of unseen events, 0* N_0 = N_1, IS known. This is 
 * supplied by the function zeroCountMass(), from which an application can
 * calculate a value for either 0* or N_0 by assuming a value for the other
 * quantity.
 *
 * Although the standard Good-Turing formulation uses integer frequencies,
 * partial support is provided here for floating-point frequencies. Initial
 * creation still requires integer freqs, which can be obtained by binning,
 * for example; but once this is done, the smoothing function takes either
 * integer or float arguments.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef GOOD_TURING_H
#define GOOD_TURING_H

#include <cmath>
#include <portage_defs.h>
#include "ls_poly_fit.h"

namespace Portage {

/// Good-Turing frequency smoothing.
class GoodTuring {

   static const Uint poly_order = 1; ///< order of smoothing polynomial
   vector<double> poly_coeffs;  ///< polynomial coefficients c_0, c_1, ... c_n

   void constructFromCounts(Uint n, const Uint freqs[], const Uint freq_counts[]);

   double calcPolyLog(double freq) const {
      return Portage::calcPoly(log(freq), poly_order, &poly_coeffs[0]);
   }

public:
   
   /**
    * Construct from raw frequencies.
    * @param n size of item_freqs
    * @param item_freqs a frequency for each word
    */
   GoodTuring(Uint n, const Uint item_freqs[]);
   
   /**
    * Construct from frequencies and frequency counts.
    * @param n size of freqs and freq_counts
    * @param freqs a list of frequencies
    * @param freq_counts freq_counts[i] contains number of words having
    * frequency freqs[i]
    */
   GoodTuring(Uint n, const Uint freqs[], const Uint freq_counts[]) 
      {constructFromCounts(n, freqs, freq_counts);}

   /**
    * Return the Good-Turing estimate for a given non-zero frequency.
    * @param freq
    * @return Returns the Good-Turing estimate for a given non-zero frequency.
    */
   template<class T>
   double smoothedFreq(T freq) const {
      return (freq+1) * exp(calcPolyLog((double)(freq+1)) - calcPolyLog((double)freq));
   }

   /**
    * Return the smoothed count for a given non-zero frequency.
    * @param freq
    * @return Returns the moothed count for a given non-zero frequency.
    */
   template<class T>
   double smoothedCount(T freq) const {return exp(calcPolyLog(freq));}

   /**
    * Return the total count mass for 0-frequency events.
    * @return Returns the total count mass for 0-frequency events.
    */
   double zeroCountMass() {return exp(calcPolyLog(1));}

};
}
#endif
