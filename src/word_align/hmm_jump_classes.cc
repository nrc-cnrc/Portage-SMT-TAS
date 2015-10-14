/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_classes.cc  Implementation of jump strategy conditioning jump
 *                            probabilities on word classes
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "hmm_jump_classes.h"
#include "binio.h"
#include "ibm.h"

using namespace Portage;

HMMJumpClasses::HMMJumpClasses(const char* word_class_file, bool end_dist)
   : end_dist(end_dist)
{
   if ( word_class_file ) {
      wc.read(word_class_file);
      class_prob.resize(wc.getHighestClassId() + 1);
   }
}

HMMJumpClasses* HMMJumpClasses::Clone() const {
   // use the automatically generated copy constructor
   return new HMMJumpClasses(*this);
}

void HMMJumpClasses::write(ostream& out, bool bin) const {
   out.precision(9);
   out << p_zero << " "
       << uniform_p0 << " "
       << alpha << " "
       << lambda << " "
       << anchor << " "
       << end_dist << " "
       << max_jump << " "
       << class_prob.size() << endl;
   global_prob.write(out);
   init_prob.write(out);
   for ( Uint i(0); i < class_prob.size(); ++i ) {
      out << i << " ";
      class_prob[i].write(out);
   }
   wc.writeStream(out);
}

void HMMJumpClasses::read(istream& in, const char* stream_name) {
   Uint class_prob_size;
   in >> p_zero >> uniform_p0 >> alpha >> lambda >> anchor
      >> end_dist >> max_jump >> class_prob_size;
   if ( in.eof() )
      error(ETFatal, "Unexpected EOF in %s after basic parameters",
            stream_name);
   if ( ! global_prob.read(in) )
      error(ETFatal, "Error in global jump parameters in %s", stream_name);
   if ( in.eof() )
      error(ETFatal, "Unexpected EOF in %s after global jump parameters",
            stream_name);
   if ( ! init_prob.read(in) )
      error(ETFatal, "Error in init jump parameters in %s", stream_name);
   if ( in.eof() )
      error(ETFatal, "Unexpected EOF in %s after init jump parameters",
            stream_name);
   class_prob.resize(class_prob_size);
   for ( Uint i(0); i < class_prob.size(); ++i ) {
      Uint read_i;
      in >> read_i;
      if ( i != read_i )
         error(ETFatal, "Expected parameters for class %u, got %u in %s",
               i, read_i, stream_name);
      if ( ! class_prob[i].read(in) )
         error(ETFatal, "Error in class %d jump parameters in %s",
               i, stream_name);
      if ( in.eof() )
         error(ETFatal, "Unexpected EOF in %s after jump parameters for "
               "class %u", stream_name, i);
   }
   wc.readStream(in, stream_name);
}

void HMMJumpClasses::writeBinCountsImpl(ostream& os) const {
   using namespace BinIO;
   global_count.writeBin(os);
   writebin(os, init_count);
   writebin(os, Uint(class_count.size()));
   for ( Uint i = 0; i < class_count.size(); ++i )
      class_count[i].writeBin(os);
}

void HMMJumpClasses::readAddBinCountsImpl(istream& is, const char* stream_name) {
   using namespace BinIO;
   S tmp_s;
   tmp_s.readBin(is);
   global_count += tmp_s;
   BiVector<double> tmp_v;
   readbin(is, tmp_v);
   init_count += tmp_v;
   Uint class_count_size(0);
   readbin(is, class_count_size);
   if ( class_count_size != class_count.size() )
      error(ETFatal, "Wrong number of classes in %s.  Found %u, expected %u",
            stream_name, class_count_size, class_count.size());
   for ( Uint i = 0; i < class_count_size; ++i ) {
      tmp_s.readBin(is);
      class_count[i] += tmp_s;
   }
}

bool HMMJumpClasses::hasSameCounts(const HMMJumpStrategy* s) {
   const HMMJumpClasses* that = dynamic_cast<const HMMJumpClasses*>(s);
   if ( that == NULL ) return false;
   return ( global_count == that->global_count &&
            init_count == that->init_count &&
            class_count == that->class_count );
}

void HMMJumpClasses::initCounts(const TTable& tt) {
   global_count.clear();
   for ( Uint i = 0; i < class_count.size(); ++i )
      class_count[i].clear();
   class_count.resize(wc.getHighestClassId() + 1);
   init_count.clear();
}

void HMMJumpClasses::countJumps(const dMatrix& A_counts,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I
) {
   assert(src_toks[0] == IBM1::nullWord());
   // jumps from start position
   {
      const Uint i = 0;
      for ( Uint j = 1; j <= I; ++j )
         S::get_count(init_count,i,j,I,max_jump) +=
            A_counts(i,j) + A_counts(i + I+1, j);
   }
   // Jumps from other positions
   const Uint max_normal_i = anchor ? (I-1) : I;
   assert(max_normal_i == src_toks.size() - 1);
   const Uint max_reg_jump_to_i = (anchor && end_dist) ? (I-1) : I;
   for ( Uint i(1); i <= max_normal_i; ++i ) {
      const Uint class_id = wc.classOf(src_toks[i].c_str());
      assert(class_id == WordClasses::NoClass || class_id < class_count.size());
      for ( Uint j(1); j <= max_reg_jump_to_i; ++j ) {
         const double new_count = A_counts(i,j) + A_counts(i + I+1, j);
         S::get_count(global_count.jump,i,j,I,max_jump) += new_count;
         if ( class_id != WordClasses::NoClass )
            S::get_count(class_count[class_id].jump,i,j,I,max_jump) += new_count;
      }
      if ( anchor && end_dist ) {
         const Uint j = I;
         const double new_count = A_counts(i,j) + A_counts(i + I+1, j);
         global_count.final += new_count;
         if ( class_id != WordClasses::NoClass )
            class_count[class_id].final += new_count;
      }
   }
}

void HMMJumpClasses::estimate() {
   global_prob = global_count;
   class_prob = class_count;
   init_prob = init_count;
}

void HMMJumpClasses::fillHMMJumpProbs(HMM* hmm,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I
) const {
   assert(hmm);
   assert(src_toks[0] == IBM1::nullWord());

   // fill jumps to null states, back into start state or out of final anchor
   const double p0 = fillDefaultHMMJumpProbs(hmm, I);

   const double remaining_prob_mass = 1.0 - p0;
   const Uint max_normal_i = anchor ? (I-1) : I;
   assert(max_normal_i == src_toks.size() - 1);
   const Uint max_reg_jump_to_i = (anchor && end_dist) ? (I-1) : I;
   // do i = 0 - jumps from start position (<s>)
   {
      const Uint i = 0;
      double sum(0.0);
      for ( Uint j = 1; j <= I; ++j )
         sum += S::get_jump_p(init_prob,i,j,I,max_jump) + lambda;
      for ( Uint j = 1; j <= I; ++j ) {
         hmm->A(i + I+1, j) =
         hmm->A(i,j) =
            remaining_prob_mass *
               alpha_smooth(S::get_jump_p(init_prob,i,j,I,max_jump), sum, I);
      }
   }
   // do jumps from regular words
   for ( Uint i = 1; i <= max_normal_i; ++i ) {
      const Uint class_id = wc.classOf(src_toks[i].c_str());
      assert(class_id == WordClasses::NoClass || class_id < class_prob.size());
      const S& prob(class_id == WordClasses::NoClass
                     ? global_prob
                     : class_prob[class_id]);
      double sum(0.0);
      for ( Uint j = 1; j <= max_reg_jump_to_i; ++j )
         sum += S::get_jump_p(prob.jump,i,j,max_reg_jump_to_i,max_jump) + lambda;
      if ( anchor && end_dist )
         sum += prob.final + lambda;
      for ( Uint j = 1; j <= max_reg_jump_to_i; ++j )
         hmm->A(i + I+1, j) =
         hmm->A(i,j) =
            remaining_prob_mass *
               alpha_smooth(S::get_jump_p(prob.jump,i,j,max_reg_jump_to_i,max_jump)
                             + lambda, sum, I);
      if ( anchor && end_dist )
         hmm->A(i + I+1, I) =
         hmm->A(i,I) =
            remaining_prob_mass *
               alpha_smooth(prob.final + lambda, sum, I);
   }
} // fillHMMJumpProbs()






