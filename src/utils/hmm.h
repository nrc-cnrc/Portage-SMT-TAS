/**
 * @author Eric Joanis
 * @file hmm.h Standard HMM object with typical HMM manipulation methods
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __HMM_H__
#define __HMM_H__

#include "portage_defs.h"
#include <iostream>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>

namespace Portage {

// Convenient typedefs so we don't have to repeat boost::num... all over.
typedef double HMMPrecision;
typedef boost::numeric::ublas::vector<HMMPrecision> dVector;
typedef boost::numeric::ublas::matrix<HMMPrecision> dMatrix;
typedef boost::numeric::ublas::vector<Uint> uintVector;
typedef boost::numeric::ublas::matrix<Uint> uintMatrix;

/**
 * HMM object with standard definition and methods.
 *
 * All variable names are based on those used by Manning and Schuetze, chapter
 * 9, which are generally consistent with Rabiner, 1989.
 *
 * The reader who is not already well familiar with HMMs is advised to read at
 * least one of these two references before attempting to understand this code.
 * Most algorithms implemented here do not include documentation redundant with
 * those references.
 *
 * This implementation allows storing a subset of a model, where 0 .. N-1 is
 * not the complete set of states, and 0 .. M-1 is not the complete emission
 * alphabet.  In this case, the probability distributions in B are allowed to
 * have a sum smaller than 1, and the missing probability mass is assumed to be
 * for symbols that will not appear in the observed sequences.  A should still
 * contain complete probability distributions, since transitions from i < M to
 * j >= M must be assumed to have 0 probabilities.
 *
 * Most model parameters are publicly changeable because the user is expected
 * to initialize them directly.
 */
class HMM {

  private:
   const Uint N; ///< number of states
   const Uint M; ///< Emission alphabet size

  public:
   /**
    * HMM emission types
    */
   enum EmissionType {
      arc_emit,             ///< emit on transition from i to j
      state_entry_emit,     ///< emit upon entering state j (most typical)
      state_exit_emit       ///< emit upon leaving state i
   };

  private:
   const EmissionType emission_type; ///< emission type of this HMM

  public:
   /**
    * Model parameter types
    */
   enum ParameterType {
      /// model stores all parameters in log-probs; legal values; [-inf, 0.0]
      log_probs,
      /// model stores all parameters in regular probs; legal value: [0.0, 1.0]
      regular_probs,
   };

  private:
   /// whether Pi, A and B contain log probs or regular probs
   ParameterType parm_type;

   /**
    * Initial state probabilities
    */
   dVector Pi_storage;

   /**
    * State transition probabilities.
    * A_ij is A(i,j)
    */
   dMatrix A_storage;

   /**
    * Symbol emission probabilities.
    * B_ijk is B[i](j,k) in an arc_emit HMM,
    * B_jk is B[0](j,k) is a state_entry_emit HMM.
    * B_ik is B[0](i,k) is a state_exit_emit HMM.
    */
   vector<dMatrix> B_storage;

   /**
    * Does the actual work for Viterbi(), for a log-prob model.
    */
   double logViterbi(const uintVector &O, uintVector &X_hat,
                     bool verbose) const;

   /**
    * Does the actual work for Viterbi(), for a straight prob model.
    */
   double probViterbi(const uintVector &O, uintVector &X_hat,
                      bool verbose) const;

  public:
   /**
    * Number of states in the HMM: Start state is 0, other states are 1 .. N-1.
    */
   Uint getN() { return N; }

   /**
    * Emission alphabet size: emission symbols are 0 .. M-1.
    */
   Uint getM() { return M; }

   /**
    * HMM emission type of this HMM: arc or state.
    */
   EmissionType getEmissionType() { return emission_type; }

   /**
    * Convert this HMM to a log-prob model, by taking the log of all
    * parameters.
    *
    * Note: this was implemented with the assumption that Viterbi would run
    * faster on a log HMM model than a regular prob one, but that turns out to
    * be false, as I found out doing some benchmarks.  Hence, although this
    * method is still available, its use is not generally recommended.
    *
    * @pre parm_type == regular_probs
    * @post parm_type == log_probs
    */
   void convertToLogModel();

   /**
    * Initial probabilities.
    * Pi(i) = Pi_i = probability of starting in state i.
    * @param i state
    * @return modifiable ref to Pi_i
    */
   double & Pi(Uint i) {
      #ifndef NDEBUG
      assert (i < N);
      #endif
      return Pi_storage[i];
   }
   const double & Pi(Uint i) const {
      return const_cast<HMM*>(this)->Pi(i);
   }

   /**
    * State transition probabilities.
    * A(i,j) = A_ij = probability of going from state i to state j
    * @param i from state
    * @param j to state
    * @pre i < N and j < N
    * @return modifiable ref to A_ij
    */
   double & A(Uint i, Uint j) {
      #ifndef NDEBUG
      assert(i < N);
      assert(j < N);
      #endif
      return A_storage(i,j);
   }
   const double & A(Uint i, Uint j) const {
      return const_cast<HMM*>(this)->A(i,j);
   }

   /**
    * Check if Pi and A are proper probability distributions.
    * The user may call this to make sure they initialized Pi and A correctly.
    * @param complete  if true, assume the transition distributions are
    *                  complete and must sum to 1; otherwise assume this HMM is
    *                  a subset of a larger HMM and the sum may be less than 1.
    * @param may_exceed_1  if true, allow a sum>1 from a state without
    *                  complaint.  Intended for use with optimization tricks
    *                  such as conflating states that have disjoint emission
    *                  sets.  (implies !complete)
    * @return true iff Pi and A are all proper probability distributions.
    */
   bool checkTransitionDistributions(bool complete = true, bool may_exceed_1 = false) const;

   /**
    * Symbol emission probabilities.
    * Access B_ijk in an arc emission HMM, or B_jk in a state emission HMM.
    *
    * if emission_type == arc_emit
    *    B_ijk = prob of emitting symbol k when going from state i to j.
    * if emission_type == state_entry_emit
    *    B_jk = B_ijk = prob of emitting symbol k when entering state j.
    * if emission_type == state_exit_emit
    *    B_jk = B_ijk = prob of emitting symbol k when leaving state i.
    *
    * State 0 being the start state, it is assumed not to emit initially, only
    * when returned to later on.
    *
    * @param i from state (ignored if emission_type == state_entry_emit)
    * @param j to state (ignored if emission_type == state_exit_emit)
    * @param k emission symbol
    * @pre i < N, j < N, and k < M.
    * @return modifiable ref to B_ijk
    */
   double & B(Uint i, Uint j, Uint k) {
      #ifndef NDEBUG
      assert(i < N);
      assert(j < N);
      assert(k < M);
      #endif
      switch (emission_type) {
         case arc_emit:         return B_storage[i](j,k);
         case state_entry_emit: return B_storage[0](j,k);
         case state_exit_emit:  return B_storage[0](i,k);
         default:               assert(false);
      }
   }
   const double & B(Uint i, Uint j, Uint k) const {
      return const_cast<HMM*>(this)->B(i,j,k);
   }

   /**
    * Symbol emission probabilities for state emission HMMs only.
    * B(i,k) is the same as B(i,anything,k) for state exit emission.
    * B(i,k) is the same as B(anything,i,k) for state entry emission.
    * @param i state
    * @param k emission symbol
    * @return modifiable ref to B_ik
    */
   double & B(Uint i, Uint k) {
      #ifndef NDEBUG
      assert(i < N);
      assert(k < M);
      assert(emission_type != arc_emit);
      #endif
      return B_storage[0](i,k);
   }
   const double & B(Uint i, Uint k) const {
      return const_cast<HMM*>(this)->B(i,k);
   }

   /**
    * Verify that the emission probabilities are valid.
    * @param complete   if true, assume the emission distribution is complete
    *                   and must sum to 1; otherwise assume it is a subset of a
    *                   larger HMM and the sum may be less than 1.
    * @param may_exceed_1  if true, allow a sum>1 from a state without
    *                  complaint.  Intended for use with optimization tricks
    *                  such as conflating states that have disjoint emission
    *                  sets.  (implies !complete)
    * @return true iff B contains no invalid probability distributions.
    */
   bool checkEmissionDistributions(bool complete, bool may_exceed_1 = false) const;

   /**
    * Constructor.
    * Allocates memory for A, B and Pi.
    *
    * Leaves A and B uninitialized: the user *must* initialize them using Aij()
    * and Bijk().
    *
    * Initializes Pi_0 to 1.0 and Pi_i to 0.0 for 0 < i < N, which is correct
    * if 0 is the only start state.  Pi can be reinitialized using Pi_i if 0 is
    * not the only start state.
    *
    * @param N          number of states, including the start state
    * @param M emission alphabet size
    * @param emission_type type of emission (arc or state) of this HMM
    * @param parm_type  whether Pi, A and B will contain log or straight probs
    */
   HMM(Uint N, Uint M, EmissionType emission_type, ParameterType parm_type);

   /**
    * Destructor.
    */
   ~HMM();

   /**
    * Write the HMM is compact but human readable form
    * @param os output stream where to write
    */
   void write(ostream& os) const;

   /**
    * Run the Viterbi algorithm on a given sequence
    * @param O          observed output sequence; let T = O.size().
    * @param X_hat      will be set to the most probable state sequence; will
    *                   have size T+1; X_hat[0] will be the start state,
    *                   X_hat[T] will be the final state.
    * @param verbose    print a lot of stuff to cerr if true
    * @return log(P(X_hat, O | mu)) (using natural log)
    */
   double Viterbi(const uintVector &O, uintVector &X_hat,
                  bool verbose = false) const {
      return parm_type == log_probs ? logViterbi(O, X_hat, verbose)
                                    : probViterbi(O, X_hat, verbose);
   }

   /**
    * Calculate the posterior probability of each state at each time:
    * P(X_t == i | O, mu).  See Manning and Schuetze sct 9.3.2, p. 331.  Uses
    * the forward backward procedure.
    * This can be used to calculate the most likely state at each time step,
    * but there is no guarantee that the result will be the same as the Viterbi
    * sequence, and the total probability is not the product of the individual
    * probabilities.
    *
    * @param O          observed output sequence; let T = O.size().
    * @param[out] gamma will be resized to T+1 x N, and gamma(t,i) will
    *                   contain gamma_i(t) == P(X_t == i | O, mu) (See M+S),
    *                   with gamma_i(0) being initial state probabilities given
    *                   O and mu, gamma_i(T) being end state probabilities, etc.
    * @param verbose    print a lot of stuff to cerr if true
    * @return log(P(O|mu))
    */
   double statePosteriors(const uintVector &O, dMatrix &gamma,
                          bool verbose = false) const;

   /**
    * Same as statePosteriors() with 3 parameters, to use when you've already
    * called ForwardProcedure() and BackwardProcedure() yourself.
    * @param alpha_hat  calculated by ForwardProcedure()
    * @param beta_hat   calculated by BackwardProcedure()
    */
   void statePosteriors(const uintVector &O,
                        const dMatrix &alpha_hat, const dMatrix &beta_hat,
                        dMatrix& gamma, bool verbose = false) const;

   /**
    * Calculate the posterior probability of each transition at each time:
    * P(X_t = i, X_t+1 = j | O, mu).  The Forward-Backward procedure must have
    * been executed first.  Calculated using eqn 9.16 of Manning+Schuetze.
    * @param O          observed output sequence; let T = O.size().
    * @param alpha_hat  calculated by ForwardProcedure()
    * @param beta_hat   calculated by BackwardProcedure()
    * @param[out] p     Will have size T x N x N, and P[t](i,j) will
    *                   contain P_t(i,j) = P(X_t = i, X_t+1 = j | O, mu)
    * @param verbose    print a lot of stuff to cerr if true
    */
   void transitionPosteriors(const uintVector &O, 
                             const dMatrix &alpha_hat, const dMatrix &beta_hat,
                             vector<dMatrix>& p, bool verbose = false) const;

   /**
    * Calculate the posterior probability of each transition at time t:
    * P(X_t = i, X_t+1 = j | O, mu).  The Forward-Backward procedure must have
    * been executed first.  Calculated using eqn 9.16 of Manning+Schuetze.
    * @param O          observed output sequence; let T = O.size().
    * @param alpha_hat  calculated by ForwardProcedure()
    * @param beta_hat   calculated by BackwardProcedure()
    * @param t          The time 0 <= t < T for which to calculate p_t
    * @param[out] p_t   Will have size N x N, and p_t(i,j) will contain
    *                   P_t(i,j) = P(X_t = i, X_t+1 = j | O, mu)
    * @param verbose    print a lot of stuff to cerr if true
    */
   void transitionPosteriors(const uintVector &O, 
                             const dMatrix &alpha_hat, const dMatrix &beta_hat,
                             Uint t, dMatrix& p_t,
                             bool verbose = false) const;

   /**
    * Run the forward procedure for calculating the prob of an observation.
    * The calculated alpha_hat(t,i) is alpha^_t(i) from Rabiner 1989 section V.
    * A. Scaling, and c[t] is c_t from the same reference.
    *
    * P(O|mu) = 1 / prod_{t=0..T}(c[t]). It is done in logs to avoid underflow.
    *
    * @pre the model is a straight prob model (not log-prob)
    *
    * @param O          observed output sequence; let T = O.size().
    * @param alpha_hat  this matrix will be resized to T+1 x N and will be
    *                   calculated by this procedure: alpha_hat(t, i) is
    *                   alpha^_t(i) from Rabiner (1989) section "V." subsection
    *                   "A.  Scaling"
    * @param c          this vector will be resized to T+1 and will be
    *                   calculated by this procedure: c[t] is c_t from the same
    *                   reference.
    * @param scale      if true, use Rabiner scaling
    *                   if false, c[i] = 1.0 for all i
    * @param verbose    print a lot of stuff to cerr if true
    * @return log(P(O|mu))
    */
   double ForwardProcedure(const uintVector &O, dMatrix &alpha_hat, dVector &c,
                           bool scale = true, bool verbose = false) const;

   /**
    * Run the backward procedure for calculating the prob of an observation.
    * The calculated beta_hat(t,i) is beta^_t(i) from Rabiner 1989 section V.
    * A. Scaling, and c[t] is c_t from the same reference.
    *
    * P(O|mu) is 1 / prod_{t=0..T}(c[t]), but it is not calculated because it
    * is seldom needed.  It should be done in logs whenever needed, to avoid
    * underflow.
    *
    * @pre the model is a straight prob model (not log-prob)
    *
    * @param O          observed output sequence; let T = O.size().
    * @param c          vector of size T+1 calculated by ForwardProcedure().
    * @param beta_hat   this matrix will be resized to T+1 x N and will be
    *                   calculated by this procedure: beta_hat(t, i) is
    *                   beta^_t(i) from Rabiner (1989) section "V." subsection
    *                   "A. Scaling"
    * @param verbose    print a lot of stuff to cerr if true
    */
   void BackwardProcedure(const uintVector &O, const dVector& c,
                          dMatrix &beta_hat,
                          bool verbose = false) const;

   /**
    * Run the count step (Expectation) of the EM, aka Baum-Welch, aka Forward
    * Backward, algorithm.  Again, see Manning+Schuetze or Rabiner for details.
    *
    * This procedure does the counting but not the normalization.  The user
    * should apply any tying of parameters and/or apply these counts to any
    * larger HMM this sub-HMM is a part of, and perform normalization
    * themselves afterwards.  Alternatively, the Maximize() method below can be
    * called if this HMM is complete and has no tied variables.
    *
    * @pre the model is a straight prob model (not log-prob)
    *
    * @param O          observed output sequence; let T = O.size().
    * @param alpha_hat  calculated by ForwardProcedure()
    * @param beta_hat   calculated by BackwardProcedure()
    * @param Pi_counts  this vector will be resized to N; Pi_counts(i) will
    *                   contain the expected frequency in state i at time t=0.
    * @param A_counts   this matrix will be resized to N x N; A_counts(i,j)
    *                   will contain the expected number of transitions from
    *                   state i to j in O.
    * @param B_counts   In an arc emission HMM, this vector of matrices will be
    *                   resized to N x N x M; B_counts(i,j,k) will contain the
    *                   expected number of transitions from i to j in O with k
    *                   observed.
    *                   In a state emission HMM, this vector of matrices will
    *                   be resized to 1 x N x M; B_counts(0,j,k) will contain
    *                   the expected number of transitions into j in O with k
    *                   observed.
    * @param verbose    print a lot of stuff to cerr if true
    */
   void BWCountExpectation(const uintVector &O, const dMatrix &alpha_hat,
                           const dMatrix &beta_hat,
                           dVector &Pi_counts, dMatrix &A_counts,
                           vector<dMatrix> &B_counts,
                           bool verbose = false) const;

   /**
    * Run the whole forward-backward count procedure for Baum-Welch.
    *
    * @pre the model is a straight prob model (not log-prob)
    *
    * @param O          observed output sequence; let T = O.size().
    * @param Pi_counts  this vector will be resized to N; Pi_counts(i) will
    *                   contain the expected frequency in state i at time t=0.
    * @param A_counts   this matrix will be resized to N x N; A_counts(i,j)
    *                   will contain the expected number of transitions from
    *                   state i to j in O.
    * @param B_counts   In an arc emission HMM, this vector of matrices will be
    *                   resized to N x N x M; B_counts(i,j,k) will contain the
    *                   expected number of transitions from i to j in O with k
    *                   observed.
    *                   In a state emission HMM, this vector of matrices will
    *                   be resized to 1 x N x M; B_counts(0,j,k) will contain
    *                   the expected number of transitions into j in O with k
    *                   observed.
    * @param verbose    print a lot of stuff to cerr if true
    * @return log(prob(O|mu))
    */
   double BWForwardBackwardCount(const uintVector &O, dVector &Pi_counts,
                                 dMatrix &A_counts, vector<dMatrix> &B_counts,
                                 bool verbose = false) const;

   /**
    * Apply the maximization step (aka normalization, aka reestimation) of the
    * EM, aka Baum-Welch, aka Forward Backward, algorithm.
    *
    * Modifies this HMM in place.
    *
    * If this HMM is complete and has no tied variables, this procedure can be
    * called after CountExpectation() to do the reestimation step.  It simply
    * normalizes all counts provided and updates Pi, A and B accordingly.
    *
    * If your HMM has tied variable, or if this is a subset of a larger HMM,
    * you should do this step yourself in a routine that knows the
    * peculiarities of your model.
    *
    * @pre the model is a straight prob model (not log-prob)
    *
    * @param Pi_counts  Expectation counts for Pi (size N)
    * @param A_counts   Expectation counts for A (size N x N)
    * @param B_counts   Expectation counts for B (size N x N x M for an
    *                   arc emission HMM, 1 x N x M for a state emission HMM)
    * @param verbose    show progress
    */
   void BWReestimate(const dVector &Pi_counts, const dMatrix &A_counts,
                     const vector<dMatrix> &B_counts, bool verbose = false);


}; // HMM

} // Portage


#endif // __HMM_H__
