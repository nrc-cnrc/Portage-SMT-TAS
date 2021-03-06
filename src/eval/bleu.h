/**
 * @author Aaron Tikuisis
 * @file bleu.h  BLEUstats which holds the ngrams count to calculate BLEU.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef BLEU_H
#define BLEU_H

//#define MAX_NGRAMS 4

#include "portage_defs.h"
#include "basic_data_structure.h"
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <math.h>   // exp

/// add this flag at compile to change DEFAULT_SMOOTHING_VALUE
/// -DDEFAULT_SMOOTHING_VALUE=2
#ifndef DEFAULT_SMOOTHING_VALUE
#define DEFAULT_SMOOTHING_VALUE 1
#endif

using namespace std;

namespace Portage
{
// EJJ 11AUG05: the BLEU_STAT_TYPE has to be signed because BLEUstats
// objects are sometimes used to store deltas, which can by definition be
// negative.  However, it is illegal to compute a score when any of the
// values are negative.
//typedef Uint BLEU_STAT_TYPE;
typedef double BLEU_STAT_TYPE;

/// A class which maintains statistics for the BLEU score.
class BLEUstats
{
private:
   typedef vector<BLEU_STAT_TYPE> BLEU_STATS;

public:
   /**
    * Holds the maximum N value in Ngrams when creating a BLEUstats.
    * This allows us to set its value a the beginning of a pgm execution
    * and then generate BLEUstats with N=MAX_NGRAMS.
    * 1 <= MAX_NGRAMS
    */
   static Uint MAX_NGRAMS;
   /**
    * Holds the maximum N value in Ngrams when using BLEUstats::score.
    * The value should be 1 <= MAX_NGRAMS_SCORE <= MAX_NGRAMS.
    */
   static Uint MAX_NGRAMS_SCORE;
   /// Defines the default smoothing type.
   static Uint DEFAULT_SMOOTHING;

public:
   /**
    * Gets the Maximum value of N in Ngrams
    * @return Returns the maximum value of N.
    */
   static Uint getMaxNgrams() { return MAX_NGRAMS; }
   /**
    * Use this function to set the maximum value of N in Ngrams.
    * This function prevents changing the value later during the execution of the pgm.
    * @param n  maximum value of N.
    */
   static void setMaxNgrams(const Uint n);

   /**
    * Gets the Maximum value of N in Ngrams when scoring
    * @return Returns the maximum value of N.
    */
   static Uint getMaxNgramsScore() { return MAX_NGRAMS_SCORE; }
   /**
    * Use this function to set the maximum value of N in Ngrams for scoring.
    * This function prevents changing the value later during the execution of the pgm.
    * @param n  maximum value of N for scoring.
    */
   static void setMaxNgramsScore(const Uint n);

   /**
    * Gets the default smoothing type.
    * @return Returns the default smoothing type.
    */
   static Uint getDefaultSmoothing() { return DEFAULT_SMOOTHING; }
   /**
    * Use this function to set the default smoothing type.
    * This function prevents changing the value later during the execution of the pgm.
    * @param n  default value smoothing type.
    */
   static void setDefaultSmoothing(const Uint n);


public:
   BLEU_STATS match;                  ///< ngrams match for n = [1 4]
   BLEU_STATS total;                  ///< maximum ngram match possible for n = [1 4]
   BLEU_STAT_TYPE length;             ///< candidate translation length
   BLEU_STAT_TYPE bmlength;           ///< best match reference length
   int smooth;                        ///< smoothing method

public:
   /**
    * Initializes a new BLEUstats with values of 0, the stats for an empty
    * set of translations.
    */
   BLEUstats();
   /**
    * Initializes a new BLEUstats with values of 0, the stats for an empty
    * set of translations and sets the smoothing type.
    * @param sm  smoothing type
    */
   BLEUstats(int sm);
   /**
    * Copy constructor.
    * @param s  BLEUstats to copy.
    */
   BLEUstats(const BLEUstats &s);
   /**
    * Initializes a new BLEUstats with values computed for the given
    * translation sentence.
    * @param trans  translation
    * @param refs   translation's references
    * @param sm     smoothing type
    */
   BLEUstats(const Sentence &trans, const References& refs, int sm = DEFAULT_SMOOTHING);
   /**
    * Initializes a new BLEUstats with values computed for the given
    * translation sentence.
    * @param tgt   translation
    * @param refs  translation's references.
    * @param sm    smoothing type
    */
   BLEUstats(const string &tgt, const vector<string> &refs, int sm = DEFAULT_SMOOTHING);
   /**
    * Initializes a new BLEUstats with values computed for the given
    * translation sentence.
    * @param tgt_words   contains the words for the translated sentence
    * being scored.
    * @param refs_words  is a vector of reference translations, each
    * element containing the words for a reference sentence.
    * @param sm          smoothing type
    */
   BLEUstats(const Tokens &tgt_words, const vector<Tokens>& refs_words, int sm);

   //@{
   /**
    * Calculates the ngram matches between the target and its references.
    * @param tgt_words   translation
    * @param refs_words  reference sentences.
    * @param sm          smoothing type
    */
   void init(const Sentence &trans, const References& refs, int sm);
   void init(const Tokens &trans, const vector<Tokens>& refs_words, int sm);
   void init(const vector<Uint> &trans, const vector< vector<Uint> >& refs_words, int sm);
   //@}

   /**
    * Computes the log BLEU score for this stats.
    * BLEU score is calculated in the following maner:
    * \f$ \log BLEU = max(1 - \frac{bmlength}{length}) + \sum_{n=1}^{N} (\frac{1}{N}) \log(\frac{match_n}{total_n}) \f$
    * The generalized BLEU score is actually given by
    * \f$ \log BLEU = max(1 - \frac{bmlength}{length}) + \sum_{n=1}^{N} w_n \log(\frac{match_n}{total_n}) \f$
    * for some choice of weights (\f$ w_n \f$).  Generally, the weights are \f$ \frac{1}{N} \f$, which is
    * what is used here.
    *
    * @param maxN     maximum N to calculated BLEU score
    * @param epsilon  smoothing value used when there is no ngram match.
    * @return Returns
    */
   double score(Uint maxN = MAX_NGRAMS_SCORE, double epsilon = 1e-30) const;

   BLEU_STAT_TYPE getBestMatchLength() const { return bmlength; }

   /// What is this metric's name.
   /// @return Returns this metric's name => BLEU.
   static const char* const name() {
      return "BLEU";
   }

   /**
    * Convert "internal" (log) score value to display format.
    * @param value internal value (eg, from score()) 
    * @return display value.
    */
   static double convertToDisplay(double value) {
      return exp(value);
   }
   /**
    * Convert display format to internal (log) format.
    * @param value display value
    * @return internal value.
    */
   static double convertFromDisplay(double value) {
      return log(value);
   }

   /**
    * Convert "internal" (log) score value to pnorm format: in [0,1],
    * higher scores are better. Identical to convertToDisplay() for BLEU,
    * but not for WER/PER!
    * @param value internal value (eg, from score()) 
    * @return pnorm value
    */
   static double convertToPnorm(double value) {
      return convertToDisplay(value);
   }
   /**
    * Convert "internal" (log) score value from pnorm format: in [0,1],
    * higher scores are better. Identical to convertFromDisplay() for BLEU,
    * but not for WER/PER!
    * @param value pnorm value 
    * @return internal value
    */
   static double convertFromPnorm(double value) {
      return convertFromDisplay(value);
   }

   /**
    * Prints the ngram count and match length in a human readable format to out.
    * @param out  output stream defaults to cout.
    */
   void output(ostream &out = cout) const;

   /**
    * Prints the ngrams count and match length so that it can be reread.
    * @param out  output stream mainly a file.
    */
   void write(ostream &out) const;

   /**
    * Reads from in in the format used by BLEUstats::write().
    * @param in  input stream.
    */
   void read(istream &in);

   /**
    * Finds the difference in statistics between two BLEUstats objects.
    * @relates BLEUstats
    * @param other  right-hand side operand
    * @return Returns a BLEUstats containing this - other
    */
   BLEUstats& operator-=(const BLEUstats& other);

   /**
    * Adds together the statistics of two BLEUstats objects, returning
    * the result.
    * @relates BLEUstats
    * @param other  right-hand side operand
    * @return Returns a BLEUstats containing this + other
    */
   BLEUstats& operator+=(const BLEUstats& other);

   /// Callable entity for booststrap confidence interval. 
   struct CIcomputer
   {
      // Seamingly pointless constructor quiets spurious warning from g++ 4.4.4
      CIcomputer() {}
      // Seamingly pointless copy constructor quiets spurious warning from g++ 4.4.4
      CIcomputer(const CIcomputer&) {}

      /// Define what is an iterator for a CIcomputer.
      typedef vector<BLEUstats>::const_iterator iterator;

      /**
       * Cumulates all BLEUstats from the range.
       * @param begin  start iterator
       * @param end    end iterator
       * @return Returns the BLEU score [0 1] once the BLEU stats are all cumulated for the range.
       */
      double operator()(iterator begin, iterator end) {
         BLEUstats total;
         return exp(std::accumulate(begin, end, total).score());
      }
   };
};

/**
 * Finds the difference in statistics between two BLEUstats objects.
 * @relates BLEUstats
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns a BLEUstats containing s1 - s2
 */
BLEUstats operator-(const BLEUstats &s1, const BLEUstats &s2);
/**
 * Adds together the statistics of two BLEUstats objects, returning the result.
 * @relates BLEUstats
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns a BLEUstats containing s1 + s2
 */
BLEUstats operator+(const BLEUstats &s1, const BLEUstats &s2);
/**
 * Compares two BLEUstats for equality.
 * @relates BLEUstats
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns true iff all statistics are equal between s1 and s2
 */
bool operator==(const BLEUstats &s1, const BLEUstats &s2);
/**
 * Compares two BLEUstats for inequality.
 * @relates BLEUstats
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns true iff it exists one statistics that is not equal between s1 and s2
 */
bool operator!=(const BLEUstats &s1, const BLEUstats &s2);

/**
 * Scale BLEUstats by a constant.
 */
BLEUstats operator*(BLEUstats s, double c);

/**
 * Calculates the BLEUstats for a set of nbest lists and their references.
 * Fills in the 2-D array bleu in accordance with the input requirement for
 * powell() (and linemax()).
 * tgt_sents[s][0], .. , tgt_sents[s][K-1] are the candidate translations
 * corresponding to the reference translations ref_sents[s][0], .. ,
 * ref_sents[s][R-1].
 * @relates BLEUstats
 * @param bleu       returned BLEUstats matrix.
 * @param tgt_sents  list of list of hypotheses
 * @param ref_sents  list of list of references.
 */
void computeArray(MatrixBLEUstats& bleu,
      const vector< vector<string> >& tgt_sents,
      const vector< vector<string> >& ref_sents);

/**
 * Calculates the BLEUstats for and nbest list and its references.
 * Fills in the 1-D array bleu in accordance with the input requirement for
 * powell() (and linemax()).
 * @relates BLEUstats
 * @param bleu    returned BLEUstats values for the nbest.  It size will be min(max, nbest.size)
 * @param nbest   list of hypotheses for a source.
 * @param refs    the references for the set of nbest.
 * @param max     maximum number of hypotheses to calculate their BLEUstats.
 * @param smooth  smoothing type.
 */
void computeArrayRow(RowBLEUstats& bleu,
      const Nbest& nbest,
      const References& refs,
      Uint max = numeric_limits<Uint>::max(),
      int smooth = BLEUstats::DEFAULT_SMOOTHING);

/**
 * Calculates the BLEUstats for and nbest list and its references.
 * Fills in the 1-D array bleu in accordance with the input requirement for
 * powell() (and linemax()).
 * @relates BLEUstats
 * @param bleu       returned BLEUstats values for the nbest.  It size will be min(max, nbest.size)
 * @param tgt_sents  list of hypotheses for a source.
 * @param ref_sents  the references for the set of nbest.
 * @param max        maximum number of hypotheses to calculate their BLEUstats.
 * @param smooth     smoothing type.
 */
void computeArrayRow(RowBLEUstats& bleu,
      const vector<string>& tgt_sents,
      const vector<string>& ref_sents,
      Uint max = numeric_limits<Uint>::max(),
      int smooth = BLEUstats::DEFAULT_SMOOTHING);

/**
 * Writes to a stream a matrix of BLEUstats in a format that will be able to read.
 * @relates BLEUstats
 * @param out   output stream to write the matrix.
 * @param bleu  matrix to be written
 */
void writeBLEUArray(ostream &out, const MatrixBLEUstats& bleu);
/**
 * Reads a matrix from a stream in the format used by writeBLEUArray(ostream &out, const MatrixBLEUstats& bleu)
 * @relates BLEUstats
 * @param in    input stream from which to read the matrix.
 * @param bleu  returned BLEUstats matrix.
 */
void readBLEUArray(istream &in, MatrixBLEUstats& bleu);
}
#endif // BLEU_H
