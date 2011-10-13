/**
 * @author Eric Joanis
 * @file hmm_aligner.cc  Implementation of the HMM aligner
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <vector>  // include before portage_defs to avoid parse errors in klockwork
#include "portage_defs.h"
#include "errors.h"
#include "hmm_aligner.h"
#include "tmp_val.h"

using namespace Portage;

HMMAligner::StringVecWithExplicitNull::StringVecWithExplicitNull(
      const vector<string>& toks, bool implicitNull)
: toks_with_null(implicitNull ? storage : toks)
{
   if ( implicitNull ) {
      storage.reserve(toks.size() + 1);
      storage.push_back(nullWord());
      storage.insert(storage.end(), toks.begin(), toks.end());
   } else {
      assert(!toks.empty() && toks[0] == nullWord());
   }
}


HMMAligner::HMMAligner(double p_zero, double uniform_p0,
                       double alpha, double lambda,
                       bool anchor, bool end_dist, Uint max_jump,
                       const char* word_classes_file,
                       double map_tau, double map_min_count)
   : useLiangSymVariant(false)
{
   jump_strategy = HMMJumpStrategy::CreateNew(
         p_zero, uniform_p0, alpha, lambda, anchor, end_dist, max_jump,
         word_classes_file, map_tau, map_min_count);

   cerr << "Created new HMM model with p0: " << p_zero
        << " up0: " << uniform_p0 << " alpha: " << alpha
        << " lambda: " << lambda << " anchor: " << anchor
        << " end-dist: " << end_dist << " max-jump: " << max_jump;
   if (word_classes_file)
      cerr << " using word classes from: " << word_classes_file;
   if (map_tau > 0.0)
      cerr << " using MAP learning with tau: " << map_tau
           << " min-count: " << map_min_count;
   cerr << endl;
}

HMMAligner::HMMAligner(const string& ttable_file, string dontcare,
                       double p_zero, double uniform_p0,
                       double alpha, double lambda,
                       bool anchor, bool end_dist, Uint max_jump,
                       const char* word_classes_file,
                       double map_tau, double map_min_count)
   : IBM1(ttable_file)
   , useLiangSymVariant(false)
{
   jump_strategy = HMMJumpStrategy::CreateNew(
         p_zero, uniform_p0, alpha, lambda, anchor, end_dist, max_jump,
         word_classes_file, map_tau, map_min_count);

   cerr << "Created HMM model with existing IBM1 model and p0: " << p_zero
        << " up0: " << uniform_p0 << " alpha: " << alpha
        << " lambda: " << lambda << " anchor: " << anchor
        << " end-dist: " << end_dist << " max-jump: " << max_jump;
   if (word_classes_file)
      cerr << " using word classes from: " << word_classes_file;
   if (map_tau > 0.0)
      cerr << " using MAP learning with tau: " << map_tau
           << " min-count: " << map_min_count;
   cerr << endl;
}

HMMAligner::HMMAligner(const string& ttable_file,
                       optional<double> p_zero,
                       optional<double> uniform_p0,
                       optional<double> alpha,
                       optional<double> lambda,
                       optional<bool>   anchor,
                       optional<bool>   end_dist,
                       optional<Uint>   max_jump)
   : IBM1(ttable_file)
   , useLiangSymVariant(false)
{
   const string dist_file_name = distParamFileName(ttable_file);
   jump_strategy = HMMJumpStrategy::CreateAndRead(dist_file_name,
         p_zero, uniform_p0, alpha, lambda, anchor, end_dist, max_jump);
}

HMMAligner::HMMAligner(const HMMAligner& that)
   : IBM1(*this)
{
   jump_strategy = that.jump_strategy->Clone();
}

HMMAligner::~HMMAligner() {
   delete jump_strategy;
}

static const bool super_verbose_hmm = getenv("PORTAGE_SUPER_VERBOSE_HMM");

shared_ptr<HMM> HMMAligner::makeHMM(const vector<string>& src_toks_arg,
                                    const vector<string>& tgt_toks,
                                    uintVector &O, double smooth,
                                    double eq_smooth, bool use_null) {
   use_null = use_null || useImplicitNulls;

   StringVecWithExplicitNull src_toks(src_toks_arg, use_null);

   // Variable names:
   //  - I and J are from HMM alignment descriptions (Vogel et al, Och+Ney)
   //    I = length of hidden sequence, not counting the NULL word.
   //    J = length of observed sequence
   //  - N, M are from HMM modeling (Manning+Schuetze, Rabiner)
   //    N = number of states in the HMM = 2 * (I + 1)
   //    M = output alphabet size = J

   assert ( !tgt_toks.empty() );
   assert ( src_toks.size() > 1 );
   assert ( src_toks[0] == nullWord() );

   if ( (src_toks.size() * tgt_toks.size()) > (300*300) ) {
      if ( (src_toks.size() * tgt_toks.size()) > (1000*1000) )
         error(ETWarn, "Very long sentence pair (%d and %d tokens) may take a "
               "very long time to process.", src_toks.size(), tgt_toks.size());
      else
         error(ETWarn, "Long sentence pair (%d and %d tokens) may take a "
               "long time to process.", src_toks.size(), tgt_toks.size());
   }

   Uint J = tgt_toks.size();
   Uint I = src_toks.size() - 1; // -1 because src_toks[0] is the null word.

   // Set up virtual src/tgt end tokens if called for. These act like normal
   // tokens, except that the state corresponding to the virtual tgt end token
   // can produce only the virtual src end symbol, forcing </s> to align to
   // </s>. (This is slightly wasteful, since the null state for the tgt end
   // token is superfluous, but it simplifies the code.)
   bool anchor = jump_strategy->getAnchor();
   if (anchor) {
      ++I;
      ++J;
   }

   // 1 dummy start state
   // + I alignments to words
   // + 1 sequence-initial null alignment
   // + I other null alignments
   Uint N = 2 * (I + 1);

   // Simplification: we ignore the fact that some words might be the same:
   // identical words will simply get the same emission probabilities, and the
   // counts will be merged at the end, so this simplification here doesn't
   // change the final model or the functionning of the EM algorithm.
   Uint M = J; // Size of the subset of the output alphabet we care about.

   HMM* hmm = new HMM(N, M, HMM::state_entry_emit, HMM::regular_probs);

   // Pi: <1.0, 0.0, ...> - so we keep the default initializtion in HMM();
   // State 0 is simply considered to align <s> to <s> (not explicitely
   // represented), i.e., a_0 = 0;

   // A: see Och+Ney sct 2.1.1, eqns 13-16
   // Most of the jump parameters are handled by the HMMJumpStrategy in place.
   jump_strategy->fillHMMJumpProbs(hmm, src_toks, tgt_toks, I);

   if ( ! hmm->checkTransitionDistributions(false, true) ) {
      cerr << "observed: " << join(tgt_toks) << endl;
      cerr << "hidden: " << join(src_toks) << endl;
      hmm->write(cerr);
      error(ETFatal, "Bad HMM");
   }

   // B: from the ttable, just like IBM1/IBM2.
   // state 0 never emits since it is never entered, but it has to have a valid
   // distribution anyway, so (arbitrarily) use <1.0, 0.0, ...>
   hmm->B(0,0) = 1.0;
   for ( Uint k = 1; k < M; ++k ) hmm->B(0,k) = 0.0;

   // states 1 to I for emitting from hidden words 0 .. I-1 =
   // src_toks[1]..src_toks[I]
   for ( Uint i = 1; i <= I; ++i ) {

      // force end state to produce only the end symbol
      if (anchor && i == I) {
         for (Uint k = 0; k < M-1; ++k)
            hmm->B(i,k) = 0.0;
         hmm->B(i,M-1) = 1.0;
         continue;
      }

      const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_toks[i]);
      //double sum(0.0);
      // Loop over observed words = tgt_toks
      for ( Uint k = 0; k < M; ++k ) {
         hmm->B(i,k) = 0.0;
         if (anchor && k == M-1) // ordinary states can't produce end symbol
            continue;
         const Uint tindex = tt.targetIndex(tgt_toks[k]);
         if ( tindex != tt.numTargetWords() ) {
            int offset = tt.targetOffset(tindex, src_distn);
            if ( offset != -1 ) {
               hmm->B(i,k) = src_distn[offset].second;
            }
         }
         if ( hmm->B(i,k) == 0.0 ) {
            if (eq_smooth && src_toks[i] == tgt_toks[k])
               hmm->B(i,k) = eq_smooth;
            else
               hmm->B(i,k) = smooth;
         }
         //sum += hmm->B(i,k);
      }

      /*
        if ( sum == 0.0 )
        error(ETWarn, "No observed words with non-zero prob for hidden word %s",
        src_toks[i].c_str());
        if ( sum > 1.0 )
        error(ETWarn, "Emission probabilities sum to %f for hidden word %s",
        sum, src_toks[i].c_str());
      */
   }
   // states 0 + I+1 to I + I+1 are for null alignments, and they all share the
   // same emission distribution, which depends only on each observed word's
   // null-alignment probability.
   {
      const TTable::SrcDistn& src_distn = tt.getSourceDistn(nullWord());
      //double sum(0.0);
      for ( Uint k = 0; k < M; ++k ) {
         double b_i_k = 0.0;
         const Uint tindex = (anchor && k == M-1) ?
            tt.numTargetWords() : // force zero prob for end symbol
            tt.targetIndex(tgt_toks[k]); // look up p(token|null)
         if ( tindex != tt.numTargetWords() ) {
            const int offset = tt.targetOffset(tindex, src_distn);
            if ( offset != -1 ) b_i_k = src_distn[offset].second;
            //sum += b_i_k;
         }

         // Smoothing for NULL is 10 * smoothing for other words, so default
         // is 1e-09 for NULL smoothing, 1e-10 for regular smoothing, in
         // line with the default pruning parameters.
         //EJJ changed 2 Dec 2008: not such a good idea to favour NULL so much;
         //let the jump parameters determine NULL alignments for OOVs.
         //if ( b_i_k == 0.0 ) b_i_k = 10*smooth;
         if ( b_i_k == 0.0 ) b_i_k = smooth;

         for ( Uint i = 0; i <= I; ++i )
            hmm->B(i + I+1, k) = b_i_k;
         if ( anchor ) hmm->B(I + I+1, k) = 0.0;
      }
      /*
      if ( sum == 0.0 ) error(ETWarn,
         "No observed words with non-zero prob for NULL hidden word");
      */
   }

   if ( ! hmm->checkEmissionDistributions(false, true) ) {
      cerr << "observed: " << join(tgt_toks) << endl;
      cerr << "hidden: " << join(src_toks) << endl;
      hmm->write(cerr);
      error(ETFatal, "Bad HMM");
   }

   if ( super_verbose_hmm ) {
      cerr << "observed: " << join(tgt_toks) << nf_endl;
      cerr << "hidden: " << join(src_toks) << nf_endl;
      hmm->write(cerr);
   }

   // O: observed sequence - since we map tgt_toks[j] to output value j is the
   // output alphabet, O is simply the sequence [0, 1, ..., J-1].
   O.resize(J);
   for ( Uint j(0); j < J; ++j )
      O[j] = j;

   return shared_ptr<HMM>(hmm);
} // makeHMM()

void HMMAligner::initCounts() {
   IBM1::initCounts();
   jump_strategy->initCounts(tt);
}

static const char* split_long_sentences_str =
   getenv("PORTAGE_HMM_SPLIT_LONG_SENTENCES");
static const Uint split_long_sentences =
   split_long_sentences_str ? conv<Uint>(split_long_sentences_str) : 200;

static Uint splitInEvenChunks(
   const vector<string>& src_toks,
   const vector<string>& tgt_toks,
   vector<vector<string> >& src_toks_v,
   vector<vector<string> >& tgt_toks_v
)
{
   if ( split_long_sentences == 0 ) return 1;

   Uint src_size = src_toks.size() - 1;
   Uint tgt_size = tgt_toks.size();
   Uint src_chunks = (src_size-1) / split_long_sentences + 1;
   Uint tgt_chunks = (tgt_size-1) / split_long_sentences + 1;
   Uint chunks = max(src_chunks, tgt_chunks);
   if ( src_size < chunks ) chunks = src_size;
   if ( tgt_size < chunks ) chunks = tgt_size;
   if ( chunks <= 1 ) {
      return 1;
   }
   error(ETWarn, "HMM module splitting long sentence pair (%u/%u) "
         "into %u chunks", src_size, tgt_size, chunks);
   src_toks_v.resize(chunks);
   tgt_toks_v.resize(chunks);
   Uint src_end(1), tgt_end(0);
   for ( Uint i = 0; i < chunks; ++i ) {
      Uint src_begin = src_end;
      src_end = 1 + ((i+1) * src_size) / chunks;
      src_toks_v[i].clear();
      src_toks_v[i].reserve(src_end-src_begin+1);
      src_toks_v[i].push_back(src_toks[0]); // NULL word
      for ( Uint j = src_begin; j < src_end; ++j )
         src_toks_v[i].push_back(src_toks[j]);

      Uint tgt_begin = tgt_end;
      tgt_end = ((i+1) * tgt_size) / chunks;
      tgt_toks_v[i].clear();
      tgt_toks_v[i].reserve(tgt_end-tgt_begin);
      for ( Uint j = tgt_begin; j < tgt_end; ++j )
         tgt_toks_v[i].push_back(tgt_toks[j]);
      //cerr << " src size " << src_toks_v[i].size();
      //cerr << " tgt size " << tgt_toks_v[i].size();
      //cerr << endl;
   }
   return chunks;
}

void HMMAligner::count(const vector<string>& src_toks_arg,
                       const vector<string>& tgt_toks,
                       bool use_null)
{
   assert(!useImplicitNulls);

   StringVecWithExplicitNull src_toks(src_toks_arg, use_null);

   if ( split_long_sentences && 
        ( src_toks.size() > split_long_sentences + 1 ||
          tgt_toks.size() > split_long_sentences ) ) {
      vector<vector<string> > src_toks_v;
      vector<vector<string> > tgt_toks_v;
      const Uint chunks = splitInEvenChunks(src_toks, tgt_toks, src_toks_v, tgt_toks_v);
      if ( chunks > 1 ) {
         for ( Uint c = 0; c < chunks; ++c )
            count(src_toks_v[c], tgt_toks_v[c], false);
         return;
      }
   }

   // Variable names:
   //  - I and J are from HMM alignment descriptions (Vogel et al, Och+Ney)
   //    I = length of hidden sequence, not counting the NULL word.
   //    J = length of observed sequence
   //  - N, M are from HMM modeling (Manning+Schuetze, Rabiner)
   //    N = number of states in the HMM = 2 * (I + 1)
   //    M = output alphabet size = J

   assert(!src_toks.empty() && src_toks[0] == nullWord());

   if ( tgt_toks.empty() ) {
      //error(ETWarn, "empty observed sequence");
      return;
   }
   if ( src_toks.size() <= 1 ) {
      //error(ETWarn, "empty hidden sequence");
      return;
   }

   //Uint J = tgt_toks.size();
   Uint I = src_toks.size() - 1; // -1 because src_toks[0] is the null word.

   uintVector O;
   //shared_ptr<HMM> hmm(makeHMM(src_toks, tgt_toks, O));
   shared_ptr<HMM> hmm(makeHMM(src_toks, tgt_toks, O, 1e-10));
   //Uint N = hmm->getN(); // N = 2 * (I + 1)
   Uint M = hmm->getM(); // M = J;

   bool anchor = jump_strategy->getAnchor();
   if (anchor) {
      ++I;                      // include virtual end state
      --M;                      // but exclude virtual end symbol
   }

   dVector Pi_counts; // dummy, since Pi = <1.0, 0.0, ...>
   dMatrix A_counts;
   vector<dMatrix> B_counts;
   double cur_logprob =
      hmm->BWForwardBackwardCount(O, Pi_counts, A_counts, B_counts);

   if ( isfinite(cur_logprob) ) {
      logprob += cur_logprob;
      num_toks += tgt_toks.size();
   } else {
      string s = join(src_toks);
      string t = join(tgt_toks);
      error(ETWarn, "BWForwardBackwardCount logprob = %g, skipping line:\n%s\n%s",
            cur_logprob, s.c_str(), t.c_str());
      hmm->write(cerr);
      return;
   }

   // Tally the counts into the global model counts.
   // Transitions: update the jump counts
   jump_strategy->countJumps(A_counts, src_toks, tgt_toks, I);

   // Emissions: update the IBM1 counts.
   // Loop over hidden sequence = src_toks
   for ( Uint i = 1; i <= I; ++i ) {
      if (anchor && i == I)     // don't count output from virtual end state
         continue;
      const Uint src_index = tt.sourceIndex(src_toks[i]);
      const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
      // Loop over observed sequence = tgt_toks
      for ( Uint k = 0; k < M; ++k ) {
         const Uint tindex = tt.targetIndex(tgt_toks[k]);
         if ( tindex != tt.numTargetWords() ) {
            const int offset = tt.targetOffset(tindex, src_distn);
            if ( offset != -1 )
               counts[src_index][offset] += B_counts[0](i,k);
         }
      }
   }
   // NULL alignments: emissions from states I+1 to 2I+1
   // (if anchoring, we don't need to exclude output from the virtual end state
   // here, since these counts will be zero anyway)
   {
      const Uint src_index = tt.sourceIndex(nullWord());
      const TTable::SrcDistn& src_distn = tt.getSourceDistn(src_index);
      for ( Uint k = 0; k < M; ++k ) {
         Uint tindex = tt.targetIndex(tgt_toks[k]);
         if ( tindex != tt.numTargetWords() ) {
            int offset = tt.targetOffset(tindex, src_distn);
            if ( offset != -1 )
               for ( Uint i = 0; i <= I; ++i )
                  counts[src_index][offset] += B_counts[0](i + I+1, k);
         }
      }
   }
} // HMMAligner::count()

static const bool debug_count_sym = false;

HMMAligner::count_symmetrized_helper::count_symmetrized_helper(
      HMMAligner* parent,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks)
   : parent(parent)
   , src_toks(src_toks)
   , src_toks_with_null(src_toks, true)
   , tgt_toks(tgt_toks)
{
   // Build the HMM, run the Forward-Backward procedure and calculate posteriors

   // J = tgt_toks.size();
   I = src_toks.size();
   hmm = parent->makeHMM(src_toks, tgt_toks, O, 1e-10, 0.0, true);
   // N = hmm->getN(); // N = 2 * (I+1)
   M = hmm->getM(); // M = J
   if ( parent->jump_strategy->getAnchor() ) {
      ++I;
      --M;
   }
   assert(M == tgt_toks.size());
   cur_logprob = hmm->ForwardProcedure(O, alpha_hat, c, true);
   hmm->BackwardProcedure(O, c, beta_hat);
   hmm->statePosteriors(O, alpha_hat, beta_hat, gamma);

   if ( debug_count_sym ) dump();

   if ( gamma(0,0) != 1 ) {
      cerr << nf_endl << "Gamma(0,0) != 1, aborting" << nf_endl;
      dump();
   }
   assert(gamma(0,0) == 1);
   if ( parent->jump_strategy->getAnchor() ) {
      if ( abs(gamma(M+1,I)-1) >= 0.0001 ) {
         cerr << nf_endl << "Gamma(M+1,I) = " << gamma(M+1,I)
              << " != 1, aborting" << nf_endl;
         dump();
      }
      assert(abs(gamma(M+1,I)-1) < 0.0001);
   }

   /////////////// NULL alignment posteriors for this model
   // Tally NULL alignment probs for each observed word
   null_posteriors.resize(M, 0.0);
   if ( debug_count_sym ) cerr << "null_posteriors: ";
   for ( Uint k = 0; k < M; ++k ) {
      null_posteriors[k] = 0.0;
      for ( Uint i = I+1; i < hmm->getN(); ++i )
         null_posteriors[k] += gamma(k+1, i);
      if ( debug_count_sym ) cerr << null_posteriors[k];
   }
   if ( debug_count_sym ) cerr << nf_endl;
   // Indirectly calculate the null alignment prob in the reverse direction of
   // this model
   null_posteriors_rev.resize(src_toks.size(), 0.0);
   if ( debug_count_sym ) cerr << "null_posteriors_rev: ";
   for ( Uint i = 0; i < src_toks.size(); ++i ) {
      null_posteriors_rev[i] = 1.0;
      for ( Uint k = 0; k < M; ++k )
         null_posteriors_rev[i] *= 1 - gamma(k+1,i+1);
      if ( debug_count_sym ) cerr << null_posteriors_rev[i];
   }
   if ( debug_count_sym ) cerr << nf_endl;
} // HMMAligner::count_symmetrized_helper::count_symmetrized_helper()

void HMMAligner::count_symmetrized_helper::dump()
{
   cerr << nf_endl << "===================================================="
        << nf_endl;
   cerr << "src: " << join(src_toks) << nf_endl;
   cerr << "tgt: " << join(tgt_toks) << nf_endl;
   cerr << "logprob: " << cur_logprob << nf_endl;
   cerr << nf_endl << "HMM Model ================================" << nf_endl;
   hmm->write(cerr);
   cerr << "gamma =================================" << nf_endl;
   cerr << "    <s> " << join(src_toks) << " </s> " << nullWord()
        << "(0) NULL(1) ..." << nf_endl;
   Uint tgt_i = 0;
   for ( dMatrix::iterator1 i(gamma.begin1()), i_end(gamma.end1());
         i != i_end; ++i, ++tgt_i ) {
      if      ( tgt_i == 0 )               cerr << "<s>";
      else if ( tgt_i <= tgt_toks.size() ) cerr << tgt_toks[tgt_i-1];
      else                                 cerr << "</s>";
      for ( dMatrix::iterator2 j(i.begin()), j_end(i.end());
            j != j_end; ++j )
         cerr << " " << *j;
      cerr << nf_endl;
   }
   cerr << endl;
}

void HMMAligner::count_symmetrized_helper::count_expectations(
   const HMMAligner::count_symmetrized_helper& r)
{
   // Non-symmetric A counts are only needed for the variant implementation.
   // They're expensive to calculate, so we don't do them if we don't need to.
   dMatrix A_counts;
   if ( parent->useLiangSymVariant ) {
      // We don't care about non-symmetric Pi and B counts, but they are
      // required for the method call, and don't cost much extra when we
      // already need the A_counts.
      dVector Pi_counts;
      vector<dMatrix> B_counts;
      hmm->BWCountExpectation(O, alpha_hat, beta_hat,
                              Pi_counts, A_counts, B_counts);
   }

   // Jump expectations, based on the product of posterior probabilities
   sym_A_counts.resize(hmm->getN(), hmm->getN(), false);
   sym_A_counts.clear(); // set all values to 0.0
   if ( parent->useLiangSymVariant ) {
      assert(hmm->getM() == O.size());
      dMatrix p_k;
      for ( Uint k(0); k < hmm->getM(); ++k ) {
         hmm->transitionPosteriors(O, alpha_hat, beta_hat, k, p_k);
         for ( Uint i(0); i <= I; ++i ) {
            for ( Uint j(1); j <= I; ++j ) {
               sym_A_counts(i,j) +=
                  (p_k(i,j) + p_k(i + I+1, j))
                  * r.gamma(i,k) * r.gamma(j,k+1);
            }
         }
      }
      if ( debug_count_sym )
         for ( Uint i(0); i <= I; ++i )
            for ( Uint j(1); j <= I; ++j )
               cerr << "i=" << i << "->j=" << j
                    << ": sym_A_counts(i,j) = " << sym_A_counts(i,j) << nf_endl;
   } else {
      for ( Uint i(0); i <= I; ++i ) {
         for ( Uint j(1); j <= I; ++j ) {
            assert(sym_A_counts(i,j) == 0.0);
            if ( debug_count_sym ) cerr << "i=" << i << "->j=" << j
               << ": sym_A_counts(i,j) =";
            for ( Uint k(0); k < hmm->getM(); ++k ) {
               const double sym_A_counts_i_j_term = 
                  (gamma(k,i)+gamma(k,i+I+1)) * gamma(k+1,j)
                  * r.gamma(i,k) * r.gamma(j,k+1);
               if ( debug_count_sym ) cerr << " + " << sym_A_counts_i_j_term;
               sym_A_counts(i,j) += sym_A_counts_i_j_term;
            }
            if ( debug_count_sym ) cerr << " = " << sym_A_counts(i,j) << nf_endl;
         }
      }
   }

   if ( debug_count_sym ) cerr << endl;

   parent->jump_strategy->countJumps(sym_A_counts, src_toks_with_null, tgt_toks, I);

   // Lexical expectations, also based on the product of posteriors
   // loop over hidden sequence
   if ( debug_count_sym ) cerr << "Lexical counts" << nf_endl;
   const bool anchor = parent->jump_strategy->getAnchor();
   for ( Uint i = 0; i < I; ++i ) {
      if ( anchor && i+1 == I )
         continue;
      const Uint src_index = parent->tt.sourceIndex(src_toks[i]);
      const TTable::SrcDistn& src_distn = parent->tt.getSourceDistn(src_index);
      // loop over observed sequence
      for ( Uint k = 0; k < M; ++k ) {
         const Uint tindex = parent->tt.targetIndex(tgt_toks[k]);
         if ( tindex != parent->tt.numTargetWords() ) {
            const int offset = parent->tt.targetOffset(tindex, src_distn);
            if ( offset != -1 ) {
               const double count_term = gamma(k+1, i+1) * r.gamma(i+1, k+1);
               if ( debug_count_sym )
                  cerr << "counts(i=" << i << ",k=" << k << ") += " << count_term << nf_endl;
               parent->counts[src_index][offset] += count_term;
            }
         }
      }
   }
   // null alignment counts
   {
      const Uint src_index = parent->tt.sourceIndex(nullWord());
      const TTable::SrcDistn& src_distn = parent->tt.getSourceDistn(src_index);
      for ( Uint k = 0; k < M; ++k ) {
         const Uint tindex = parent->tt.targetIndex(tgt_toks[k]);
         if ( tindex != parent->tt.numTargetWords() ) {
            const int offset = parent->tt.targetOffset(tindex, src_distn);
            if ( offset != -1 ) {
               const double count_term = null_posteriors[k] * r.null_posteriors_rev[k];
               if ( debug_count_sym )
                  cerr << "counts(NULL,k=" << k << ") += " << count_term << nf_endl;
               parent->counts[src_index][offset] += count_term;
            }
         }
      }
   }

} // HMMAligner::count_symmetrized_helper::count_expectations

void HMMAligner::count_symmetrized(const vector<string>& src_toks,
                                   const vector<string>& tgt_toks,
                                   bool use_null, IBM1* r_ibm1)
{
   assert(!useImplicitNulls);
   assert(use_null);
   HMMAligner* r_hmma = dynamic_cast<HMMAligner*>(r_ibm1);
   assert(r_hmma);

   if ( (! src_toks.empty() && src_toks[0] == nullWord()) ||
        (! tgt_toks.empty() && tgt_toks[0] == nullWord()) )
      error(ETWarn, "Using explicit nulls with HMMAligner::count_symmetrized()"
                    " will have incorrect effects on the models.");

   if ( src_toks.empty() || tgt_toks.empty() ) {
      //error(ETWarn, "Empty hidden or observed sequence");
      return;
   }

   // use splitInEvenChunks() when input is too long.
   if ( split_long_sentences && 
        ( src_toks.size() > split_long_sentences ||
          tgt_toks.size() > split_long_sentences ) ) {
      vector<vector<string> > src_toks_v;
      vector<vector<string> > tgt_toks_v;
      const Uint chunks = splitInEvenChunks(
         StringVecWithExplicitNull(src_toks, use_null), tgt_toks, src_toks_v, tgt_toks_v);
      if ( chunks > 1 ) {
         for ( Uint c = 0; c < chunks; ++c ) {
            const Uint old_src_size = src_toks_v[c].size();
            src_toks_v[c].erase(src_toks_v[c].begin());
            assert(src_toks_v[c].size() + 1 == old_src_size);
            count_symmetrized(src_toks_v[c], tgt_toks_v[c], use_null, r_ibm1);
         }
         return;
      }
   }

   // The first steps, including running the Forward-Backward procedure and
   // getting the various posteriors, are done in the helper's constructor for
   // each direction.
   count_symmetrized_helper f(this, src_toks, tgt_toks);
   count_symmetrized_helper r(r_hmma, tgt_toks, src_toks);

   assert(jump_strategy->getAnchor() == r_hmma->jump_strategy->getAnchor());
   assert(useLiangSymVariant == r_hmma->useLiangSymVariant);

   /////////////// Error checking for both models
   if ( isfinite(f.cur_logprob) && isfinite(r.cur_logprob) ) {
      logprob += f.cur_logprob;
      num_toks += f.tgt_toks.size();
      r_hmma->logprob += r.cur_logprob;
      r_hmma->num_toks += r.tgt_toks.size();
   } else {
      string s = join(src_toks);
      string t = join(tgt_toks);
      error(ETWarn, "HMM logprob = %g, rev logprob = %g, skipping pair:\n%s\n%s",
            f.cur_logprob, f.cur_logprob, s.c_str(), t.c_str());
      f.hmm->write(cerr);
      r.hmm->write(cerr);
      return;
   }

   // Count all jump and lexical expectations
   if ( debug_count_sym )
      cerr << "Counting all expectations for this model" << nf_endl;
   f.count_expectations(r);
   if ( debug_count_sym )
      cerr << "Counting all expectations for reverse model" << nf_endl;
   r.count_expectations(f);

   if ( debug_count_sym ) cerr << endl;

} // HMMAligner::count_symmetrized()

pair<double,Uint> HMMAligner::estimate(double pruning_threshold,
                                       double null_pruning_threshold)
{
   jump_strategy->estimate();
   return IBM1::estimate(pruning_threshold, null_pruning_threshold);
}

void HMMAligner::write(const string& ttable_file, bool bin_ttable) const
{
   IBM1::write(ttable_file, bin_ttable);

   const string dist_file = distParamFileName(ttable_file, true);
   oSafeMagicStream out(dist_file);

   out << jump_strategy->getMagicString() << endl;
   jump_strategy->write(out, bin_ttable);
}

void HMMAligner::writeBinCounts(const string& count_file) const {
   IBM1::writeBinCounts(count_file);
   string hmm_count_file (addExtension(removeZipExtension(count_file), ".hmm"));
   oSafeMagicStream os(hmm_count_file);
   jump_strategy->writeBinCounts(os);
}

void HMMAligner::readAddBinCounts(const string& count_file) {
   IBM1::readAddBinCounts(count_file);
   string hmm_count_file (addExtension(removeZipExtension(count_file), ".hmm"));
   iSafeMagicStream is(hmm_count_file);
   jump_strategy->readAddBinCounts(is, hmm_count_file.c_str());
}

double HMMAligner::pr(const vector<string>& src_toks, const string& tgt_tok,
                      Uint tpos, Uint tlen, vector<double>* probs) {
   error(ETFatal, "HMMAligner::pr() with IBM2 parms cannot be implemented");
   return 0.0;
}

double HMMAligner::pr(const vector<string>& src_toks, const string& tgt_tok,
                      vector<double>* probs) {
   error(ETFatal, "HMMAligner::pr() cannot be implemented");
   return 0.0;
}

double HMMAligner::logpr(const vector<string>& src_toks,
                         const vector<string>& tgt_toks, double smooth) {
   if ( src_toks.empty() && tgt_toks.empty() ) return 0.0;
   if ( src_toks.empty() || tgt_toks.empty() ) return log(smooth);

   if ( split_long_sentences &&
        ( src_toks.size() > split_long_sentences ||
          tgt_toks.size() > split_long_sentences ) ) {
      vector<vector<string> > src_toks_v;
      vector<vector<string> > tgt_toks_v;
      StringVecWithExplicitNull src_toks_n(src_toks, useImplicitNulls);
      const Uint chunks = splitInEvenChunks(src_toks_n, tgt_toks, src_toks_v, tgt_toks_v);
      if ( chunks > 1 ) {
         tmp_val<bool> tmp_null(useImplicitNulls, false);
         double logprob(0.0);
         for ( Uint c = 0; c < chunks; ++c )
            logprob += logpr(src_toks_v[c], tgt_toks_v[c], smooth);
         return logprob;
      }
   }

   uintVector O;
   shared_ptr<HMM> hmm(makeHMM(src_toks, tgt_toks, O, smooth));
   dMatrix alpha_hat;
   dVector c;
   double logprob = hmm->ForwardProcedure(O, alpha_hat, c);
   return logprob;
}

double HMMAligner::viterbi_logpr(const vector<string>& src_toks,
                                 const vector<string>& tgt_toks,
                                 double smooth) {
   if ( src_toks.empty() && tgt_toks.empty() ) return 0.0;
   if ( src_toks.empty() || tgt_toks.empty() ) return log(smooth);

   if ( split_long_sentences &&
        ( src_toks.size() > split_long_sentences ||
          tgt_toks.size() > split_long_sentences ) ) {
      vector<vector<string> > src_toks_v;
      vector<vector<string> > tgt_toks_v;
      StringVecWithExplicitNull src_toks_n(src_toks, useImplicitNulls);
      const Uint chunks = splitInEvenChunks(src_toks_n, tgt_toks, src_toks_v, tgt_toks_v);
      if ( chunks > 1 ) {
         tmp_val<bool> tmp_null(useImplicitNulls, false);
         double logprob(0.0);
         for ( Uint c = 0; c < chunks; ++c )
            logprob += viterbi_logpr(src_toks_v[c], tgt_toks_v[c], smooth);
         return logprob;
      }
   }

   uintVector O;
   shared_ptr<HMM> hmm(makeHMM(src_toks, tgt_toks, O, smooth));
   // One might think doing Viterbi over logs is faster, but the conversion is
   // far more expensive than running Viterbi itself, so it's not worthwhile.
   //hmm->convertToLogModel();
   uintVector X_hat;
   double logprob = hmm->Viterbi(O, X_hat);
   return logprob;
}

double HMMAligner::minlogpr(const vector<string>& src_toks,
                            const string& tgt_tok, double smooth) {
   error(ETFatal, "HMMAligner::minlogpr() N to 1 not implemented yet");
   return 0.0;
}

double HMMAligner::minlogpr(const string& src_toks,
                            const vector<string>& tgt_toks, double smooth) {
   error(ETFatal, "HMMAligner::minlogpr() 1 to N not implemented yet");
   return 0.0;
}

void HMMAligner::align(const vector<string>& src, const vector<string>& tgt,
                       vector<Uint>& tgt_al, bool twist,
                       vector<double>* tgt_al_probs) {
   assert(!twist);
   assert(tgt_al_probs == NULL);

   if ( tgt.empty() || src.empty() || (!useImplicitNulls && src.size() == 1) ) {
      tgt_al.resize(tgt.size(), 0);
      return;
   }

   if ( split_long_sentences &&
        ( src.size() > split_long_sentences ||
          tgt.size() > split_long_sentences ) ) {
      vector<vector<string> > src_toks_v;
      vector<vector<string> > tgt_toks_v;
      StringVecWithExplicitNull src_toks_n(src, useImplicitNulls);
      const Uint chunks = splitInEvenChunks(src_toks_n, tgt, src_toks_v, tgt_toks_v);
      if ( chunks > 1 ) {
         tgt_al.clear();
         tgt_al.reserve(tgt.size());
         const Uint null_position = useImplicitNulls ? src.size() : 0;
         Uint src_offset_c = useImplicitNulls ? 0 : 1;
         tmp_val<bool> tmp_null(useImplicitNulls, false);
         for ( Uint c = 0; c < chunks; ++c ) {
            vector<Uint> tgt_al_c;
            align(src_toks_v[c], tgt_toks_v[c], tgt_al_c, twist, NULL);
            assert(tgt_al_c.size() == tgt_toks_v[c].size());
            for ( Uint j = 0; j < tgt_al_c.size(); ++j ) {
               if ( tgt_al_c[j] == 0 )
                  tgt_al.push_back(null_position);
               else
                  tgt_al.push_back(tgt_al_c[j] - 1 + src_offset_c);
            }
            src_offset_c += src_toks_v[c].size() - 1;
         }
         assert(tgt_al.size() == tgt.size());
         return;
      }
   }

   Uint J = tgt.size();
   Uint I;
   if ( useImplicitNulls ) {
      I = src.size();
   } else {
      assert(src[0] == nullWord());
      I = src.size() - 1; // -1 because src[0] is the null word.
   }

   uintVector O;
   shared_ptr<HMM> hmm(makeHMM(src, tgt, O, 1e-10, 0.1));
   // was intended as an optimization, but is in fact slower:
   //hmm->convertToLogModel();

   uintVector X_hat;
   hmm->Viterbi(O, X_hat);
   assert(X_hat.size() == J+1 ||
          (jump_strategy->getAnchor() && X_hat.size() == J+2));

   tgt_al.resize(J);
   for ( Uint j = 0; j < J; ++j ) {
//      assert(X_hat[j+1] != 0);
      if (X_hat[j+1] == 0) {
         error(ETWarn, "no alignment for word <%s> in pair:\n%s\n%s",
                       tgt[j].c_str(), join(src).c_str(), join(tgt).c_str());
         X_hat[j+1] = I+1; // force null align
      }
      if ( X_hat[j+1] > I )
         tgt_al[j] = useImplicitNulls ? src.size()     : 0;
      else
         tgt_al[j] = useImplicitNulls ? X_hat[j+1] - 1 : X_hat[j+1];
   }
} // HMMAligner::align()

double HMMAligner::linkPosteriors(const vector<string>& src,
                                  const vector<string>& tgt,
                                  vector<vector<double> >& posteriors)
{
   if ( tgt.empty() ) {
      posteriors.resize(0);
      return 0.0;
   }
   if ( src.empty() || (!useImplicitNulls && src.size() == 1) ) {
      posteriors.assign(tgt.size(), vector<double>(1, 1.0));
      return tgt.size()*log(1e-10);
   }

   const Uint J = tgt.size();
   // real_I is the number of real hidden words to align to
   const Uint real_I = src.size() + (useImplicitNulls ? 0 : -1);
   // I includes the dummy anchor, if any, so I+1 is the value to use for
   // distinguising NULL alignment states from regular states.
   const Uint I = real_I + (jump_strategy->getAnchor() ? 1 : 0);

   assert(useImplicitNulls || src[0] == nullWord());

   // resize and initialize posteriors with 0.0 in each cell.
   posteriors.assign(tgt.size(), vector<double>(real_I+1, 0.0));

   if ( split_long_sentences &&
        ( src.size() > split_long_sentences ||
          tgt.size() > split_long_sentences ) ) {
      vector<vector<string> > src_toks_v;
      vector<vector<string> > tgt_toks_v;
      StringVecWithExplicitNull src_toks_n(src, useImplicitNulls);
      const Uint chunks = splitInEvenChunks(src_toks_n, tgt, src_toks_v, tgt_toks_v);
      if ( chunks > 1 ) {
         const Uint null_position = useImplicitNulls ? src.size() : 0;
         Uint src_offset_c = useImplicitNulls ? 0 : 1;
         Uint tgt_offset_c = 0;
         tmp_val<bool> tmp_null(useImplicitNulls, false);
         double logprob(0);
         for ( Uint c = 0; c < chunks; ++c ) {
            vector<vector<double> > posteriors_c;
            logprob += linkPosteriors(src_toks_v[c], tgt_toks_v[c], posteriors_c);
            assert(posteriors_c.size() == tgt_toks_v[c].size());
            assert(posteriors_c[0].size() == src_toks_v[c].size());
            for ( Uint j = 0; j < posteriors_c.size(); ++j ) {
               posteriors[j+tgt_offset_c][null_position] = posteriors_c[j][0];
               for ( Uint i = 1; i < posteriors_c[j].size(); ++i )
                  posteriors[j+tgt_offset_c][i-1+src_offset_c] = posteriors_c[j][i];
            }
            src_offset_c += src_toks_v[c].size() - 1;
            tgt_offset_c += tgt_toks_v[c].size();
         }
         assert(src_offset_c == src.size());
         assert(tgt_offset_c == tgt.size());
         return logprob;
      }
   }

   uintVector O;
   shared_ptr<HMM> hmm(makeHMM(src, tgt, O, 1e-10, 0.1));
   assert(hmm->getN() == 2 * (I + 1));

   dMatrix gamma;
   double log_pr = hmm->statePosteriors(O, gamma);

   // resize and initialize posteriors with 0.0 in each cell.
   posteriors.assign(tgt.size(), vector<double>(real_I+1, 0.0));

   const Uint nullIndex = useImplicitNulls ? src.size() : 0;
   const Uint srcOffset = useImplicitNulls ? 0 : 1;

   for ( Uint j = 0; j < J; ++j ) {
      double sum(0.0);
      for ( Uint i = 0; i < real_I; ++i )
         sum += posteriors[j][i + srcOffset] = gamma(j+1, i+1);
      double nullProb(0.0);
      for ( Uint i = I+1; i < hmm->getN(); ++i )
         nullProb += gamma(j+1, i);
      posteriors[j][nullIndex] = nullProb;

      if ( abs(1.0 - sum - nullProb) > 0.0001 ) {
         cerr << "observed: " << join(tgt) << endl;
         cerr << "hidden: " << join(src) << endl;
         hmm->write(cerr);
         hmm->statePosteriors(O, gamma, true);
         error(ETWarn, "Normalization error in HMMAligner::linkPosteriors()");
      }
   }

   return log_pr;
} // HMMAligner::linkPosteriors()



void HMMAligner::testReadWriteBinCounts(const string& count_file) const {
   IBM1::testReadWriteBinCounts(count_file);
   cerr << "Checking HMM read/writeBinCounts on " << count_file << endl;
   writeBinCounts(count_file);
   HMMAligner copy(*this);
   copy.initCounts();
   copy.readAddBinCounts(count_file);
   if ( ! jump_strategy->hasSameCounts(copy.jump_strategy) )
      error(ETWarn, "HMM counts changed");
   delete copy.jump_strategy;
   cerr << "Done checking HMM read/writeBinCounts on " << count_file << endl;
}
