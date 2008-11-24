/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_strategy.h  The HMMJumpStrategy abstract class.
 * 
 * Since we have several variants in how HMM jump parameters are implemented,
 * following different publications, we need an encapsulation that allows us
 * the easily support the different types of jump parameters and counts without
 * making the HMMAligner class to complex and full if statements.  We use the
 * Strategy design pattern to accomplish this.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */


#ifndef HMM_JUMP_STRATEGY_H
#define HMM_JUMP_STRATEGY_H

#include "portage_defs.h"
#include "hmm.h"
#include <boost/optional/optional.hpp>

namespace Portage {

using boost::optional;
class TTable;

/**
 * Macro intended for use by each subclass of HMMJumpStrategy.
 * Each subclass must declare its unique magic string using this macro.
 * This magic string will be used when writing models and in the factory method
 * CreateAndRead().
 * Usage: include a line like this one in the public: part of your subclass:
 *   HMMJUMP_MAGIC_STRING("This is my unique magic string")
 */
#define HMMJUMP_MAGIC_STRING(magic_string) \
   virtual const char* getMagicString() const \
      { return (magic_string); } \
   static bool matchMagicString(const string& s) \
      { return s == (magic_string); }

/**
 * Macro intended for use by each subclass of HMMJumpStrategy.
 * Each subclass must declare the magic string for its binary count files
 * using this macro.  Again, this string should be unique accross subclasses.
 * Usage:
 *   HMMJUMP_COUNT_STRING("This is my unique magic string for counts files")
 */
#define HMMJUMP_COUNT_STRING(count_string) \
   virtual const char* getCountMagicString() const \
      { return (count_string); }

/// Interface to the several variants of HMM jump parameters and counts.
class HMMJumpStrategy {

public:
   virtual void set_p_zero(double _p_zero) { p_zero = _p_zero; }
   virtual void set_uniform_p0(double _uniform_p0) { uniform_p0 = _uniform_p0; }
   virtual void set_alpha(double _alpha) { alpha = _alpha; }
   virtual void set_lambda(double _lambda) { lambda = _lambda; }
   virtual void set_anchor(bool _anchor) { anchor = _anchor; }
   virtual void set_max_jump(Uint _max_jump) { max_jump = _max_jump; }

protected:

   /**
    * Constant part of the probability of transition to an empty word.
    */
   double p_zero;

   /**
    * Add uniform_p0/(I+1) to p_zero for each sentence.
    * To replicate Liang et al, 2006, use uniform_p0 = 1 and p_zero = 0.
    */
   double uniform_p0;

   /**
    * Smoothing parameter for transition probabilities.
    * Alpha is used as described in Och+Ney sct 3.3
    */
   double alpha;

   /**
    * Alternative smoothing parameter for transition probabilities:
    * following Lidstone's law, add lambda to all counts before normalizing.
    */
   double lambda;

   /**
    * If true, append a dummy, force-linked, src/tgt word pair to all sequences
    * for all calculations. This is intended to encourage alignments to be more
    * diagonal, and in particular helps sentence-ending punctuation to link up.
    */
   bool anchor;

   /* *Removed, since this is handled by the two subclasses separately.*
    * If true, uses a distinct set of jump parameters from the sentence start
    * and another set of jump parameters to the final anchor.  When true,
    * anchor should preferably also be true.  Liang et al, 2006 use this.
    */
   //bool end_dist;

   /**
    * If non-zero, bounds forward and backward jump parameters and counts.
    * In this case, any jump or count larger than max_jump is binned with the
    * jump or count for max_jump.  Liang et al, 2006 use 5.  He, WMT-07 uses 7.
    */
   Uint max_jump;

   /**
    * Default constructor initializes all parameters to default values for sub
    * classes.
    */
   HMMJumpStrategy();

   /**
    * Initialize base class parameters
    */
   void init_base_class(double p_zero, double uniform_p0, double alpha,
                        double lambda, bool anchor, Uint max_jump);

   /**
    * Apply alpha smoothing
    * @param num   unsmooted probability's numerator
    * @param denom unsmooted probability's denominator
    * @param N     number of events over which to distribute alpha
    */
   double alpha_smooth(double num, double denom, Uint N) const {
      assert(N>0);
      if ( denom == 0.0 )
         // use uniform when the estimate had no data yet
         return 1.0/N;
      else
         return (1.0-alpha) * num / denom + alpha/N;
   }

   /**
    * Fill in the parts of the jump probabilities that don't typically depend
    * on the strategy.  SubClass::fillHMMJumpProbs() should invoke this method
    * if these defaults are appropriate: jumps back to the start state are not
    * allowed (i.e., prob is 0.0), jumps out of the end anchor (if used) are
    * not allowed, and all jumps to null states are based on p_zero and
    * uniform_p0.
    * @param hmm       model in which to add jump probabilities
    * @param I         length of target sequence including the end anchor, if
    *                  used, and excluding the NULL position. 
    * @return the value of p0 used for this sentence pair
    */
   double fillDefaultHMMJumpProbs(HMM* hmm, Uint I) const;

   /**
    * Read the model's jump parameters
    * @param in          input stream to read from
    * @param stream_name in's file name (or some string that is meaningful
    *                    to the user), to produce informative error messages
    * @pre if SubClass::read(in) is invoked, then in was written by
    *      SubClass::write()
    * @pre in is at the position where SubClass::write() started writing
    */
   virtual void read(istream& in, const char* stream_name) = 0;

   /**
    * Write the current counts out in binary format.
    * Note: subclasses should override this method, not writeBinCounts().
    * @param os output stream to write to
    */
   virtual void writeBinCountsImpl(ostream& os) const = 0;

   /**
    * Read counts from is, adding them to the current counts
    * Note: subclasses should override this method, not readAddBinCounts()
    * @param is          input stream to read from
    * @param stream_name is's file name (or some string that is meaningful
    *                    to the user), to produce informative error messages
    * @pre if SubClass::readAddBinCounts(is) is invoked, then is was written by
    *      SubClass::writeBinCounts()
    * @pre is is at the position where SubClass::writeBinCounts() started
    *      writing
    */
   virtual void readAddBinCountsImpl(istream& is, const char* stream_name) = 0;

public:

   /**
    * Create a new, empty strategy.
    * The type of strategy created will depend on the parameter values
    * provided.
    * Should be deallocated by caller once it is no longer needed.
    * See HMMAligner::HMMAligner() for parameter documentation.
    */
   static HMMJumpStrategy* CreateNew(
         double p_zero, double uniform_p0,
         double alpha, double lambda,
         bool anchor, bool end_dist, Uint max_jump,
         const char* word_classes_file,
         double map_tau, double map_min_count);

   /**
    * Instantiate a new strategy and load the model provided.
    * @param dist_file_name name of the file to load, which will also determine
    *                       what type of strategy will be instantiated.
    * See HMMAligner::HMMAligner() for the documentation of the remaining
    * parameters.
    */
   static HMMJumpStrategy* CreateAndRead(
         const string& dist_file_name,
         optional<double> p_zero,
         optional<double> uniform_p0,
         optional<double> alpha,
         optional<double> lambda,
         optional<bool>   anchor,
         optional<bool>   end_dist,
         optional<Uint>   max_jump);

   /**
    * Instantiate a new strategy and load the model provided.
    * @param is             input stream open on the dist file
    * @param dist_file_name name of the dist file, to provide meaningful
    *                       diagnostic messages in case of errors.
    */
   static HMMJumpStrategy* CreateAndRead(
         istream& is,
         const char* dist_file_name,
         optional<double> p_zero     = optional<double>(),
         optional<double> uniform_p0 = optional<double>(),
         optional<double> alpha      = optional<double>(),
         optional<double> lambda     = optional<double>(),
         optional<bool>   anchor     = optional<bool>(),
         optional<bool>   end_dist   = optional<bool>(),
         optional<Uint>   max_jump   = optional<Uint>());

   /**
    * Return a clone of self
    */
   virtual HMMJumpStrategy* Clone() const = 0;

   /**
    * Virtual destructor so that sub-classes get deleted correctly.
    */
   virtual ~HMMJumpStrategy();

   /**
    * Write the model's jump parameters.
    * @param out output stream to write to
    * @param bin if true, use a binary format for faster read/write
    */
   virtual void write(ostream& out, bool bin = false) const = 0;

   /**
    * Return a string that identifies this strategy in an HMM model file.
    * The string must be unique to this strategy, so that CreateAndRead() can
    * use it to determine what strategy to use when loading jump parameters.
    * The string should be descriptive, since it is also used to make jump
    * parameter files self-documenting.
    * The string may not contain any newlines.
    * This method must be overridden implicitely in each subclass via the
    * HMMJUMP_MAGIC_STRING macro.
    * @return this strategy's unique identification string
    */
   virtual const char* getMagicString() const = 0;

   /**
    * Returns true iff s is this class's magic string, as returned by
    * getMagicString().
    * This method must be overridden implicitely in each subclass via the
    * HMMJUMP_MAGIC_STRING macro.
    * @param s  magic string to test against this class's
    * @return true if s equals this class's magic string
    */
   static bool matchMagicString(const string& s) { return false; }

   /**
    * Write the current counts out in binary format.
    * Note: subclasses should not override this method, but rather
    * writeBinCountsImpl().
    * @param os output stream to write to
    */
   void writeBinCounts(ostream& os) const;

   /**
    * Read counts from is, adding them to the current counts
    * Note: subclasses should not override this method, but rather
    * readAddBinCountsImpl()
    * @param is          input stream to read from
    * @param stream_name is's file name (or some string that is meaningful
    *                    to the user), to produce informative error messages
    * @pre if SubClass::readAddBinCounts(is) is invoked, then is was written by
    *      SubClass::writeBinCounts()
    * @pre is is at the position where SubClass::writeBinCounts() started
    *      writing
    */
   void readAddBinCounts(istream& is, const char* stream_name);

   /**
    * Used for testing only - check that the counts of s are identical to this.
    * @param s  strategy object to compare this with
    * @return true iff s and *this are the same type and have the same counts
    */
   virtual bool hasSameCounts(const HMMJumpStrategy* s) = 0;

   /**
    * Return a string that identifies this strategy in an HMM count file.
    * The string must be unique to this strategy, so that HMMAligner can use
    * it to determine what strategy to use when loading jump counts.
    * The string should be descriptive, since it is also used to make jump
    * parameter files self-documenting.
    * The string may not contain any newlines.
    * This method must be overridden implicitely in each subclass via the
    * HMMJUMP_COUNT_STRING macro.
    * @return this strategy's unique identification string for count files
    */
   virtual const char* getCountMagicString() const = 0;

   /**
    * Reset all the jump counts to 0, in preparation for calling
    * readAddBinCounts() and/or countJumps().
    * @param tt  The alignment model's TTable, required by some strategies.
    */
   virtual void initCounts(const TTable& tt) = 0;

   /**
    * Count the expected jump occurrences in one sentence pair.
    * @param A_counts expected counts calculated using the forward-backward
    *                 procedure
    * @param src_toks *target* sequence, i.e., hidden states, src_toks[0]
    *                 must be IBM1::nullWord(), for null alignments.
    * @param tgt_toks *source* sequence, i.e., the observed sequence.
    * @param I        length of target sequence including the end anchor, if
    *                 used, and excluding the NULL position. 
    */
   virtual void countJumps(const dMatrix& A_counts,
         const vector<string>& src_toks,
         const vector<string>& tgt_toks,
         Uint I) = 0;

   /**
    * Estimate the model parameters from the counts.
    * @param ???
    */
   virtual void estimate() = 0;

   /**
    * Fill in the HMM jump probabilities that depend on the strategy: all but
    * jumps to null states, transitions back into start state or out of anchor.
    * @param hmm       model in which to add jump probabilities
    * @param src_toks  *target* sequence, i.e., hidden states, src_toks[0]
    *                  must be IBM1::nullWord(), for null alignments.
    * @param tgt_toks  *source* sequence, i.e., the observed sequence.
    * @param I         length of target sequence including the end anchor, if
    *                  used, and excluding the NULL position. 
    */
   virtual void fillHMMJumpProbs(HMM* hmm, 
         const vector<string>& src_toks,
         const vector<string>& tgt_toks,
         Uint I
   ) const = 0;

   /**
    * Accessor for the anchor parameter.  This accessor is required because the
    * presence of an anchor not only affects the jump parameters, controlled by
    * each strategy, but also the emission probabilities, controlled by
    * HMMAligner itself.
    */
   bool getAnchor() { return anchor; }

}; // HMMJumpStrategy

} // Portage

#endif // HMM_JUMP_STRATEGY_H
