/**
 * @author Aaron Tikuisis
 * @file bleu.cc  Implementation of BLEU stats.
 *
 * $Id$
 *
 * Evaluation Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

// TODO: redocument

#include "bleu.h"
#include "errors.h"
#include <voc.h>
#include <limits>

namespace Portage {
   // ugly code - make the code behaviour depend on an environment variable
   static const bool USE_NIST_STYLE_BLEU = getenv("PORTAGE_NIST_STYLE_BLEU");
   // discusting code - use side effects of static const initialization to show
   // a message on screen...
   static const ostream& dummy =
      (cerr << (USE_NIST_STYLE_BLEU
                 ? "Using NIST style brevity penalty in all BLEU calculations\n"
                 : ""));
}

using namespace Portage;

static const Uint MAX_BLEU_STAT_TYPE = numeric_limits<signed int>::max();

Uint BLEUstats::MAX_NGRAMS = 4;
void BLEUstats::setMaxNgrams(Uint n) {
   static bool bHasBeenInitialized(false);
   assert(bHasBeenInitialized == false);

   MAX_NGRAMS = n;
   bHasBeenInitialized = true;
}

Uint BLEUstats::MAX_NGRAMS_SCORE = 4;
void BLEUstats::setMaxNgramsScore(Uint n) {
   static bool bHasBeenInitialized(false);
   assert(bHasBeenInitialized == false);

   MAX_NGRAMS_SCORE = n;
   bHasBeenInitialized = true;
}

Uint BLEUstats::DEFAULT_SMOOTHING = DEFAULT_SMOOTHING_VALUE;
void BLEUstats::setDefaultSmoothing(const Uint n) {
   static bool bHasBeenInitialized(false);
   assert(bHasBeenInitialized == false);
   
   DEFAULT_SMOOTHING = n;
   bHasBeenInitialized = true;
}


/*
Initializes a new BLEUstats with values of 0, the stats for an empty set of translations.
*/
BLEUstats::BLEUstats()
: match(MAX_NGRAMS, 0)
, total(MAX_NGRAMS, 0)
, length(0)
, bmlength(0)
, smooth(DEFAULT_SMOOTHING)
{ }

/*
Initializes a new BLEUstats with values of 0, the stats for an empty set of
translations.  Set the smoothing type to sm
*/
BLEUstats::BLEUstats(int sm)
: match(MAX_NGRAMS, 0)
, total(MAX_NGRAMS, 0)
, length(0)
, bmlength(0)
, smooth(sm)
{ }

/*
Copy constructor
*/
BLEUstats::BLEUstats(const BLEUstats &s)
{
   *this = s;
}

bool nGramMatch(Uint n, vector<Uint>::const_iterator it1,
                        vector<Uint>::const_iterator it2)
{
   bool match = true;
   for (Uint i = 0; match && i < n; i++, it1++, it2++)
   {
      match = (*it1 == *it2);
   } // for
   return match;
} // nGramMatch

BLEUstats::BLEUstats(const string &tgt, const vector<string> &refs, int sm)
: match(MAX_NGRAMS, 0)
, total(MAX_NGRAMS, 0)
, length(0)
, bmlength(0)
, smooth(sm)
{
   typedef vector<string>::const_iterator IT;
   Tokens  tgt_words;
   split(tgt, tgt_words, " ");

   vector<Tokens>  refs_words;
   for (IT it(refs.begin()); it < refs.end(); ++it)
   {
      refs_words.push_back(Tokens());
      split(*it, refs_words.back(), " ");
   } // for
   init(tgt_words, refs_words, sm);
} // BLEUstats

/*
Initializes a new BLEUstats with values computed for the given translation
sentence.  tgt_words contains the words for the translated sentence being
scored.  refs_words is a vector of reference translations, each element
containing the words for a reference sentence.
*/
BLEUstats::BLEUstats(const Tokens &tgt_words, const vector<Tokens> &refs_words, int sm)
: match(MAX_NGRAMS, 0)
, total(MAX_NGRAMS, 0)
, length(0)
, bmlength(0)
, smooth(sm)
{
   init(tgt_words, refs_words, sm);
} // BLEUstats

BLEUstats::BLEUstats(const Sentence& trans, const References& refs, int sm)
: match(MAX_NGRAMS, 0)
, total(MAX_NGRAMS, 0)
, length(0)
, bmlength(0)
, smooth(sm)
{
   init(trans, refs, sm);
}

void BLEUstats::init(const Sentence& trans, const References& refs, int sm)
{
   typedef Tokens::const_iterator SIT;
   typedef References::const_iterator RIT;
   // Optimization: calculate the BLEU stats on Uints instead of strings
   Voc vocab;
   Voc::addConverter aConverter(vocab);

   // Tokenizing the translation
   vector<Uint> tgt_Uints;
   tgt_Uints.reserve(trans.size());
   split(trans.c_str(), tgt_Uints, aConverter);

   // Tokenizing the references
   vector< vector<Uint> > refs_Uints;
   tokenize(refs, vocab, refs_Uints);

   init(tgt_Uints, refs_Uints, sm);
}


void BLEUstats::init(const Tokens &tgt_words, const vector<Tokens> &refs_words, int sm)
{
   typedef Tokens::const_iterator TIT;
   typedef vector<Tokens>::const_iterator VIT;

   // Optimization: calculate the BLEU stats on Uints instead of strings
   Voc vocab;

   vector<Uint> tgt_Uints;
   tgt_Uints.reserve(tgt_words.size());
   for (TIT itTokens(tgt_words.begin()); itTokens != tgt_words.end(); ++itTokens) {
      tgt_Uints.push_back(vocab.add(itTokens->c_str()));
   }

   vector< vector<Uint> > refs_Uints;
   refs_Uints.reserve(refs_words.size());
   for (VIT itRefs(refs_words.begin()); itRefs != refs_words.end(); ++itRefs) {
      refs_Uints.push_back(vector<Uint>());
      refs_Uints.back().reserve(itRefs->size());
      for (TIT itTokens(itRefs->begin()); itTokens != itRefs->end(); ++itTokens) {
         refs_Uints.back().push_back(vocab.add(itTokens->c_str()));
      }
   }

   init(tgt_Uints, refs_Uints, sm);
}


void BLEUstats::init(const vector<Uint> &tgt_words, const vector< vector<Uint> > &refs_words, int sm)
{
   assert(tgt_words.size() <= MAX_BLEU_STAT_TYPE);
   length = tgt_words.size();
   for (Uint n = 0; n < MAX_NGRAMS; n++)
   {
      /*
       Initialize total number of n-grams and (temporary) detailed statistics
       (count, clipCount) for each n-gram.
       Since we have just one sentence, the total number of n-grams is
       precisely the total number of words - (n-1).
       When an n-gram appears more than once, it is necessary NOT to treat it
       the same as multiple different n-grams (eg. if target = "the the the"
       and reference = "the", this does not count as 3 matches).
       Thus, isDuplicated[i] is true iff the n-gram starting at the i-th word
       has already appeared in the target, starting BEFORE the i-th word.
       count[n-1][i] contains the total number of occurrences in the target of
       the n-gram starting at the i-th word, and clipCount[n-1][i] will hold
       the "clipped match count" for the same n-gram (both of these apply only
       when isDuplicated[n-1][i] are false).  The "clipped match count" for an
       n-gram is defined as:
       clippedCount = min(count, max_ref_count), where max_ref_count is the
       maximum number of occurrences of the n-gram in a reference sentence.
       eg. if the 2-gram "the car" appears twice in the target and once in each
       of the 2 reference sentences, then the clipped count for "the car" would
       be 1.
      */

      total[n] = (Uint)max((int)length - (int)n, 0);
      vector<bool> isDuplicated(total[n], false);
      vector<Uint> count(total[n], 1);
      vector<Uint> clipCount(total[n], 0);

      BLEU_STAT_TYPE i = 0;
      vector<Uint>::const_iterator it = tgt_words.begin();
      for (; i < total[n]; i++, it++)
      {
         // Determine whether this n-gram is duplicated
         BLEU_STAT_TYPE j = 0;
         vector<Uint>::const_iterator jt = tgt_words.begin();
         for (; j < i && !isDuplicated[i]; j++, jt++)
         {
            isDuplicated[i] = nGramMatch(n + 1, it, jt);
            if (isDuplicated[i])
            {
               // It's duplicated; increment the count on the original
               count[j]++;
            } // if
         } // for
      } // for

      vector<Uint>::const_iterator tit = tgt_words.begin();
      i = 0;
      for (; i < total[n]; i++, tit++)
      {
         for (vector< vector<Uint> >::const_iterator it = refs_words.begin();
               !isDuplicated[i] && clipCount[i] < count[i] && it <
               refs_words.end(); it++)
         {
            if (n <= it->size())
            {
               // Count the number of matches in this sentence for each n-gram
               // If the current clipCount is already equal to the count, we
               // can't do any better, so don't bother trying.
               Uint matches = 0;
               for (vector<Uint>::const_iterator rit = it->begin(); matches < count[i]
                     && rit < it->end() - n; rit++)
               {
                  if (nGramMatch(n + 1, tit, rit))
                  {
                     matches++;
                  } // if
               } // for
               clipCount[i] = max(clipCount[i], matches);
            } // if
         } // for
      } // for

      match[n] = 0;
      for (i = 0; i < total[n]; i++)
      {
         if (!isDuplicated[i])
         {
            match[n] += clipCount[i];
         } // if
      } // for
   } // for

   if ( USE_NIST_STYLE_BLEU ) {
      // NIST uses the shortest reference length for each sentence, to
      // calculate the brevity penalty, rather than the best match one.
      bmlength = refs_words.front().size();
      for (vector< vector<Uint> >::const_iterator it = refs_words.begin() + 1;
            it < refs_words.end(); it++)
         if ( int(it->size()) < bmlength )
            bmlength = it->size();
   } else {
      // best-match reference length is consistent with what IBM and Koehn do
      // (or at least did until Jan 2007, when I investigated the issue).
      Ulong curBMLength = refs_words.front().size();
      for (vector< vector<Uint> >::const_iterator it = refs_words.begin() + 1;
            it < refs_words.end(); it++)
      {
         // Determine if the length of this candidate is the new best length
         // Notice: length - m is preferred to length + m (for m > 0)
         if (abs((int)(it->size()) - (int)length) < abs((int)curBMLength - (int)length) ||
               (abs((int)(it->size()) - (int)length) == abs((int)curBMLength - (int)length) &&
                it->size() < curBMLength))
         {
            curBMLength = it->size();
         } // if
      } // for
      assert(curBMLength <= MAX_BLEU_STAT_TYPE);
      bmlength = curBMLength;
   }
   smooth = sm;
} // BLEUstats

/*
Computes the log BLEU score for these stats; that is:
\log BLEU = max(1 - bmlength / length) + \sum_{n=1}^{N} (1/N) \log(match_n / total_n)
The generalized BLEU score is actually given by
\log BLEU = max(1 - bmlength / length) + \sum_{n=1}^{N} w_n \log(match_n / total_n)
for some choice of weights (w_n).  Generally, the weights are 1/N, which is
what is used here.

Every statistic value (length, bmlength, match[n] and total[n] for n=0, .. ,
MAX_NGRAMS-1) must be strictly positive; currently this function does not
detect such errors and in such a case, the output is undefined.
EJJ 11AUG2005: Added asserts to make sure this condition is respected.
*/
double BLEUstats::score(Uint maxN, double epsilon) const
{
   assert(length > 0);
   assert(bmlength >= 0);
   assert(maxN >= 1);
   double result = min(1 - (double)bmlength / length, 0);

   // Make sure we are within the rang of the n-grams that were calculated.
   const Uint N(min(MAX_NGRAMS, maxN));

   if (smooth==0) // no smoothing
   {
      for (Uint n = 0; n < N; n++)
      {
         if (match[n] == 0)
         {
            result = 0;
            break;
         }
         else
         {
            assert(match[n] >= 0);
            assert(total[n] > 0);
            result += log((double)match[n] / total[n]) / N;
            // cerr << n+1 << "-gram match: " << match[n] << "/" << total[n] << " -> " << result << endl;
         }
      }
   }
   else if (smooth==1)  // replace 0 by epsilon
   {
      for (Uint n = 0; n < N; n++)
      {
         if (match[n] == 0)
         {
            result += log(epsilon) / N;
         }
         else
         {
            assert(match[n] >= 0);
            assert(total[n] > 0);
            result += log((double)match[n] / total[n]) / N;
            // cerr << n+1 << "-gram match: " << match[n] << "/" << total[n] << " -> " << result << endl;
         }
      }
   }
   else if (smooth==2) //Increase the count by 1 for all n-grams with n>1 (cf. Lin & Och, Coling 2004)
   {
      // 1-gram: count is not changed
      assert(total[0] > 0);
      if (match[0]>0)
      {
         result += log((double)match[0] / total[0]) / N;

         // higher n-grams: all counts and the total are increased by 1
         for (Uint n = 1; n < N; n++)
         {
            assert(match[n]==0 || total[n] > 0);
            result += log( ( (double)match[n]+1.0) / (total[n]+1.0) ) / N;
            // cerr << n+1 << "-gram Match: " << match[n]+1.0 << "/" << total[n]+1.0 << " -> " << result << endl;
         }
      }
      else
      {
         result += log(epsilon) / N;
      }
   }
   else 
      assert(false);

   return result;
}


/*
Outputs the statistics.
*/
void BLEUstats::output(ostream &out) const
{
   for (Uint n = 0; n < MAX_NGRAMS; n++)
   {
      out << (n+1) << "-gram (match/total) " << match[n] << "/" << total[n] << " " << ((double)match[n]/total[n]) << endl;
   }
   out << "Sentence length: " << length << "; Best-match sentence length: " << bmlength << endl;
   out << "Score: " << score() << endl;
}

/*
Write stats to output stream.
*/
void BLEUstats::write(ostream &out) const
{
   for (Uint n = 0; n < MAX_NGRAMS; n++)
   {
      out << match[n] << '\t' << total[n] << endl;
   }
   out << length << '\t' << bmlength << endl;
}

/*
Read stats from input stream; reciprocal to write.
*/
void BLEUstats::read(istream &in)
{
   for (Uint n = 0; n < MAX_NGRAMS; n++)
   {
      in >> match[n];
      in >> total[n];
   }
   in >> length;
   in >> bmlength;
}


BLEUstats& BLEUstats::operator-=(const BLEUstats& other)
{
   assert(smooth == other.smooth);

   for (Uint n(0); n < BLEUstats::MAX_NGRAMS; ++n)
   {
      match[n] -= other.match[n];
      total[n] -= other.total[n];
   }
   length   -= other.length;
   bmlength -= other.bmlength;
   return *this;
}

BLEUstats& BLEUstats::operator+=(const BLEUstats& other)
{
   assert(smooth == other.smooth);

   for (Uint n(0); n < BLEUstats::MAX_NGRAMS; ++n)
   {
      match[n] += other.match[n];
      total[n] += other.total[n];
   }
   length   += other.length;
   bmlength += other.bmlength;
   return *this;
}



namespace Portage
{
   /*
      Finds the difference in statistics between two BLEUstats objects.
    */
   BLEUstats operator-(const BLEUstats &s1, const BLEUstats &s2)
   {
      //assert (s1.length >= s2.length);
      //assert (s1.bmlength >= s2.bmlength);
      assert(s1.smooth==s2.smooth);

      BLEUstats result(s1.smooth);
      for (Uint n = 0; n < BLEUstats::MAX_NGRAMS; n++)
      {
         result.match[n] = s1.match[n] - s2.match[n];
         result.total[n] = s1.total[n] - s2.total[n];
      }
      result.length = s1.length - s2.length;
      result.bmlength = s1.bmlength - s2.bmlength;
      return result;
   }

   /*
      Adds together the statistics of two BLEUstats objects, returning the result.
    */
   BLEUstats operator+(const BLEUstats &s1, const BLEUstats &s2)
   {
      assert((Ulong)s1.length + (Ulong)s2.length <= MAX_BLEU_STAT_TYPE);
      assert((Ulong)s1.bmlength + (Ulong)s2.bmlength <= MAX_BLEU_STAT_TYPE);
      assert(s1.smooth==s2.smooth);

      BLEUstats result(s1.smooth);
      for (Uint n = 0; n < BLEUstats::MAX_NGRAMS; n++)
      {
         result.match[n] = s1.match[n] + s2.match[n];
         result.total[n] = s1.total[n] + s2.total[n];
      }
      result.length = s1.length + s2.length;
      result.bmlength = s1.bmlength + s2.bmlength;
      return result;
   }

   bool operator==(const BLEUstats &s1, const BLEUstats &s2)
   {
      bool result = s1.length == s2.length && s1.bmlength == s2.bmlength && s1.smooth == s2.smooth;
      for (Uint n = 0; n < BLEUstats::MAX_NGRAMS && result; n++)
      {
         result = s1.match[n] == s2.match[n] && s1.total[n] == s2.total[n];
      } // for
      return result;
   } // operator==

   bool operator!=(const BLEUstats &s1, const BLEUstats &s2)
   {
      return !(s1 == s2);
   } // operator!=


   ////////////////////////////////////////////////////////////////////////////////
   void computeBLEUArrayRow(RowBLEUstats& bleu,
         const Nbest& nbest,
         const References& refs,
         Uint max,
         int smooth)
   {
      const Uint K = min(max, nbest.size());
      bleu.resize(K);
      Voc voc;

      vector<vector<Uint> > nbest_uint;
      tokenize(nbest, voc, nbest_uint);

      vector<vector<Uint> > refs_uint;
      tokenize(refs, voc, refs_uint);

      int k;
#pragma omp parallel for private(k)
      for (k=0; k<(int)K; ++k) {
         bleu[k].init(nbest_uint[k], refs_uint, smooth);
      }
   } // computeBLEUArrayRow

   void computeBLEUArrayRow(RowBLEUstats& bleu,
         const vector<string>& tgt_sents,
         const vector<string>& ref_sents,
         Uint max,
         int smooth)
   {
      const Uint K = min(max, tgt_sents.size());
      bleu.resize(K);
      Voc voc;

      vector<vector<Uint> > nbest_uint;
      tokenize(tgt_sents, voc, nbest_uint);

      vector<vector<Uint> > refs_uint;
      tokenize(ref_sents, voc, refs_uint);

      int k;
#pragma omp parallel for private(k)
      for (k=0; k<(int)K; ++k) {
         bleu[k].init(nbest_uint[k], refs_uint, smooth);
      }
   } // computeBLEUArrayRow

   void computeBLEUArray(MatrixBLEUstats& bleu,
         const vector< vector<string> >& tgt_sents,
         const vector< vector<string> >& ref_sents)
   {
      assert(bleu.size() == tgt_sents.size());
      assert(bleu.size() == ref_sents.size());

      const Uint S(tgt_sents.size());
      for (Uint s(0); s<S; ++s) {
         computeBLEUArrayRow(bleu[s], tgt_sents[s], ref_sents[s]);
      }
   } // computeBLEUArray


   void writeBLEUArray(ostream &out, const MatrixBLEUstats& bleu)
   {
      out << bleu.size() << endl;
      for (Uint s = 0; s < bleu.size(); s++)
      {
         out << bleu[s].size() << endl;
         for (Uint k = 0; k < bleu[s].size(); k++)
         {
            bleu[s][k].write(out);
            out << endl;
         }
         out << endl;
      }
   }

   void readBLEUArray(istream &in, MatrixBLEUstats& bleu)
   {
      Uint S(0);
      in >> S;
      bleu.clear();
      bleu.resize(S);
      for (Uint s = 0; s < S; s++)
      {
         Uint K(0);
         in >> K;
         bleu[s].resize(K);
         for (Uint k = 0; k < K; k++)
         {
            bleu[s][k].read(in);
            if (in.eof())
            {
               error(ETFatal, "Unexpected end of file");
            }
         }
      }
   }
}
