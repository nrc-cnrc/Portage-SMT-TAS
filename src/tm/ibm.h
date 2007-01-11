/**
 * @author George Foster
 * @file ibm.h  IBM models 1 and 2.
 * 
 * 
 * COMMENTS:
 *
 * IBM models 1 and 2. See train_ibm.cc and run_ibm.cc for examples on how to
 * use this API.
 *
 * - To incorporate the null word in calculations, prepend nullWord() to src_toks
 *   in all functions that take a src_toks argument. 
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
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
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
     * Fill in each tgt_al slot with a src position (src.size() if unaligned)
     * @param src
     * @param tgt
     * @param tgt_al
     * @param twist if true, consider src word order to be reversed when
     * assigning alignments.
     * @param tgt_al_probs if not NULL, fill in vector with link probabilities
     * for alignment (probabilities of unaligned words are undefined).
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
    Uint sent_count;   ///< Keeps count of the number of sentences.

  public:
    /// Constructor.
    /// @param  filename file name containing the GIZA alignments.
    GizaAlignmentFile(string& filename);
    /// Destructor.
    ~GizaAlignmentFile();

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
     * Needs description
     * @param spos1  
     * @param spos2  
     * @param slen  
     * @param tpos  
     * @param tlen  
     * @param twist  
     * @return 
     */
    bool closer(Uint spos1, Uint spos2, Uint slen, Uint tpos, Uint tlen, bool twist = false);
   
  public:

    /**
     * Construct using an existing ttable file.
     * @param ttable_file  file containing the ttable.
     */
    IBM1(const string& ttable_file) : tt(ttable_file) {useImplicitNulls = false;}

    /**
     * Construct an empty model. Use add() to populate.
     */
    IBM1() {useImplicitNulls = false;}

    /// Destructor.
    virtual ~IBM1() {}

    /// Writes the ttable to a file.
    /// @param ttable_file  file name to output ttable
    void write(const string& ttable_file) const;

    /// Simply returns the string "NULL"
    /// @return Returns the string "NULL"
    static char const * const nullWord() {return "NULL";}
   
    /// Verifies if the ttable contains the nullWord().
    /// @return Returns if the ttable contains the nullWord().
    bool trainedWithNulls() {return tt.sourceIndex((string)nullWord()) != tt.numSourceWords();}

    /**
     * Set this variable to true to have pr(), logpr(), and align() take NULLs
     * into account implicitly (ie, without a prepended NULL token). For align(),
     * this is equivalent to prepending a NULL token, aligning, then removing
     * the NULL token, setting all 0 indices to src_toks.size(), and subtracting
     * 1 from all other indices. By default this is false; it should not be set
     * to true unless the model was trained with NULL's. NB: this affects IBM2
     * too.
     */
    bool useImplicitNulls;

    /**
     * Add all word pairs in given sequences to the model. compile() MUST be
     * called when add()'s are finished.
     * @param src_toks
     * @param tgt_toks
     */
    void add(const vector<string>& src_toks, const vector<string>& tgt_toks);

    /// Makes the ttable distances uniform.
    void compile() {tt.makeDistnsUniform();}

    /**
     * Training functions. Call initCounts(), then count() for each segment
     * pair, then estimate() for one complete EM cycle.
     */
    void initCounts();

    /**
     * Needs description
     * @param src_toks
     * @param tgt_toks
     */
    void count(const vector<string>& src_toks, const vector<string>& tgt_toks);

    /** 
     * Estimate new parameters from counts.
     * @param pruning_threshold remove all probabilities less than this
     * @return "<prev-ppx,new-size>"
     */
    pair<double,Uint> estimate(double pruning_threshold = 0.0);
   
    /**
     * Virtual p(tgt-tok|src_seg).
     * @param src_toks
     * @param tgt_tok
     * @param tpos 0-based position of tgt-tok within tgt sentence (IBM2 only)
     * @param tlen length of tgt sentence (IBM2 only)
     * @param probs if not null, (*probs)[i] is set to p(tgt_tok|src_toks[i]),
     * for all i. If useImplicitNulls is set, a null token is considered to be
     * implicitly prepended to src_toks, ie probs[0] contains p(tgt_tok|NULL),
     * probs[1] contains p(tgt_tok|src_toks[0]), etc.
     * @return Returns p(tgt-tok|src_seg).
     */
    virtual double pr(const vector<string>& src_toks, const string& tgt_tok, 
                      Uint tpos, Uint tlen,
                      vector<double>* probs = NULL) {
      return pr(src_toks, tgt_tok, probs);
    }
   
    /**
     * p(tok|src_seg) 
     * See double pr(const vector<string>& src_toks, const string& tgt_tok, Uint tpos, Uint tlen, vector<double>* probs)
     */
    double pr(const vector<string>& src_toks, const string& tgt_tok, 
              vector<double>* probs = NULL);

    /**
     * p(tgt_seg|src_seg).
     * @param src_toks
     * @param tgt_toks
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    virtual double logpr(const vector<string>& src_toks, const vector<string>& tgt_toks,
                         double smooth = 1e-50);

    virtual void align(const vector<string>& src, const vector<string>& tgt, 
                       vector<Uint>& tgt_al, bool twist = false, 
                       vector<double>* tgt_al_probs = NULL);

    /// Get the ttable.
    /// @return Returns the ttable.
    TTable& getTTable() {return tt;}
  };

  /*--------------------------------------------------------------------------------
    IBM2
    --------------------------------------------------------------------------------*/

  /// IBM2 model
  class IBM2 : public IBM1 {
    Uint max_slen;
    Uint max_tlen;
    Uint backoff_size;

    Uint sblock_size;
    Uint npos_params;		///< size of probs and counts arrays
   
    float* pos_probs;		///< main p(spos|tpos,tlen,slen) array
    float* pos_counts;
   
    float *backoff_probs;	///< p(spos/slen|tpos/tlen) backoff array
    float *backoff_counts;

    vector<float> backoff_distn;

    void createProbTables();
    void initProbTables();

    /**
     * Location of the distribution p(spos|tpos,tlen,slen), containing slen elems.
     * @param tpos
     * @param tlen
     * @param slen
     * @return Returns the index of p(spos|tpos,tlen,slen).
     */
    Uint posOffset(Uint tpos, Uint tlen, Uint slen) const {
      return (tpos + tlen * (tlen-1) / 2) * sblock_size + slen * (slen-1) / 2;
    }

    /**
     * Location of the distribution p(spos|tpos,tlen,slen), containing slen elems.
     * @param tpos
     * @param tlen
     * @param slen
     * @return Returns the index of p(spos|tpos,tlen,slen).
     */
    float* posDistn(Uint tpos, Uint tlen, Uint slen) {
      return tlen <= max_tlen && slen <= max_slen ?
        pos_probs + posOffset(tpos, tlen, slen) : getBackoffDistn(tpos, tlen, slen);
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

  public:

    /**
     * Get the .pos filename from the base ttable filename.
     * @param ttable_name  
     * @return 
     */
    static const string posParamFileName(const string& ttable_name);

    /**
     * Construct an empty model; use add() to populate. The parameters specify
     * the size of the standard IBM2 position table, and the size of the backoff
     * table, respectively. See initial comments for interpretation of these.
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
     * Construct using an existing ttable file and position parameters file. The
     * position params file must be called posParamFileName(ttable_file).
     * @param ttable_file  file containing the ttable.
     */
    IBM2(const string& ttable_file);

    /// Destructor.
    virtual ~IBM2() {
      if (pos_probs) delete[] pos_probs;
      if (pos_counts) delete[] pos_counts;
      if (backoff_probs) delete[] backoff_probs;
      if (backoff_counts) delete[] backoff_counts;
    }

    /**
     * Write IBM1 params to ttable_file and IBM2 params to
     * posParamFileName(ttable_file).
     * @param ttable_file  file name to output the IBM1 and IBM2 values.
     */
    void write(const string& ttable_file) const;

    /**
     * Add all word pairs in given sequences to the model. compile() MUST be
     * called when add()'s are finished.
     * @param src_toks
     * @param tgt_toks
     */
    void add(const vector<string>& src_toks, const vector<string>& tgt_toks) {
      IBM1::add(src_toks, tgt_toks);
    }

    /// Makes the ttable distances uniform.
    void compile() {IBM1::compile();}

    /**
     * Training functions. Call initCounts(), then count() for each segment
     * pair, then estimate() for one complete EM cycle.
     */
    void initCounts();

    /**
     * Needs description
     * @param src_toks
     * @param tgt_toks
     */
    void count(const vector<string>& src_toks, const vector<string>& tgt_toks);

    /** 
     * Estimate new parameters from counts.
     * @param pruning_threshold remove all probabilities less than this
     * @return "<prev-ppx,new-size>"
     */
    pair<double,Uint> estimate(double pruning_threshold = 0.0);

    /**
     * p(tok|src_seg) 
     * See double pr(const vector<string>& src_toks, const string& tgt_tok, Uint tpos, Uint tlen, vector<double>* probs)
     */
    double pr(const vector<string>& src_toks, const string& tgt_tok, 
              Uint tpos, Uint tlen, vector<double>* probs = NULL);
   
    /**
     * p(tgt_seg|src_seg).
     * @param src_toks
     * @param tgt_toks
     * @param smooth used in place of 0 probs when calculating logpr()
     */
    double logpr(const vector<string>& src_toks, const vector<string>& tgt_toks,
                 double smooth = 1e-50);

    void align(const vector<string>& src, const vector<string>& tgt, 
               vector<Uint>& tgt_al, bool twist = false,
               vector<double>* tgt_al_probs = NULL);
  };

}

#endif
