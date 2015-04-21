/**
 * @author Samuel Larkin
 * @file eval/perSentenceStats.h
 * @brief A scoring metric wrap to perform on sentence level scoring.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */


#ifndef __PER_SENTENCE_STATS_H__
#define __PER_SENTENCE_STATS_H__

#include "basic_data_structure.h"

namespace Portage {

template <class ScoreMetric>
class perSentenceStats {
   public:
      double total;
      Uint   count;
   public:
      perSentenceStats()
      : total(0.0f)
      , count(0)
      {}

      perSentenceStats(const Translation& trans, const References& refs)
      : total(0.0f)
      , count(0)
      {
         init(trans, refs);
      }
      
      void init(const Translation& trans, const References& refs) {
         total = ScoreMetric::convertToDisplay(ScoreMetric(trans, refs).score());
         count = 1;
      }

      /// What is this metric's name.
      /// @return Returns this metric's name => Amber.
      static const char* const name() {
         static const string name(string("perSentenceStats<") + ScoreMetric::name() + ">");
         return name.c_str();
      }
      /**
       *
       * @param value internal value (eg, from score()) 
       * @return display value.
       */
      static double convertToDisplay(double value) {
	 return value;
      }
      /**
       *
       * @param value display value
       * @return internal value.
       */
      static double convertFromDisplay(double value) {
	 return value;
      }

      /**
       * Convert "internal" score value to pnorm format: in [0,1],
       * higher scores are better. Identical to convertToDisplay() for Amber,
       * but not for WER/PER!
       * @param value internal value (eg, from score()) 
       * @return pnorm value
       */
      static double convertToPnorm(double value) {
	 return convertToDisplay(value);
      }
      /**
       * Convert "internal" score value from pnorm format: in [0,1],
       * higher scores are better. Identical to convertFromDisplay() for Amber,
       * but not for WER/PER!
       * @param value pnorm value 
       * @return internal value
       */
      static double convertFromPnorm(double value) {
	 return convertFromDisplay(value);
      }


      double score() const { return (count > 0 ? total / count : 0.0f); }

      /**
       * Prints the ngram count and match length in a human readable format to out.
       * @param out  output stream defaults to cout.
       */
      void output(ostream &out = cout) const {
         out << "total: " << total << endl;
         out << "count: " << count << endl;
         out << "Score: " << score() << endl;
      }

      /**
       * Prints the ngrams count and match length so that it can be reread.
       * @param out  output stream mainly a file.
       */
      void write(ostream &out) const {
         out << total << "\t" << count << endl;
      }

      /**
       * Finds the difference in statistics between two BLEUstats objects.
       * @relates BLEUstats
       * @param other  right-hand side operand
       * @return Returns a BLEUstats containing this - other
       */
      perSentenceStats<ScoreMetric>& operator-=(const perSentenceStats<ScoreMetric>& other) {
         total -= other.total;
         count -= other.count;
         return *this;
      }
      /**
       * Adds together the statistics of two BLEUstats objects, returning
       * the result.
       * @relates BLEUstats
       * @param other  right-hand side operand
       * @return Returns a BLEUstats containing this + other
    */
      perSentenceStats<ScoreMetric>& operator+=(const perSentenceStats<ScoreMetric>& other) {
         total += other.total;
         count += other.count;
         return *this;
      }

      /// Callable entity for booststrap confidence interval. 
      struct CIcomputer
      {
         /// Define what is an iterator for a CIcomputer.
         typedef typename vector<perSentenceStats<ScoreMetric> >::const_iterator iterator;

         /**
          * Cumulates all BLEUstats from the range.
          * @param begin  start iterator
          * @param end    end iterator
          * @return Returns the Amber score [0 1] once the BLEU stats are all cumulated for the range.
          */
         double operator()(iterator begin, iterator end) {
            perSentenceStats<ScoreMetric> total;
            return std::accumulate(begin, end, total).score();
         }
      };
};

template <class ScoreMetric>
perSentenceStats<ScoreMetric> operator-(const perSentenceStats<ScoreMetric>& s1, const perSentenceStats<ScoreMetric>& s2) {
   perSentenceStats<ScoreMetric> result(s1);
   result -= s2;
   return result;
}

template <class ScoreMetric>
perSentenceStats<ScoreMetric> operator+(const perSentenceStats<ScoreMetric>& s1, const perSentenceStats<ScoreMetric>& s2) {
   perSentenceStats<ScoreMetric> result(s1);
   result += s2;
   return result;
}

template <class ScoreMetric>
bool operator==(const perSentenceStats<ScoreMetric>& s1, const perSentenceStats<ScoreMetric>& s2) {
   return s1.total == s2.total && s1.count == s2.count;
}

template <class ScoreMetric>
bool operator!=(const perSentenceStats<ScoreMetric>& s1, const perSentenceStats<ScoreMetric>& s2) {
   return !(s1 == s2);
}

/**
 * Scale ScoreMetric by a constant.
 */
template <class ScoreMetric>
perSentenceStats<ScoreMetric> operator*(perSentenceStats<ScoreMetric> &s, double c) {
   s.total *= c;
   return s;
}

template <class ScoreMetric>
void computeArrayRow(vector<perSentenceStats<ScoreMetric> >& scores,
                     const Nbest& nbest,
                     const References& refs,
                     Uint max)
{
   const Uint K = min(max, nbest.size());
   scores.resize(K);
   Voc voc;

   vector<vector<Uint> > nbest_uint;
   tokenize(nbest, voc, nbest_uint);

   vector<vector<Uint> > refs_uint;
   tokenize(refs, voc, refs_uint);

   int k;
#pragma omp parallel for private(k)
   for (k=0; k<(int)K; ++k) {
      scores[k].init(nbest_uint[k], refs_uint);
   }
}

} // ends namespace Portage

#endif  // __PER_SENTENCE_STATS_H__
