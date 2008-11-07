/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_strategy.cc  Implementation for the few methods provided by
 *                             HMMJumpStrategy.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "portage_defs.h"
#include "errors.h"
#include "file_utils.h"
#include "hmm_jump_strategy.h"
#include "hmm_jump_simple.h"
#include "hmm_jump_end_dist.h"
#include "hmm_jump_classes.h"
#include "hmm_jump_map.h"

using namespace Portage;

HMMJumpStrategy::HMMJumpStrategy()
   : p_zero(0.05)
   , uniform_p0(0.0)
   , alpha(0.01)
   , lambda(0.0)
   , anchor(false)
   , max_jump(0)
{}

void HMMJumpStrategy::init_base_class(
      double _p_zero, double _uniform_p0, double _alpha,
      double _lambda, bool _anchor, Uint _max_jump
) {
   p_zero     = _p_zero;
   uniform_p0 = _uniform_p0;
   alpha      = _alpha;
   lambda     = _lambda;
   anchor     = _anchor;
   max_jump   = _max_jump;
}

HMMJumpStrategy* HMMJumpStrategy::CreateNew(
      double p_zero, double uniform_p0,
      double alpha, double lambda,
      bool anchor, bool end_dist, Uint max_jump,
      const char* word_classes_file,
      double map_tau, double map_min_count
) {
   if ( alpha >= 1 ) error(ETFatal, "HMMAligner: alpha must be < 1");
   if ( alpha < 0 ) error(ETFatal, "HMMAligner: alpha must be >= 0");
   if ( p_zero >= 1 ) error(ETFatal, "HMMAligner: p_zero must be < 1");

   HMMJumpStrategy* jump_strategy;
   if ( word_classes_file )
      jump_strategy = new HMMJumpClasses(word_classes_file, end_dist);
   else if ( end_dist )
      jump_strategy = new HMMJumpEndDist();
   else
      jump_strategy = new HMMJumpSimple();
   jump_strategy->init_base_class(p_zero, uniform_p0, alpha,
                                  lambda, anchor, max_jump);

   if ( map_tau > 0.0 ) {
      HMMJumpStrategy* prior = jump_strategy;
      jump_strategy = new HMMJumpMAP(map_tau, map_min_count, end_dist, prior);
      jump_strategy->init_base_class(p_zero, uniform_p0, alpha,
                                     lambda, anchor, max_jump);
   }

   return jump_strategy;
} // CreateNew()

HMMJumpStrategy* HMMJumpStrategy::CreateAndRead(
      const string& dist_file_name,
      optional<double> p_zero,
      optional<double> uniform_p0,
      optional<double> alpha,
      optional<double> lambda,
      optional<bool>   anchor,
      optional<bool>   end_dist,
      optional<Uint>   max_jump
) {
   iSafeMagicStream dist_file(dist_file_name);
   return CreateAndRead(dist_file, dist_file_name.c_str(), p_zero, uniform_p0,
         alpha, lambda, anchor, end_dist, max_jump);
} // CreateAndRead()

HMMJumpStrategy* HMMJumpStrategy::CreateAndRead(
      istream& dist_file,
      const char* dist_file_name,
      optional<double> p_zero,
      optional<double> uniform_p0,
      optional<double> alpha,
      optional<double> lambda,
      optional<bool>   anchor,
      optional<bool>   end_dist,
      optional<Uint>   max_jump
) {
   string line;
   if ( !getline(dist_file, line) )
      error(ETFatal, "Empty file: %s", dist_file_name);

   HMMJumpStrategy* jump_strategy(NULL);
   if ( HMMJumpSimple::matchMagicString(line) ) {
      jump_strategy = new HMMJumpSimple;
   } else if ( HMMJumpEndDist::matchMagicString(line) ) {
      jump_strategy = new HMMJumpEndDist;
   } else if ( HMMJumpClasses::matchMagicString(line) ) {
      jump_strategy = new HMMJumpClasses;
   } else if ( HMMJumpMAP::matchMagicString(line) ) {
      jump_strategy = new HMMJumpMAP;
   } else {
      error(ETFatal, "Unknown magic line in %s, aborting",
         dist_file_name, line.c_str());
   }

   jump_strategy->read(dist_file, dist_file_name);

   if ( p_zero )     jump_strategy->set_p_zero     ( *p_zero );
   if ( alpha )      jump_strategy->set_alpha      ( *alpha );
   if ( lambda )     jump_strategy->set_lambda     ( *lambda );
   if ( anchor )     jump_strategy->set_anchor     ( *anchor );
   if ( uniform_p0 ) jump_strategy->set_uniform_p0 ( *uniform_p0 );
   if ( max_jump )   jump_strategy->set_max_jump   ( *max_jump );

   if ( end_dist )
      error(ETFatal, "Trying to override end_dist after training - this is not currently implemented.");

   cerr << "Loaded HMM model with p0: " << jump_strategy->p_zero
        << " up0: " << jump_strategy->uniform_p0
        << " alpha: " << jump_strategy->alpha
        << " lambda: " << jump_strategy->lambda
        << " anchor: " << jump_strategy->anchor
        << " end-dist: " << (!HMMJumpSimple::matchMagicString(line))
        << " max-jump: " << jump_strategy->max_jump
        << endl;

   return jump_strategy;
} // CreateAndRead()

HMMJumpStrategy::~HMMJumpStrategy() {}

void HMMJumpStrategy::writeBinCounts(ostream& os) const {
   os << getCountMagicString() << endl;
   writeBinCountsImpl(os);
   os << "End of " << getCountMagicString() << endl;
}

void HMMJumpStrategy::readAddBinCounts(istream& is, const char* stream_name) {
   // "Magic number"
   string line;
   if ( ! getline(is, line) )
      error(ETFatal, "No input in bin HMMAligner count file %s",
            stream_name);
   if ( getCountMagicString() != line )
      error(ETFatal,
            "Magic line does not match current strategy: expected %s; got %s",
            getCountMagicString(), line.c_str());

   // actual data
   readAddBinCountsImpl(is, stream_name);

   // footer
   if ( ! getline(is, line) )
      error(ETFatal, "No footer in bin HMMAligner count file %s",
            stream_name);
   if ( line != string("End of ") + getCountMagicString() )
      error(ETWarn, "Bad footer in bin HMMAligner count file %s: %s",
            stream_name, line.c_str());
   
}

double HMMJumpStrategy::fillDefaultHMMJumpProbs(HMM* hmm, Uint I) const {
   // Calculate p0 based on p_zero (Och style) and uniform_p0 (Liang style).
   double p0 = p_zero + uniform_p0/(I+1);
   if ( p0 > .5 )
      error(ETWarn, "p0 very high (%f) when I=(%d), consider revising your "
            "P0 (%f) and UP0 (%f) parameters",
            p0, I, p_zero, uniform_p0);
   if ( p0 > 1 )
      error(ETFatal, "p0 > 1, can't build coherent HMM");
   if ( p0 <= 0 ) {
      error(ETWarn, "p0 = %f when I=(%d), using 0 instead; consider revising "
            "your P0 (%f) and UP0 (%f) parameters",
            p0, I, p_zero, uniform_p0);
      p0 = 0.0;
   }

   // Some jump probabilities don't depend on the strategy, fill them here
   for ( Uint i = 0; i <= I; ++i ) {
      // transition away from the </s> anchor is never possible
      if ( anchor && i == I ) {
         for ( Uint j = 0; j <= I; ++j ) {
            hmm->A(i + I+1, j) = hmm->A(i,j) = 0.0;
            hmm->A(i + I+1, j + I+1) = hmm->A(i, j + I+1) = 0.0;
         }
         // These jumps will never be possible, due the the emission probs, but
         // the distribution of jumps out of any state still needs to add up to
         // 1, so we arbitrarily set one such jump to 1.
         hmm->A(i + I+1, 0) = hmm->A(i,0) = 1.0;
         continue;
      }

      // can never go back to dummy start state
      hmm->A(i,0) = hmm->A(i + I+1, 0) = 0.0;

      // transition to a null state: p0 if j = i, 0 otherwise
      for ( Uint j = 0; j <= I; ++j )
         hmm->A(i, j + I+1) = hmm->A(i + I+1, j + I+1) = 0.0;
      hmm->A(i, i + I+1) = hmm->A(i + I+1, i + I+1) = p0;
   }

   return p0;
} // fillDefaultHMMJumpProbs()
