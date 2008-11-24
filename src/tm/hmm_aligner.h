/**
 * @author Eric Joanis
 * @file hmm_aligner.h  HMM-based extension of the IBM 1 alignment model.
 *                      This alignment model is an alternative to IBM 2 models.
 * 
 * Based on:
 *  - George Foster's "The HMM alignment model and you" problem specs.
 *    hmm-alignment.{tex,pdf}.  Read this first for an overview of the model
 *    this class implements.
 *  - Vogel, Ney and Tillmann. 1996. "HMM-based word alignment in statistical
 *    translation." COLING. 
 *  - Och and Ney. 2003. "A systematic comparison of various statistical
 *    alignment models". CL 29(1).
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __HMM_ALIGNER_H__
#define __HMM_ALIGNER_H__

#include "portage_defs.h"
#include "ibm.h"
#include "hmm.h"
#include "hmm_jump_strategy.h"
#include <boost/optional/optional.hpp>

namespace Portage {

using boost::optional;

/// HMM word alignment model
class HMMAligner : public IBM1 {
   /// Jump parameter and count strategy
   HMMJumpStrategy* jump_strategy;

   /**
    * Make the HMM model for the given sentence pair
    * @param src_toks  hidden states, src_toks[0]
    *                  must be IBM1::nullWord(), for null alignments.
    * @param tgt_toks  observed sequence.
    * @param O         will be set to the observed sequence in the format the
    *                  HMM will expect it.
    * @param smooth    value to use for alignments of unknown word pairs.
    * @param eq_smooth value to use for alignments of unknown word pairs
    *                  that are the same string; 0 means use \<smooth\> 
    * @param use_null  if true, has the same effect as useImplicitNulls
    * @return HMM model for this sentence pair; should be deleted by caller.
    */
   shared_ptr<HMM> makeHMM(const vector<string>& src_toks,
                           const vector<string>& tgt_toks,
                           uintVector &O, double smooth = 0.0,
                           double eq_smooth = 0.0, bool use_null = false);

   /// Object to encapsulate the handling of implicit nulls in src_toks.
   /// Implicit nulls make everything more complicated.  We hide that
   /// complexity using this class, which explicitly represents nullWord().
   class StringVecWithExplicitNull {
      vector<string> storage;
      const vector<string>& toks_with_null;
     public:
      StringVecWithExplicitNull(const vector<string>& toks, bool implicitNull);
      size_t size() { return toks_with_null.size(); }
      bool empty() { return toks_with_null.empty(); }
      const string& operator[](size_t i) { return toks_with_null[i]; }
      /// Implicit conversion to a const vector<string>& so these can be
      /// easily passed to the many methods expecting such vectors.
      operator const vector<string>& () { return toks_with_null; }
   };

   /// Private data structure and helper for count_symmetrized()
   struct count_symmetrized_helper {
      /// Pointer to parent model
      HMMAligner* parent;
      /// Source (hidden) sequence
      const vector<string>& src_toks;
      /// Copy of source (hidden) sequence with the NULL token prepended
      StringVecWithExplicitNull src_toks_with_null;
      /// Target (observed) sequence
      const vector<string>& tgt_toks;
      /// Source token count including anchor (if any)
      Uint I;
      /// Observed sequence
      uintVector O;
      /// The actual HMM
      shared_ptr<HMM> hmm;
      /// This HMM's M value, excluding end anchor if any
      Uint M;
      /// Alpha hat matrix produced by the Forward Procedure
      dMatrix alpha_hat;
      /// c vector produced by the Forward Procedure
      dVector c;
      /// log prob of current sentence pair according to current model
      double cur_logprob;
      /// Beta hat matrix produced by the Backward Procedure
      dMatrix beta_hat;
      /**
       * Gamma matrix produced by HMM::statePosteriors()
       * gamma(k+1, i+1) is the posterior probability of aligning tgt[k] to
       * src[i]
       * gamma(0,0) = 1, gamma(0,i+1) = 0, and gamma(k+1,0) = 0, by def'n of
       * the dummy start state
       * gamma(k+1, i+1+I+1) is the posterior probability of null-aligning
       * tgt[k], with the last previously aligned word having been aligned to
       * src[i] (joint prob, not conditional prob)
       * gamma(k+1, 0+I+1) is the posterior probability of null-aligning
       * tgt[k], having null-aligned tgt[0] .. tgt[k-1] too.
       */
      dMatrix gamma;
      /// Posterior probabily of NULL alignment for each observed word
      /// Will have size M
      vector<double> null_posteriors;
      /// Indirectly calculated NULL ailgnment posterior prob for each hidden
      /// word, defined in the tech report.  Will have size src_toks.size().
      vector<double> null_posteriors_rev;

      /// The constructor does all computation that is not dependent on the
      /// other direction
      count_symmetrized_helper(HMMAligner* parent,
         const vector<string>& src_toks,
         const vector<string>& tgt_toks);

      /// jumps counts - filled by count_jumps(), not by
      /// HMM::BWCountExpectation() as would be the case in the non-symmetrized
      /// case.
      dMatrix sym_A_counts;

      /// This method counts the symmetrized jump and lexical expectations
      /// @param r Helper for the reversed direction
      void count_expectations(const count_symmetrized_helper& r);

      /// For debugging purposes, dump debugging information to cerr
      void dump();

   };

  public:
   /// Affects the behaviour of count_symmetrized() - see that method's
   /// documentation for details.  Only relevant when training symmetrized
   /// models.
   bool useLiangSymVariant;

  public:
   /**
    * Construct an empty model; use add() to populate and do a few IBM1
    * training iterations first.
    * @param p_zero     p_0 parameter (Och+Ney sct 2.2.1): constant part of
    *                   prob of transition to an empty word.
    * @param uniform_p0 add uniform_p0/(I+1) (where I is the source sentence
    *                   length, i.e. src_toks.size() in the arguments to
    *                   makeHMM() above) to p_zero for each sentence.  
    *                   To replicate Liang et al, 2006, use uniform_p0 = 1 and
    *                   p_zero = 0.
    * @param alpha      alpha parameter (Och+Ney sct 3.3): smoothing parameter
    *                   for transition probabilities.
    * @param lambda     parameter for +lambda smoothing (Lidstone's law)
    * @param anchor     if true, anchor alignments to end of sequences - this
    *                   is equivalent to appending a dummy src/tgt word pair
    *                   that are always linked to each other
    * @param end_dist   if true, use distinct parms for initial and final jumps
    * @param max_jump   if non-zero, bin jumps >= max_jump together.
    * @param word_classes_file  if non-null, the name of a file containing word
    *                   classes; implies the HMMJumpClasses strategy.
    * @param map_tau    The tau parameter to MAP learning, which determines
    *                   the weight given the prior model; a non-zero value
    *                   implies the HMMJumpMAP strategy, with a prior chosen
    *                   according to all the parameters above.
    * @param map_min_count  Pruning parameter for MAP learning: words with
    *                   fewer than map_min_count total jump counts are pruned
    *                   and use the prior alone.
    *                   Only meaningful if map_tau != 0.
    */
   HMMAligner(double p_zero, double uniform_p0,
              double alpha, double lambda,
              bool anchor, bool end_dist, Uint max_jump,
              const char* word_classes_file,
              double map_tau, double map_min_count);

   /**
    * Construct a model using an existing ttable file, but empty HMM distance
    * parameters.
    * @param ttable_file file containing the ttable.
    * @param dontcare   Dummy parameter to disambiguate this constructor's
    *                   signature from the next one's.
    * See the first constructor's documentation for the rest of the parameters.
    */
   HMMAligner(const string& ttable_file, string dontcare,
              double p_zero, double uniform_p0,
              double alpha, double lambda,
              bool anchor, bool end_dist, Uint max_jump,
              const char* word_classes_file,
              double map_tau, double map_min_count);

   /**
    * Construct using an existing ttable file and distance parameters file. The
    * distance params file must be called distParamFileName(ttable_file).
    *
    * Note about all optional<T> parameters: they may be initialized directly
    * from a value of type T, if the desired values are known, or left formally
    * uninitialized by invoking optional<T>'s default constructor, in which
    * case the value of that parameter will be read from the HMM distance
    * parameter file.  This is the default behaviour if these parameters are
    * left out.
    *
    * @param ttable_file  file containing the ttable.
    * @param p_zero     p_0 parameter (Och+Ney sct 2.2.1): prob of transition
    *                   to an empty word.
    * @param uniform_p0 add uniform_p0/(I+1) (where I is the source sentence
    *                   length, i.e. src_toks.size() in the arguments to
    *                   makeHMM() above) to p_zero for each sentence.  
    *                   To replicate Liang et al, 2006, use uniform_p0 = 1 and
    *                   p_zero = 0.
    * @param alpha      alpha parameter (Och+Ney sct 3.3): smoothing parameter
    *                   for transition probabilities.
    * @param lambda     parameter for +lambda smoothing (Lidstone's law).
    * @param anchor     if true, anchor alignments to end of sequences - this
    *                   is equivalent to appending a dummy src/tgt word pair
    *                   that are always linked to each other.
    * @param end_dist   if true, use distinct parms for initial and final jumps
    * @param max_jump   if non-zero, bin jumps >= max_jump together.
    */
   HMMAligner(const string& ttable_file,
              optional<double> p_zero     = optional<double>(),
              optional<double> uniform_p0 = optional<double>(),
              optional<double> alpha      = optional<double>(),
              optional<double> lambda     = optional<double>(),
              optional<bool>   anchor     = optional<bool>(),
              optional<bool>   end_dist   = optional<bool>(),
              optional<Uint>   max_jump   = optional<Uint>());

   /**
    * Get the .dist filename from the base ttable filename.
    * @param ttable_name  
    * @return 
    */
   static const string distParamFileName(const string& ttable_name) {
      return addExtension(removeZipExtension(ttable_name), ".dist");
   };

   /**
    * Write IBM1 params to ttable_file and HMM params to
    * distParamFileName(ttable_file).
    * @param ttable_file  file name to output the IBM1 and HMM values.
    * @param bin_ttable   write the ttable part in binary format.
    */
   virtual void write(const string& ttable_file, bool bin_ttable = false) const;

   /// Writes the current counts to a file.
   /// @param count_file   file name to output the IBM1 counts, HMM counts
   ///                     will go to count_file.hmm
   virtual void writeBinCounts(const string& count_file) const;

   /// Reads counts from a file, adding them to the current counts
   /// @param count_file   file name to read the IBM1 counts from, HMM counts
   ///                     will go be read from count_file.hmm
   virtual void readAddBinCounts(const string& count_file);

   // Overloaded methods - see class IBM1 for any missing documentation

   virtual void initCounts();

   /**
    * Count the expected alignment occurrences in one sentence pair.
    * @param src_toks hidden sequence, src_toks[0]
    *                 should be IBM1::nullWord(), for null alignments.
    * @param tgt_toks observed sequence.
    * @param use_null prepend an implicit null to src_toks if true
    */
   virtual void count(const vector<string>& src_toks,
                      const vector<string>& tgt_toks, bool use_null);

   /**
    * @copydoc IBM1::count_symmetrized(const vector<string>&,const vector<string>&,bool,IBM1*)
    * @pre reverse_model must be an HMMAligner*
    *
    * This method's behaviour is affected by useLiangSymVariant: if false,
    * jump parameters are updated using the base implementation described in
    * the "Decoding Liang et al's Alignment by Agreement" tech report, if false
    * the variant implementation described in the same paper is used.
    */
   virtual void count_symmetrized(const vector<string>& src_toks,
                                  const vector<string>& tgt_toks,
                                  bool use_null, IBM1* reverse_model);

   virtual pair<double,Uint> estimate(double pruning_threshold,
                                      double null_pruning_threshold);

   /// Not implementable since the whole tgt sentence is required to perform
   /// any HMM calculatios - fatal error if called
   virtual double pr(const vector<string>& src_toks, const string& tgt_tok, 
                     Uint tpos, Uint tlen, vector<double>* probs = NULL);

   /// Not implementable since the whole tgt sentence is required to perform
   /// any HMM calculatios - fatal error if called
   virtual double pr(const vector<string>& src_toks, const string& tgt_tok, 
                     vector<double>* probs = NULL);

   /// Returns the probability summed over all alignments
   virtual double logpr(const vector<string>& src_toks,
                        const vector<string>& tgt_toks,
                        double smooth = 1e-07);

   /// Returns the probability over the most probable (i.e., Viterbi) alignment
   double viterbi_logpr(const vector<string>& src_toks,
                        const vector<string>& tgt_toks,
                        double smooth = 1e-07);

   /// Not implemented - fatal error if called
   virtual double minlogpr(const vector<string>& src_toks,
                           const string& tgt_tok,
                           double smooth = 1e-07);

   /// Not implemented - fatal error if called
   virtual double minlogpr(const string& src_toks,
                           const vector<string>& tgt_toks,
                           double smooth = 1e-07);

   virtual void align(const vector<string>& src, const vector<string>& tgt, 
                      vector<Uint>& tgt_al, bool twist = false, 
                      vector<double>* tgt_al_probs = NULL);

   virtual double linkPosteriors(const vector<string>& src,
                                 const vector<string>& tgt,
                                 vector<vector<double> >& posteriors);

   virtual void testReadWriteBinCounts(const string& count_file) const;

}; // HMMAligner
} // Portage

#endif // __HMM_ALIGNER_H__

