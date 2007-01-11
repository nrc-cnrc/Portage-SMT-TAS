/**
 * @author Aaron Tikuisis
 * @file bleu.h  BLEUstats which holds the ngrams count to calculate BLEU.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
*/

#ifndef BLEU_H
#define BLEU_H

#define MAX_NGRAMS 4

#include <portage_defs.h>
#include <basic_data_structure.h>
#include <iostream>
#include <string>
#include <vector>
#include <limits>

using namespace std;

namespace Portage
{
    // EJJ 11AUG05: the BLEU_STAT_TYPE has to be signed because BLEUstats
    // objects are sometimes used to store deltas, which can by definition be
    // negative.  However, it is illegal to compute a score when any of the
    // values are negative.
    //typedef Uint BLEU_STAT_TYPE;
    typedef signed int BLEU_STAT_TYPE;
    const Ulong MAX_BLEU_STAT_TYPE = (Ulong)1 << (sizeof(BLEU_STAT_TYPE) * 8) - 1;

    /// A class which maintains statistics for the BLEU score.
    class BLEUstats
    {
    public:
        BLEU_STAT_TYPE match[MAX_NGRAMS];  ///< ngrams match for n = [1 4]
        BLEU_STAT_TYPE total[MAX_NGRAMS];  ///< maximum ngram match possible for n = [1 4]
        BLEU_STAT_TYPE length;             ///< candidate translation length
        BLEU_STAT_TYPE bmlength;           ///< best match reference length
        int smooth;                        ///< smoothing method

    private:
        //@{
        /**
         * Calculates the ngram matches between the target and its references.
         * @param tgt_words   target sentence
         * @param refs_words  reference sentences.
         * @param sm          smoothing type
         */
        void init(const Tokens &tgt_words, const vector<Tokens>& refs_words, const int &sm);
        void init(const vector<Uint> &tgt_words, const vector< vector<Uint> >& refs_words, const int &sm);
        //@}

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
        BLEUstats(const int &sm);
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
        BLEUstats(const Sentence& trans, const References& refs, const int &sm);
        /**
         * Initializes a new BLEUstats with values computed for the given
         * translation sentence.
         * @param tgt   translation
         * @param refs  translation's references.
         * @param sm    smoothing type
         */
        BLEUstats(const string &tgt, const vector<string> &refs, const int &sm);
        /**
         * Initializes a new BLEUstats with values computed for the given
         * translation sentence.
         * @param tgt_words   contains the words for the translated sentence
         * being scored.
         * @param refs_words  is a vector of reference translations, each
         * element containing the words for a reference sentence.
         * @param sm          smoothing type
         */
        BLEUstats(const Tokens &tgt_words, const vector<Tokens>& refs_words, const int &sm);

        /**
         * Computes the log BLEU score for this stats.
         * BLEU score is calculated in the following maner:
         * \f$ \log BLEU = max(1 - \frac{bmlength}{length}) + \sum_{n=1}^{N} (\frac{1}{N}) \log(\frac{match_n}{total_n}) \f$
         * The generalized BLEU score is actually given by
         * \f$ \log BLEU = max(1 - \frac{bmlength}{length}) + \sum_{n=1}^{N} w_n \log(\frac{match_n}{total_n}) \f$
         * for some choice of weights (\f$ w_n \f$).  Generally, the weights are \f$ \frac{1}{N} \f$, which is
         * what is used here.
         *
         * @param epsilon  smoothing value used when there is no ngram match.
         * @return Returns
         */
        double score(double epsilon = 1e-30) const;
        
        /**
         * Prints the ngram count and match length in a humain readable format to out.
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
    void computeBLEUArray(MatrixBLEUstats& bleu,
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
    void computeBLEUArrayRow(RowBLEUstats& bleu,
                             const Nbest& nbest,
                             const References& refs,
                             const Uint max = numeric_limits<Uint>::max(),
                             const int &smooth = 1);

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
    void computeBLEUArrayRow(RowBLEUstats& bleu,
                             const vector<string>& tgt_sents,
                             const vector<string>& ref_sents,
                             const Uint max = numeric_limits<Uint>::max(),
                             const int &smooth = 1);

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
