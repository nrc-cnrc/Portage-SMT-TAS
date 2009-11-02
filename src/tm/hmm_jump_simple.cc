/**
 * @author Eric Joanis
 * @file hmm_jump_simple.cc  Implementation of the simplest jump strategy.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */


#include "portage_defs.h"
#include "hmm_jump_simple.h"
#include "binio.h"
#include "errors.h"
#include "str_utils.h"

using namespace Portage;

double HMMJumpSimple::jump_p(Uint i_prime, Uint i, Uint I) const {
   assert(i <= I);
   assert(i_prime <= I);
   const int delta(int(i)-int(i_prime));
   if ( delta < 0 ) {
      if ( max_jump && (-delta) >= int(max_jump) ) {
         if ( max_jump > backward_jump_p.size() )
            return 0.0;
         else if ( i == 0 )
            // jump back to start state isn't allowed: we always return 0.
            return 0.0;
         else {
            assert(i_prime > max_jump);
            return backward_jump_p[max_jump-1] / (int(i_prime)-int(max_jump));
         }
      } else {
         Uint i(-delta - 1);
         if ( i >= backward_jump_p.size() )
            return 0.0;
         else
            return backward_jump_p[i];
      }
   }
   else {
      if ( max_jump && delta >= int(max_jump) ) {
         if ( max_jump + 1 > forward_jump_p.size() )
            return 0.0;
         else {
            assert(I + 1 > i_prime + max_jump);
            return forward_jump_p[max_jump] /
                   (I + 1 - i_prime - max_jump);
         }
      } else {
         Uint i(delta);
         if ( i >= forward_jump_p.size() )
            return 0.0;
         else
            return forward_jump_p[i];
      }
   }
} // jump_p()

double& HMMJumpSimple::jump_count(Uint i_prime, Uint i, Uint I) {
   const int delta(int(i) - int(i_prime));
   if ( delta < 0 ) {
      if ( max_jump && (-delta) >= int(max_jump) ) {
         if ( max_jump > backward_jump_counts.size() )
            backward_jump_counts.resize(max_jump, 0.0);
         return backward_jump_counts[max_jump-1];
      } else {
         Uint i(-delta - 1);
         if ( i >= backward_jump_counts.size() )
            backward_jump_counts.resize(Uint(1.5*(i+1)), 0.0);
         return backward_jump_counts[i];
      }
   } else {
      if ( max_jump && delta >= int(max_jump) ) {
         if ( max_jump + 1 > forward_jump_counts.size() )
            forward_jump_counts.resize(max_jump+1, 0.0);
         return forward_jump_counts[max_jump];
      } else {
         Uint i(delta);
         if ( i >= forward_jump_counts.size() )
            forward_jump_counts.resize(Uint(1.5*(i+1)), 0.0);
         return forward_jump_counts[i];
      }
   }
} // jump_count()

static const char* const subversion1_0b_magic_string =
   "Version v1.0b with extra parms: uniform_p0, anchor, max_jump";

void HMMJumpSimple::read(istream& in, const char* stream_name) {
   // Version 1.0 - initial version of the HMM dist parameter file, without
   // the additional parameters we added to replicate Liang et al, 2006.
   uniform_p0 = 0.0;
   anchor = false;
   max_jump = false;

   Uint backward_size, forward_size;
   in >> backward_size >> forward_size >> p_zero >> alpha >> lambda;
   backward_jump_p.reserve(backward_size);
   forward_jump_p.reserve(forward_size);

   string line;
   if ( !getline(in, line) )
      error(ETFatal, "No jump parameters in %s", stream_name);
   if ( line != "" )
      error(ETFatal, "Extraneous stuff on the second line in %s", stream_name);
   if ( !getline(in, line) )
      error(ETFatal, "Missing backward jump parameter line from %s",
            stream_name);
   // v1.0b adds a few extra parameters and is flagged by a magic line here
   if ( line == subversion1_0b_magic_string ){
      in >> uniform_p0 >> anchor >> max_jump;
      if ( !getline(in, line) )
         error(ETFatal, "No jump parameters after extra parameters in %s",
               stream_name);
      if ( line != "" )
         error(ETFatal, "Extraneous stuff on the fourth line in %s",
               stream_name);
      if ( !getline(in, line) )
         error(ETFatal, "Missing backward jump parameter line after extra "
               "parameters in %s", stream_name);
   }

   // This line says "jump counts", but really means "jump parameters".
   if ( line == "Binary HMM jump counts v1.0" ) {
      using namespace BinIO;
      readbin(in, backward_jump_p);
      readbin(in, forward_jump_p);
      if ( !getline(in, line) )
         error(ETFatal, "Missing footer in %s after binary jump parameters",
               stream_name);
      if ( line != "End binary HMM jump counts v1.0" )
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
         error(ETFatal, "Wrong number of forward jump parameters in %s: expected %d, got %d",
            stream_name, forward_size, forward_jump_p.size());
   }
} // read()

HMMJumpSimple* HMMJumpSimple::Clone() const {
   return new HMMJumpSimple(*this);
}

void HMMJumpSimple::write(ostream& out, bool bin) const {
   int max_back;
   for ( max_back = backward_jump_p.size() - 1; max_back >= 0; --max_back )
      if ( backward_jump_p[max_back] != 0.0 ) break;
   //backward_jump_p.resize(max_back+1);

   int max_for;
   for ( max_for = forward_jump_p.size() - 1; max_for >= 0; --max_for )
      if ( forward_jump_p[max_for] != 0.0 ) break;
   //forward_jump_p.resize(max_for+1);

   out << (max_back+1) << " "
       << (max_for+1) << " "
       << p_zero << " "
       << alpha << " "
       << lambda << endl;
   if ( anchor || max_jump || uniform_p0 )
      out << subversion1_0b_magic_string << endl
          << uniform_p0 << " "
          << anchor << " "
          << max_jump << endl;
   if ( false && bin ) {
      using namespace BinIO;
      out << "Binary HMM jump counts v1.0" << endl;
      writebin(out, backward_jump_p);
      writebin(out, forward_jump_p);
      out << "End binary HMM jump counts v1.0" << endl;
   } else {
      out << joini(backward_jump_p.begin(), backward_jump_p.begin()+max_back+1)
          << endl;
      out << joini(forward_jump_p.begin(), forward_jump_p.begin()+max_for+1)
          << endl;
   }
} // write()

void HMMJumpSimple::writeBinCountsImpl(ostream& os) const {
   using namespace BinIO;
   writebin(os, forward_jump_counts);
   writebin(os, backward_jump_counts);
}

void HMMJumpSimple::readAddBinCountsImpl(istream& is, const char* stream_name) {
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
}

bool HMMJumpSimple::hasSameCounts(const HMMJumpStrategy* s) {
   const HMMJumpSimple* o = dynamic_cast<const HMMJumpSimple*>(s);
   return o != NULL &&
          o->forward_jump_counts == forward_jump_counts &&
          o->backward_jump_counts == backward_jump_counts;
}

void HMMJumpSimple::initCounts(const TTable& tt) {
   fill(forward_jump_counts.begin(), forward_jump_counts.end(), 0.0);
   fill(backward_jump_counts.begin(), backward_jump_counts.end(), 0.0);
}

void HMMJumpSimple::countJumps(const dMatrix& A_counts,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I
) {
   for ( Uint i(0); i <= I; ++i ) {
      for ( Uint j(0); j <= I; ++j ) {
         jump_count(i, j, I) +=
            // Transitions between regular alignment states
            A_counts(i,j) +
            // Transitions from NULL alignment to regular alignment
            A_counts(i + I+1, j);

         // Transitions to NULL alignments are not counted, since p_zero is
         // held constant.  Thus, A_counts(*, j + I+1) are ignored.
      }
   }
}

void HMMJumpSimple::estimate() {
   forward_jump_p = forward_jump_counts;
   backward_jump_p = backward_jump_counts;
}

void HMMJumpSimple::fillHMMJumpProbs(HMM* hmm,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I
) const {
   assert(hmm);

   double p0 = fillDefaultHMMJumpProbs(hmm, I);

   // A: see Och+Ney sct 2.1.1, eqns 13-16
   for ( Uint i = 0; i <= I; ++i ) {
      if ( anchor && i == I ) continue; // handled outside strategy

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
      // Setting alpha and/or lambda to 0 disables the corresponding smoothing
      // method.
      double sum(0.0);
      for ( Uint j = 1; j <= I; ++j )
         sum += jump_p(i,j,I) + lambda;
      for ( Uint j = 1; j <= I; ++j ) {
         hmm->A(i + I+1, j) =
         hmm->A(i,j) = (1.0-p0) * alpha_smooth(jump_p(i,j,I)+lambda, sum, I);
      }
   }
} // fillHMMJumpProbs()

