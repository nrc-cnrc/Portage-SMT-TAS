/**
 * @author George Foster / Eric Joanis
 * @file ibm.h  IBM models 1 and 2.
 *
 *
 * COMMENTS:
 *
 * IBM models 1 and 2. See train_ibm.cc and run_ibm.cc for examples on how to
 * use this API.
 *
 * - To incorporate the null word in calculations, prepend nullWord() to
 *   src_toks in all functions that take a src_toks argument.
 * - During training, UNK target tokens are just skipped; UNK source tokens
 *   count only as placeholders for position probabilities.
 * - Use getTTable() (see ttable.h) for low-level access to IBM1 parameters.
 *
 * - IBM2 position parameters are normally p(spos|tpos,tlen,slen). This
 *   requires building a big and very sparse 4-dimensional table. To reduce
 *   space and produce smoother models, this implementation includes a backoff
 *   distribution p(spos/slen|tpos/tlen), where the ratios are discretized into
 *   a given number of bins. To train an IBM2, you specify a maximum slen and
 *   tlen (default 30,30), and the number backoff_size of discretizing bins
 *   (default 100). The backoff parameters are reestimated in parallel with the
 *   normal IBM2 parameters, and are used in place of the normal parameters for
 *   any sentence pair longer than slen,tlen (during both training and test).
 *
 * WARNING: This model is based on p(tgt|src), with tgt being the observed
 *          sequence and tgt_al = sequence of src positions being the hidden
 *          state sequence.  This is the opposite of the convention found in
 *          SOME OF the literature.
 *          So in Och+Ney 2003, as well as Brown et al 1993,
 *          f is tgt here, and e is src here.  And in hmm-alignment.tex, s is
 *          tgt here, and t is src here.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef IBM_H
#define IBM_H

#include "MagicStream.h"
#include "ttable.h"

namespace Portage {

  /// Interface for IBM alignments.
  class IBMAligner {
  public:

    /**
     * Calculate the Viterbi alignment for src/tgt.
     * Fill in each tgt_al slot with a src position (if unaligned: src.size()
     * if useImplicitNulls is true, 0 otherwise).
     * @param src hidden token sequence
     * @param tgt observed token sequence
     * @param[out] tgt_al for each tgt token, what src position it aligns to;
     * will have size() == tgt.size().
     * @param twist if true, consider src word order to be reversed when
     * assigning alignments.
     * @param[out] tgt_al_probs if not NULL, fill in vector with link
     * probabilities for alignment (probabilities of unaligned words are
     * undefined); also if not NULL, will have size() == tgt.size().
     */
    virtual void align(const vector<string>& src, const vector<string>& tgt,
                       vector<Uint>& tgt_al,
                       bool twist = false,
                       vector<double>* tgt_al_probs = NULL) = 0;

    /**
     * Virtual destructor needed for abstract class!
     */
    virtual ~IBMAligner() {}
  };


  /// Reads GIZA alignment from a file.
  class GizaAlignmentFile : public IBMAligner {
  private:
    iMagicStream in;   ///< Input stream containing the GIZA alignments.
  protected:
    Uint sent_count;   ///< Keeps count of the number of sentences.

    GizaAlignmentFile();

  public:
    /// Constructor.
    /// @param  filename file name containing the GIZA alignments.
    GizaAlignmentFile(string& filename);
    /// Destructor.
    virtual ~GizaAlignmentFile();

    /**
     * This is a read, really; src and tgt are used only to check
     * the validity of the alignment.
     * @copydoc IBMAligner::align()
     */
    virtual void align(const vector<string>& src, const vector<string>& tgt,
                       vector<Uint>& tgt_al,
                       bool twist = false,
                       vector<double>* tgt_al_probs = NULL);
  };


  /// Reads GIZA++-v2 alignment from a file.
  class Giza2AlignmentFile : public GizaAlignmentFile {
  protected:
    istream* p_in;   ///< Input stream containing the GIZA alignments.

  public:
    /// Constructor.
    /// @param  filename file name containing the GIZA alignments.
    Giza2AlignmentFile(string& filename);
    Giza2AlignmentFile(istream* in);
    /// Destructor.
    virtual ~Giza2AlignmentFile();

    /**
     * This is a read, really; src and tgt are used only to check
     * the validity of the alignment.
     * @copydoc IBMAligner::align()
     */
    virtual void align(const vector<string>& src, const vector<string>& tgt,
                       vector<Uint>& tgt_al,
                       bool twist = false,
                       vector<double>* tgt_al_probs = NULL);
  };


  /// IBM1 alignments from a ttable.
  class IBM1 : public IBMAligner {
  protected:

    TTable tt;

    vector<vector<float> > counts;
    double logprob;
    Uint num_toks;

    /**
     * is spos1 closer to tpos than spos2?
     */
    bool closer(Uint spos1, Uint spos2, Uint slen, Uint tpos, Uint tlen,
                bool twist = false);

    /**
     * Helper for count_symmetrized(), does the actual updating of the counts
     */
    void count_sym_helper(const vector<string>& src_toks,
                          const vector<string>& tgt_toks,
                          bool use_null,
                          const vector<vector<double> >& posteriors,
                          const vector<vector<double> >& r_posteriors);

  public:

    /**
     * Construct using an existing ttable file.
     * @param ttable_file  file containing the ttable.
     * @param src_voc   if not null, causes the ttable to be filtered while
     *                  loading, keeping only lines where the source word
     *                  exists in *src_voc.
     */
    IBM1(const string& ttable_file, const Voc* src_voc = NULL)
      : tt(ttable_file, src_voc) {
        useImplicitNulls = trainedWithNulls();
    }

    /**
     * Construct an empty model. Use add() to populate.
     */
    IBM1() { useImplicitNulls = false; }

    /// Destructor.
    virtual ~IBM1() {}

    /// Writes the ttable to a file.
    /// @param ttable_file  file name to output ttable
    /// @param bin_ttable   write the ttable in binary format.
    virtual void write(const string& ttable_file,
                       bool bin_ttable = false) const;

    /// Writes the current counts to a file.
    /// @param count_file   file name to output the counts
    virtual void writeBinCounts(const string& count_file) const;

    /// Reads counts from a file, adding them to the current counts
    /// @param count_file   file name to read counts from
    virtual void readAddBinCounts(const string& count_file);

    /// Simply returns the string "NULL"
    /// @return Returns the string "NULL"
    static char const * const nullWord() {return "NULL";}

    /// Verifies if the ttable contains the nullWord().
    /// @return Returns if the ttable contains the nullWord().
    bool trainedWithNulls() {
        return tt.sourceIndex((string)nullWord()) != tt.numSourceWords();
    }

    /**
     * If this variable is set to false, pr(), logpr(), and align()
     * expect an explicit prepended NULL word; otherwise, they take
     * NULLs into account implicitly (ie, without a prepended NULL
     * token). For align(), useImplicitNulls==true is equivalent to
     * prepending a NULL token, aligning, then removing the NULL
     * token, setting all 0 indices to src_toks.size(), and
     * subtracting 1 from all other indices. By default this is true;
     * it should be set to false if the model was **not** trained with
     * NULL's (not something you want to do anyway).
     * NB: this affects IBM2 too.
     */
    bool useImplicitNulls;

    /**
     * Add all word pairs in given sequences to the model. compile() MUST be
     * called when add()'s are finished.
     * @param src_toks hidden token sequence; not const because it might be
     *        temporarily modified during the execution of this method, but if
     *        so its value is restored before the method returns
     * @param tgt_toks observed token sequence
     * @param use_null if true, nullWord() is implicitely added to src_toks
     */
    virtual void add(vector<string>& src_toks,
                     const vector<string>& tgt_toks,
                     bool use_null);

    /// Makes the ttable distributions uniform.
    virtual void compile() {tt.makeDistnsUniform();}

    /**
     * Training functions. Call initCounts(), then count() for each segment
     * pair, then estimate() for one complete EM cycle.
     */
    virtual void initCounts();

    /**
     * Count the expected alignment occurrences in one sentence pair.
     * @param src_toks  hidden token sequence
     * @param tgt_toks  observed token sequence
     * @param use_null  if true, or if useImplicitNulls is true, nullWord() is
     *                  implicitly inserted at the beginning of src_toks
     */
    virtual void count(const vector<string>& src_toks,
                       const vector<string>& tgt_toks, bool use_null);

    /**
     * Count the expected alingment occurrences in one sentence pair,
     * symmetrizing the counts using the joint objective function of Liang,
     * Taskar and Klein, HLT-2006.
     *
     * Note: in count(), the evidence from all possible alignments is tallied,
     * so that a sentence pair always provides a evidence summing to 1,
     * distributed among the possible alignments.  With the Liang et al
     * symmetrization, the posterior probability of a link according to one
     * model (normalized within the model) is multiplied by the posterior prob
     * of the same link according to the other model (normalized within that
     * model), so that the result is no longer normalized.  As Liang et al
     * state, intuitively, this means a link is discounted when the models
     * disagree.  As a further consequence, a sentence pair where the models
     * disagree a lot will effectively be discounted as a whole, since many of
     * its links will be themselves discounted.
     *
     * @param src_toks hidden token sequence
     * @param tgt_toks observed token sequence
     * @param use_null neither src_toks nor tgt_toks should include the NULL
     *    token; instead set either this parameters or useImplicitNulls to true
     *    to train with nulls.
     * @param reverse_model model for the reverse direction - reverse_model
     *    will be used to calculate the counts for *this, and *this will also
     *    be used to update the counts in reverse_model.  Note that calling
     *    modelA->count_symmetrized(src,tgt,modelB) has exactly the same effect
     *    as calling modelB->count_symmetrized(tgt,src,modelA) and only one of
     *    these two calls should be used.
     */
    virtual void count_symmetrized(const vector<string>& src_toks,
                                   const vector<string>& tgt_toks,
                                   bool use_null, IBM1* reverse_model);

    /**
     * Estimate new parameters from counts.
     * @param pruning_threshold remove all probabilities less than this
     * @param null_pruning_threshold remove NULL alignments probs less than this
     * @return "<prev-ppx,new-size>"
     */
    virtual pair<double,Uint> estimate(double pruning_threshold,
                                       double null_pruning_threshold);

    /**
     * Calculate p(tgt_tok|src_toks), i.e., the probability of the single word
     * tgt_tok in position tpos given the source sentence src_toks.
     * @param src_toks hidden token sequence
     * @param tgt_tok  individual observed token
     * @param tpos     0-based position of tgt_tok within tgt sentence (used by
     *                 IBM2 only)
     * @param tlen     length of tgt sentence (used by IBM2 only)
     * @param probs    if not null, (*probs)[i] is set to
     *                 p(tgt_tok|src_toks[i]), for all i. If useImplicitNulls
     *                 is set, a null token is considered to be implicitly
     *                 prepended to src_toks, ie probs[0] contains
     *                 p(tgt_tok|NULL), probs[1] contains
     *                 p(tgt_tok|src_toks[0]), etc.
     * @return p(tgt_tok|src_toks).
     */
    virtual double pr(const vector<string>& src_toks, const string& tgt_tok,
                      Uint tpos, Uint tlen, vector<double>* probs = NULL) {
       return IBM1::pr(src_toks, tgt_tok, probs);
    }

    /**
     * Calculate p(tok|src_seg)
     * See double pr(const vector<string>& src_toks, const string& tgt_tok,
     * Uint tpos, Uint tlen, vector<double>* probs)
     */
    virtual double pr(const vector<string>& src_toks, const string& tgt_tok,
                      vector<double>* probs = NULL);

    /**
     * Calculate log(p(tgt_seg|src_seg)).
     * @param src_toks hidden token sequence
     * @param tgt_toks observed token sequence
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    virtual double logpr(const vector<string>& src_toks,
                         const vector<string>& tgt_toks,
                         double smooth = 1e-07);

    /**
     * max_{s in src_seq} log(p(tgt_tok|s)).
     * @param src_toks hidden token sequence
     * @param tgt_tok  observed token sequence
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    virtual double minlogpr(const vector<string>& src_toks,
                            const string& tgt_tok,
                            double smooth = 1e-07);

    /**
     * max_{s in src_seq} log(p(s|tgt_tok)).
     * @param src_toks hidden token sequence
     * @param tgt_toks observed token sequence
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    virtual double minlogpr(const string& src_toks,
                            const vector<string>& tgt_toks,
                            double smooth = 1e-07);

    virtual void align(const vector<string>& src, const vector<string>& tgt,
                       vector<Uint>& tgt_al, bool twist = false,
                       vector<double>* tgt_al_probs = NULL);

    /**
     * Calculate the posterior alignment probability for each possible link.
     * @param src hidden token sequence
     * @param tgt observed token sequence
     * @param[out] posteriors will have size
     *                  tgt.size() x (src.size() + useImplicitNulls ? 1 : 0)
     *             posteriors[j][i] will contain the probability of
     *             aligning tgt[j] to src[i],
     *             posteriors[j][useImplicitNulls ? src.size() : 0]
     *             will contain the probability of aligning tgt[j] to NULL.
     *             Note that for a 1-many alignment model, this always holds:
     *                 sum_i(posteriors[j][i]) = 1.0 for all j.
     * @return log(p(tgt|src)), the log of the global prob summed over all
     *         alignments - note: words causing p(tgt|src) to be 0.0 will be
     *         ignored in this calculation, and no smoothing is used, so the
     *         value returned will over-estimate p(tgt|src) in such cases.
     *         use logpr() for more accurate smoothed values.
     */
    virtual double linkPosteriors(const vector<string>& src,
                                  const vector<string>& tgt,
                                  vector<vector<double> >& posteriors);

    /// Get the ttable.
    /// @return Returns the ttable.
    TTable& getTTable() {return tt;}

    /// test writeBinCounts() and readAddBinCounts()
    /// @param count_file Temp file where to write the counts to
    virtual void testReadWriteBinCounts(const string& count_file) const;
  };

  /*-------------------------------------------------------------------------
    IBM2
    -------------------------------------------------------------------------*/

  /// IBM2 model
  class IBM2 : public IBM1 {
    Uint max_slen;
    Uint max_tlen;
    Uint backoff_size;

    Uint sblock_size;
    Uint npos_params;           ///< size of probs and counts arrays

    float* pos_probs;           ///< main p(spos|tpos,tlen,slen) array
    float* pos_counts;

    float *backoff_probs;       ///< p(spos/slen|tpos/tlen) backoff array
    float *backoff_counts;

    vector<float> backoff_distn;

    void createProbTables();
    void initProbTables();

    /**
     * Location of the distribution p(spos|tpos,tlen,slen), containing slen
     * elems, in pos_probs.
     * @return Returns the starting index of p(spos|tpos,tlen,slen).
     */
    Uint posOffset(Uint tpos, Uint tlen, Uint slen) const {
      return (tpos + tlen * (tlen-1) / 2) * sblock_size + slen * (slen-1) / 2;
    }

    /**
     * Get the distribution p(spos|tpos,tlen,slen), containing slen elems.
     */
    float* posDistn(Uint tpos, Uint tlen, Uint slen) {
      return tlen <= max_tlen && slen <= max_slen
         ? pos_probs + posOffset(tpos, tlen, slen)
         : getBackoffDistn(tpos, tlen, slen);
    }

    /**
     * Gets a backoff distribution for given conditioning variables.
     * @param tpos
     * @param tlen
     * @param slen
     * @return Returns the backoff distribution
     */
    float* getBackoffDistn(Uint tpos, Uint tlen, Uint slen);

    /**
     *
     * @param spos
     * @param slen
     * @return Returns
     */
    Uint backoffSrcOffset(Uint spos, Uint slen) {
      return spos == 0 ? 0 : (backoff_size-1) * spos / slen + 1;
    }

    void count_sym_helper(const vector<string>& src_toks,
                          const vector<string>& tgt_toks,
                          bool use_null,
                          const vector<vector<double> >& posteriors,
                          const vector<vector<double> >& r_posteriors);

    /// Assignment is not allowed
    IBM2& operator=(const IBM2&);

    /**
     * Read an existing position parameters file.
     * For use by constructors only.
     * @param dist_file  file containing the distributional parameters
     */
    void read(const string& dist_file);

  public:

    // Avoid hiding base class overriden functions.
    using IBM1::pr;
    using IBM1::minlogpr;

    /**
     * Get the .pos filename from the base ttable filename.
     * @param ttable_name
     * @return ttable_name with .pos added, before a .gz extension if any
     */
    static const string posParamFileName(const string& ttable_name) {
      return addExtension(ttable_name, ".pos");
    }

    /**
     * Default constructor.
     */
    IBM2();

    /**
     * Construct an empty model; use add() to populate. The parameters specify
     * the size of the standard IBM2 position table, and the size of the
     * backoff table, respectively. See initial comments for interpretation of
     * these.
     * @param max_slen
     * @param max_tlen
     * @param backoff_size
     */
    IBM2(Uint max_slen = 50, Uint max_tlen = 50, Uint backoff_size = 50);

    /**
     * Construct using an existing ttable file, but an empty IBM2 position
     * table, with given dimensions. The "<placeholder>" param is ignored.
     * @param ttable_file  file containing the ttable.
     * @param placeholder
     * @param max_slen
     * @param max_tlen
     * @param backoff_size
     */
    IBM2(const string& ttable_file, Uint placeholder,
         Uint max_slen = 50, Uint max_tlen = 50, Uint backoff_size = 50);

    /**
     * Construct using an existing ttable file and position parameters file.
     * The position params file must be called posParamFileName(ttable_file).
     * @param ttable_file  file containing the ttable.
     */
    IBM2(const string& ttable_file);

    /**
     * Construct using an existing .dist file but no TTable file.
     * This constructor exists only to support ibmcat.cc.
     * @param bogus  NULL pointer to disambiguate with the previous constructor
     * @param pos_file  file containing the positional parameters
     */
    IBM2(void* bogus, const string& pos_file);

    /// Copy constructor (slow, intended for testing purposes only)
    /// Deep copy of the TTable and IBM1 counts, but not of IBM2 positional
    /// probs or counts.
    IBM2(const IBM2&);

    /// Destructor.
    virtual ~IBM2();

    /**
     * Write IBM1 params to ttable_file and IBM2 params to
     * posParamFileName(ttable_file).
     * @param ttable_file  file name to output the IBM1 and IBM2 values.
     * @param bin_ttable   write the ttable part in binary format.
     */
    virtual void write(const string& ttable_file,
                       bool bin_ttable = false) const;

    /// Writes the current counts to a file.
    /// @param count_file   file name to output the IBM1 counts, IBM2 counts
    ///                     will go to count_file.ibm2
    virtual void writeBinCounts(const string& count_file) const;

    /// Reads counts from a file, adding them to the current counts
    /// @param count_file   file name to read the IBM1 counts from, IBM2 counts
    ///                     will go be read from count_file.ibm2
    virtual void readAddBinCounts(const string& count_file);

    virtual void initCounts();

    virtual void count(const vector<string>& src_toks,
                       const vector<string>& tgt_toks, bool use_null);

    /**
     * @copydoc IBM1::count_symmetrized()
     * @pre reverse_model must be an IBM2*
     */
    virtual void count_symmetrized(const vector<string>& src_toks,
                                   const vector<string>& tgt_toks,
                                   bool use_null, IBM1* reverse_model);

    virtual pair<double,Uint> estimate(double pruning_threshold,
                                       double null_pruning_threshold);

    virtual double pr(const vector<string>& src_toks, const string& tgt_tok,
                      Uint tpos, Uint tlen, vector<double>* probs = NULL);

    /**
     * Not implemented for IBM2 models; missing required parameters.
     * Causes fatal error if called, to avoid calling it by mistake.
     */
    virtual double pr(const vector<string>& src_toks, const string& tgt_tok,
                      vector<double>* probs = NULL);

    /**
     * p(tgt_seg|src_seg).
     * @param src_toks
     * @param tgt_toks
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    virtual double logpr(const vector<string>& src_toks,
                         const vector<string>& tgt_toks,
                         double smooth = 1e-07);

    /**
     * max_{s in src_seq} p(tgt_toks[i]|s)
     * (or max_{t in tgt_seq} p(t|src_toks[i]) if inv is true).
     * @param src_toks
     * @param tgt_toks
     * @param i
     * @param inv is true if calculating minlogpr of src_toks[i] over tgt_toks
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    virtual double minlogpr(const vector<string>& src_toks,
                            const vector<string>& tgt_toks,
                            const int i, bool inv=false, double smooth = 1e-07);

    virtual void align(const vector<string>& src, const vector<string>& tgt,
                       vector<Uint>& tgt_al, bool twist = false,
                       vector<double>* tgt_al_probs = NULL);

    virtual double linkPosteriors(const vector<string>& src,
                                  const vector<string>& tgt,
                                  vector<vector<double> >& posteriors);

    /// test writeBinCounts() and readAddBinCounts()
    /// @param count_file Temp file where to write the counts to
    virtual void testReadWriteBinCounts(const string& count_file) const;

  }; // IBM2

}

#endif
