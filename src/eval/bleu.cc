/**
 * @author Aaron Tikuisis
 * @file bleu.cc  Implementation of BLEU stats.
 *
 * $Id$
 *
 * Evaluation Module
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

// TODO: redocument

#include <bleu.h>
#include <errors.h>
#include <assert.h>
#include <str_utils.h>
#include <cmath>
#include <voc.h>
#include <string>
#include <vector>
#include <iostream>

using namespace Portage;

/*
Initializes a new BLEUstats with values of 0, the stats for an empty set of translations.
*/
BLEUstats::BLEUstats()
: length(0)
, bmlength(0)
, smooth(1)
{
    for (int n = 0; n < MAX_NGRAMS; n++)
    {
        match[n] = 0;
        total[n] = 0;
    }
}

/*
Initializes a new BLEUstats with values of 0, the stats for an empty set of translations.
Set the smoothing type to sm
*/
BLEUstats::BLEUstats(const int &sm)
: length(0)
, bmlength(0)
, smooth(sm)
{
    for (int n = 0; n < MAX_NGRAMS; n++)
    {
        match[n] = 0;
        total[n] = 0;
    }
}

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

BLEUstats::BLEUstats(const string &tgt, const vector<string> &refs, const int &sm)
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
Initializes a new BLEUstats with values computed for the given translation sentence.
tgt_words contains the words for the translated sentence being scored.  refs_words is a
vector of reference translations, each element containing the words for a reference
sentence.
*/
BLEUstats::BLEUstats(const Tokens &tgt_words, const vector<Tokens> &refs_words, const int &sm)
{
  init(tgt_words, refs_words, sm);
} // BLEUstats

BLEUstats::BLEUstats(const Sentence& trans, const References& refs, const int &sm)
{
    typedef Tokens::const_iterator TIT;
    typedef References::const_iterator RIT;
    // Optimization: calculate the BLEU stats on Uints instead of strings
    Voc vocab;
    vector<Uint> tgt_Uints;
    const Tokens& transTokens(trans.getTokens());
    tgt_Uints.reserve(transTokens.size());
    for (TIT itTokens(transTokens.begin()); itTokens != transTokens.end(); ++itTokens) {
        tgt_Uints.push_back(vocab.add(itTokens->c_str()));
    }

    vector< vector<Uint> > refs_Uints;
    refs_Uints.reserve(refs.size());
    for (RIT itRefs(refs.begin()); itRefs != refs.end(); ++itRefs) {
        refs_Uints.push_back(vector<Uint>());
        refs_Uints.back().reserve(itRefs->size());
        const Tokens& refTokens(itRefs->getTokens());
        for (TIT itTokens(refTokens.begin()); itTokens != refTokens.end(); ++itTokens) {
            refs_Uints.back().push_back(vocab.add(itTokens->c_str()));
        }
    }

    init(tgt_Uints, refs_Uints, sm);
}


void BLEUstats::init(const Tokens &tgt_words, const vector<Tokens> &refs_words, const int &sm)
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


void BLEUstats::init(const vector<Uint> &tgt_words, const vector< vector<Uint> > &refs_words, const int &sm)
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
double BLEUstats::score(double epsilon) const
{
    assert(length > 0);
    assert(bmlength >= 0);
    double result = min(1 - (double)bmlength / length, 0);
    if (smooth==1) 
      {
        for (int n = 0; n < MAX_NGRAMS; n++)
          {
            if (match[n] == 0)
              {
                result += log(epsilon) / MAX_NGRAMS;
              }
            else
              {
                assert(match[n] >= 0);
                assert(total[n] > 0);
                result += log((double)match[n] / total[n]) / MAX_NGRAMS;
                // cerr << n+1 << "-gram match: " << match[n] << "/" << total[n] << " -> " << result << endl;
              }
          }
      }
    else
      {
        assert(smooth==2);
        
        // 1-gram: count is not changed
        assert(total[0] > 0);
        if (match[0]>0) 
          {
            result += log((double)match[0] / total[0]) / MAX_NGRAMS;
            
            // higher n-grams: all counts and the total are increased by 1
            for (int n = 1; n < MAX_NGRAMS; n++)
              {
                assert(match[n]==0 || total[n] > 0);
                result += log( ( (double)match[n]+1.0) / (total[n]+1.0) ) / MAX_NGRAMS;
                // cerr << n+1 << "-gram Match: " << match[n]+1.0 << "/" << total[n]+1.0 << " -> " << result << endl;
              }
          }
        else
          {
            result += log(epsilon) / MAX_NGRAMS;
          }
      }
    
    return result;
}


/*
Outputs the statistics.
*/
void BLEUstats::output(ostream &out) const
{
    for (int n = 0; n < MAX_NGRAMS; n++)
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
    for (int n = 0; n < MAX_NGRAMS; n++)
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
    for (int n = 0; n < MAX_NGRAMS; n++)
    {
        in >> match[n];
        in >> total[n];
    }
    in >> length;
    in >> bmlength;
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
        for (int n = 0; n < MAX_NGRAMS; n++)
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
       for (int n = 0; n < MAX_NGRAMS; n++)
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
        for (Uint n = 0; n < MAX_NGRAMS && result; n++)
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
                             const Uint max,
                             const int &smooth)
    {
        const Uint K = min(max, nbest.size());
        bleu.reserve(K);
        for (Uint k(0); k < K; ++k)
        {
          bleu.push_back(BLEUstats(nbest[k], refs, smooth));
        }
    } // computeBLEUArrayRow

    void computeBLEUArrayRow(RowBLEUstats& bleu,
                             const vector<string>& tgt_sents,
                             const vector<string>& ref_sents,
                             const Uint max,
                             const int &smooth)
    {
        const Uint K = min(max, tgt_sents.size());
        bleu.reserve(K);
        for (Uint k(0); k < K; ++k)
        {
            bleu.push_back(BLEUstats(tgt_sents[k], ref_sents, smooth));
        }
    } // computeBLEUArrayRow

    void computeBLEUArray(MatrixBLEUstats& bleu,
                          const vector< vector<string> >& tgt_sents,
                          const vector< vector<string> >& ref_sents)
    {
        assert(bleu.size() == tgt_sents.size());
        assert(bleu.size() == ref_sents.size());
        
        const Uint S(tgt_sents.size());
        for (Uint s(0); s<S; ++s)
        {
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
