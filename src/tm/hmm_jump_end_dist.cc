/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_end_dist.cc Implementation of the jump strategy with distinct
 *                            end distributions.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "portage_defs.h"
#include "hmm_jump_end_dist.h"
#include "binio.h"
#include "errors.h"
#include "str_utils.h"

using namespace Portage;

double HMMJumpEndDist::jump_p(Uint i_prime, Uint i, Uint I) const {
   assert(i <= I);
   assert(i_prime <= I);
   const int delta(int(i)-int(i_prime));
   if ( anchor && i == I ) {
      assert(delta >= 0);
      if ( max_jump && delta >= int(max_jump) ) {
         // the large jumps are binned, and this jump is a large jump, so
         // divide its mass among possible large jumps
         if ( max_jump > final_jump_p.size() )
            return 0.0;
         else {
            return final_jump_p[max_jump-1] / (I + 1 - max_jump);
         }
      }
      else if ( delta > int(final_jump_p.size()) )
         return 0.0; // no observed counts for this jump
      else if ( delta == 0 )
         return 0.0; // from final state back to final state is not allowed
      else
         return final_jump_p[delta-1];
   }
   else if ( i_prime == 0 && delta != 0 ) {
      assert(delta > 0);
      if ( max_jump && delta >= int(max_jump) ) {
         // the large jumps are binned, and this jump is a large jump, so
         // divide its mass among possible large jumps
         if ( max_jump > init_jump_p.size() ) {
            return 0.0;
         } else {
            const int true_I = anchor ? (I-1) : I;
            return init_jump_p[max_jump - 1] / (true_I + 1 - int(max_jump));
         }
      }
      else if ( delta > int(init_jump_p.size()) )
         return 0.0; // no observed counts for this jump
      else
         return init_jump_p[delta-1];
   }
   else
      return HMMJumpSimple::jump_p(i_prime, i, anchor? (I-1) : I);
} // jump_p()

double& HMMJumpEndDist::jump_count(Uint i_prime, Uint i, Uint I) {
   assert(i <= I);
   assert(i_prime <= I);
   const int delta(int(i) - int(i_prime));
   if ( anchor && i == I && delta != 0 ) {
      // Handle the special distribution to the end anchor

      // excluded special case: delta == 0 when i == I represents a disallowed
      // jump, for which we therefore store no count - let it go to the regular
      // case of delta == 0 not at an end, where a parameter variable exists,
      // which will simply receive +0.0 count since the HMM shouldn't be able
      // to give it more weight than that.
      assert(delta > 0);
      if ( max_jump && delta >= int(max_jump) ) {
         if ( max_jump > final_jump_counts.size() )
            final_jump_counts.resize(max_jump, 0.0);
         return final_jump_counts[max_jump-1];
      } else {
         if ( delta > int(final_jump_counts.size()) )
            final_jump_counts.resize(Uint(1.5*(delta+1)), 0.0);
         return final_jump_counts[delta-1];
      }
   }
   else if ( i_prime == 0 && delta != 0 ) {
      // Handle the special distribution from the start anchor, also excluding
      // the impossible special case where delta == 0.
      if ( max_jump && delta >= int(max_jump) ) {
         if ( max_jump > init_jump_counts.size() )
            init_jump_counts.resize(max_jump, 0.0);
         return init_jump_counts[max_jump-1];
      } else {
         if ( delta > int(init_jump_counts.size()) )
            init_jump_counts.resize(Uint(1.5*(delta+1)), 0.0);
         return init_jump_counts[delta-1];
      }
   }
   else
      return HMMJumpSimple::jump_count(i_prime, i, I);
} // jump_count()

void HMMJumpEndDist::read(istream& in, const char* stream_name) {
   // Version 1.1 - extends v1.0 by adding the parameters required to
   // replicate Liang et al, 2006:  final state anchor (added previously in
   // our code, but now also in the model file itself), p0 value depending
   // on the length of the source sentence (uniform_p0), distinct
   // distributions for the initial and final jumps, and binning of long
   // jumps.
   Uint backward_size, forward_size, init_size, final_size;
   bool end_dist;
   in >> backward_size >> forward_size >> init_size >> final_size
      >> p_zero >> uniform_p0 >> alpha >> lambda
      >> anchor >> end_dist >> max_jump;
   if ( !end_dist )
      error(ETFatal, "HMM model %s is in an obsolete format, please ask Eric to convert it.", stream_name);
   backward_jump_p.reserve(backward_size);
   forward_jump_p.reserve(forward_size);
   init_jump_p.reserve(init_size);
   final_jump_p.reserve(final_size);

   string line;
   if ( !getline(in, line) )
      error(ETFatal, "No jump parameters in %s", stream_name);
   if ( line != "" )
      error(ETFatal, "Extraneous stuff on the first line in %s", stream_name);
   if ( !getline(in, line) )
      error(ETFatal, "Missing backward jump parameter line from %s",
            stream_name);
   if ( line == "Binary HMM jump parameters v1.1" ) {
      using namespace BinIO;
      readbin(in, backward_jump_p);
      readbin(in, forward_jump_p);
      readbin(in, init_jump_p);
      readbin(in, final_jump_p);
      if ( !getline(in, line) )
         error(ETFatal, "Missing footer in %s after binary jump parameters",
               stream_name);
      if ( line != "End binary HMM jump parameters v1.1" )
         error(ETFatal, "Bad footer in %s after binary jump parameters",
               stream_name);
   } else {
      split(line, backward_jump_p);
      if ( backward_jump_p.size() != backward_size )
         error(ETFatal,
            "Wrong number of backward jump parameters in %s: expected %d, got %d",
            stream_name, backward_size, backward_jump_p.size());

      if ( !getline(in, line) )
         error(ETFatal, "Missing forward jump parameter line from %s",
               stream_name);
      split(line, forward_jump_p);
      if ( forward_jump_p.size() != forward_size )
         error(ETFatal,
            "Wrong number of forward jump parameters in %s: expected %d, got %d",
            stream_name, forward_size, forward_jump_p.size());

      if ( !getline(in, line) )
         error(ETFatal, "Missing initial jump parameter line from %s",
               stream_name);
      split(line, init_jump_p);
      if ( init_jump_p.size() != init_size )
         error(ETFatal,
            "Wrong number of initial jump parameters in %s: expected %d, got %d",
            stream_name, init_size, init_jump_p.size());

      if ( !getline(in, line) )
         error(ETFatal, "Missing final jump parameter line from %s",
               stream_name);
      split(line, final_jump_p);
      if ( final_jump_p.size() != final_size )
         error(ETFatal,
            "Wrong number of final jump parameters in %s: expected %d, got %d",
            stream_name, final_size, final_jump_p.size());
   }
} // read()

HMMJumpEndDist* HMMJumpEndDist::Clone() const {
   return new HMMJumpEndDist(*this);
}

void HMMJumpEndDist::write(ostream& out, bool bin) const {
   int max_back;
   for ( max_back = backward_jump_p.size() - 1; max_back >= 0; --max_back )
      if ( backward_jump_p[max_back] != 0.0 ) break;
   //backward_jump_p.resize(max_back+1);

   int max_for;
   for ( max_for = forward_jump_p.size() - 1; max_for >= 0; --max_for )
      if ( forward_jump_p[max_for] != 0.0 ) break;
   //forward_jump_p.resize(max_for+1);

   int max_init;
   for ( max_init = init_jump_p.size() - 1; max_init >= 0; --max_init )
      if ( init_jump_p[max_init] != 0.0 ) break;
   //init_jump_p.resize(max_init+1);

   int max_final;
   for ( max_final = final_jump_p.size() - 1; max_final >= 0; --max_final )
      if ( final_jump_p[max_final] != 0.0 ) break;
   //final_jump_p.resize(max_final+1);

   out << (max_back+1) << " "
       << (max_for+1) << " "
       << (max_init+1) << " "
       << (max_final+1) << " "
       << p_zero << " "
       << uniform_p0 << " "
       << alpha << " "
       << lambda << " "
       << anchor << " "
       << true << " "
       << max_jump << endl;
   // EJJ Jan 2008.  Disable HMM binary jump parameters: there are few enough
   // of them that the speed-up is trivial, and it's more convenient to be able
   // to read them in plain text.
   if ( false && bin ) {
      using namespace BinIO;
      // v1.0 has the confusing line: "Bin HMM jump counts v1.0" - these are
      // not jump counts, but jump parameters, so we use a more intuitive line
      // for v1.1.
      out << "Binary HMM jump parameters v1.1" << endl;
      writebin(out, backward_jump_p);
      writebin(out, forward_jump_p);
      writebin(out, init_jump_p); // new with v1.1
      writebin(out, final_jump_p); // new with v1.1
      out << "End binary HMM jump parameters v1.1" << endl;
   } else {
      out << joini(backward_jump_p.begin(), backward_jump_p.begin()+max_back+1)
          << endl;
      out << joini(forward_jump_p.begin(), forward_jump_p.begin()+max_for+1)
          << endl;
      // new with v1.1:
      out << joini(init_jump_p.begin(), init_jump_p.begin()+max_init+1)
          << endl;
      out << joini(final_jump_p.begin(), final_jump_p.begin()+max_final+1)
          << endl;
   }
}

void HMMJumpEndDist::writeBinCountsImpl(ostream& os) const {
   using namespace BinIO;
   writebin(os, forward_jump_counts);
   writebin(os, backward_jump_counts);
   writebin(os, init_jump_counts); // new with v1.1
   writebin(os, final_jump_counts); // new with v1.1
}

void HMMJumpEndDist::readAddBinCountsImpl(istream& is, const char* stream_name) {
   using namespace BinIO;

   // Forward jump counts
   vector<double> forward_jump_counts_read;
   readbin(is, forward_jump_counts_read);
   if ( forward_jump_counts_read.size() > forward_jump_counts.size() )
      forward_jump_counts.resize(forward_jump_counts_read.size(), 0.0);
   for ( Uint i(0); i < forward_jump_counts_read.size(); ++i )
      forward_jump_counts[i] += forward_jump_counts_read[i];

   // Backward jump counts
   vector<double> backward_jump_counts_read;
   readbin(is, backward_jump_counts_read);
   if ( backward_jump_counts_read.size() > backward_jump_counts.size() )
      backward_jump_counts.resize(backward_jump_counts_read.size(), 0.0);
   for ( Uint i(0); i < backward_jump_counts_read.size(); ++i )
      backward_jump_counts[i] += backward_jump_counts_read[i];

   // Initial jump counts
   vector<double> init_jump_counts_read;
   readbin(is, init_jump_counts_read);
   if ( init_jump_counts_read.size() > init_jump_counts.size() )
      init_jump_counts.resize(init_jump_counts_read.size(), 0.0);
   for ( Uint i(0); i < init_jump_counts_read.size(); ++i )
      init_jump_counts[i] += init_jump_counts_read[i];

   // Final jump counts
   vector<double> final_jump_counts_read;
   readbin(is, final_jump_counts_read);
   if ( final_jump_counts_read.size() > final_jump_counts.size() )
      final_jump_counts.resize(final_jump_counts_read.size(), 0.0);
   for ( Uint i(0); i < final_jump_counts_read.size(); ++i )
      final_jump_counts[i] += final_jump_counts_read[i];
} // readAddBinCounts()

bool HMMJumpEndDist::hasSameCounts(const HMMJumpStrategy* s) {
   const HMMJumpEndDist* o = dynamic_cast<const HMMJumpEndDist*>(s);
   return o != NULL &&
          o->forward_jump_counts == forward_jump_counts &&
          o->backward_jump_counts == backward_jump_counts &&
          o->init_jump_counts == init_jump_counts &&
          o->final_jump_counts == final_jump_counts;
}

void HMMJumpEndDist::initCounts(const TTable& tt) {
   HMMJumpSimple::initCounts(tt);
   fill(init_jump_counts.begin(), init_jump_counts.end(), 0.0);
   fill(final_jump_counts.begin(), final_jump_counts.end(), 0.0);
}

void HMMJumpEndDist::estimate() {
   HMMJumpSimple::estimate();
   init_jump_p = init_jump_counts;
   final_jump_p = final_jump_counts;
}

void HMMJumpEndDist::fillHMMJumpProbs(
      HMM* hmm,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I
) const {
   assert(hmm);

   double p0 = fillDefaultHMMJumpProbs(hmm, I);

   // Pre-calculate the denominator for normalizing jumps to the end position
   double sum_of_jumps_to_I(0.0);
   if ( anchor ) {
      const double max_delta = min(I, final_jump_p.size());
      for (Uint delta = 1; delta <= max_delta; ++delta)
         sum_of_jumps_to_I += final_jump_p[delta-1];
      sum_of_jumps_to_I += I*lambda;
      //cerr << "SumP: " << sum_of_jumps_to_I << endl;
   }

   // A: see Och+Ney sct 2.1.1, eqns 13-16
   const Uint max_normal_I = anchor ? (I-1) : I;
   for ( Uint i = 0; i <= max_normal_I; ++i ) {

      // Transition to a normal state (1 .. I) using jump_p()
      // Three smoothing methods are applied together:
      //  - linear interpolation with the uniform distribution, with weight
      //    alpha on the uniform, 1-alpha on the model (Och+Ney 2003),
      //  - +lambda smoothing (following Lidstone's law, see Manning+Schuetze
      //    sct 6.2.2, p. 204).
      //  - if max_jump > 0, we follow Liang et al, 2006, and bin together all
      //    jumps >= max_jump, distributing uniformly the probability mass from
      //    the end bin among possible such jumps.  All the required logic for
      //    this smoothing method is done in jump_p().
      // Setting alpha and/or lambda disables the corresponding smoothing
      // method.
      double remaining_prob_mass = 1.0 - p0;
      if ( anchor ) {
         // In this case normalization requires special handling: jumping to I
         // is done according to a distinct distribution, but all jumps coming
         // out of i still have to add to 1.  To solve this, we first set
         // A(i,I) according to the end distribution, and then distribute the
         // rest of the probability mass over the other transitions out of i
         // according to the regular jump parameter values.
         hmm->A(i + I+1, I) =
         hmm->A(i,I) =
            (1.0-p0) * 
               alpha_smooth(jump_p(i,I,I) + lambda, sum_of_jumps_to_I, I);

         // Now we can calculate the distribution of the rest of the jumps, and
         // normalize it over the remainting probability mass.
         remaining_prob_mass -= hmm->A(i,I);
         if ( remaining_prob_mass < 0.0 ) {
            cerr << "source: " << join(tgt_toks) << endl;
            cerr << "target: " << join(src_toks) << endl;
            hmm->write(cerr);
            error(ETFatal,
                  "Remaining prob mass < 0.0: I=%d i=%d p0=%f A(i,I)=%f",
                  I, i, p0, hmm->A(i,I));
         }
      }
      assert (remaining_prob_mass >= 0.0);
      double sum(0.0);
      for ( Uint j = 1; j <= max_normal_I; ++j )
         sum += jump_p(i,j,I) + lambda;
      for ( Uint j = 1; j <= max_normal_I; ++j ) {
         hmm->A(i + I+1, j) =
         hmm->A(i,j) =
            remaining_prob_mass *
               alpha_smooth(jump_p(i,j,I)+lambda, sum, max_normal_I);
      }
   }
} // fillHMMJumpProbs()

