/**
 * $Id$
 * @author Eric Joanis
 * @file hmm_jump_map.cc  Implementation of HMMJumpMAP.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "hmm_jump_map.h"
#include "binio.h"
#include "ibm.h"

using namespace Portage;

HMMJumpMAP::HMMJumpMAP(double tau, double min_count, bool end_dist,
                       HMMJumpStrategy* prior)
   : tau(tau)
   , min_count(min_count)
   , end_dist(end_dist)
   , prior(prior)
{
}

HMMJumpMAP::HMMJumpMAP()
   : tau(1)
   , min_count(0)
   , end_dist(true)
   , prior(NULL)
{
}

HMMJumpMAP::HMMJumpMAP(const HMMJumpMAP& that)
   : HMMJumpStrategy(that)
   , tau(that.tau)
   , min_count(that.min_count)
   , end_dist(that.end_dist)
   , voc(that.voc)
   , prior(that.prior->Clone())
   , word_prob(that.word_prob)
   , word_count(that.word_count)
{
}

HMMJumpMAP::~HMMJumpMAP() {
   delete prior;
}

HMMJumpMAP* HMMJumpMAP::Clone() const {
   return new HMMJumpMAP(*this);
}

static const char* const BinProbMagicString = "HMMJumpMAP bin lexical probs";
static const char* const RegProbMagicString = "HMMJumpMAP lexical probs";

void HMMJumpMAP::write(ostream& out, bool bin) const {
   out.precision(9);
   out << p_zero << " "
       << uniform_p0 << " "
       << alpha << " "
       << lambda << " "
       << anchor << " "
       << end_dist << " "
       << max_jump << " "
       << tau << " "
       << min_count << " "
       << voc.size() << nf_endl;
   voc.writeStream(out);
   assert(voc.size() == word_prob.size());
   if ( bin ) {
      out << BinProbMagicString << endl;
      for ( Uint i(0); i < word_prob.size(); ++i )
         word_prob[i].writeBin(out);
      out << "End of " << BinProbMagicString << endl;
   } else {
      out << RegProbMagicString << endl;
      for ( Uint i(0); i < word_prob.size(); ++i ) {
         out << i << " ";
         word_prob[i].write(out);
      }
   }
   assert(prior);
   out << prior->getMagicString() << endl;
   prior->write(out, bin);
}

void HMMJumpMAP::read(istream& in, const char* stream_name) {
   Uint voc_size;
   in >> p_zero >> uniform_p0 >> alpha >> lambda >> anchor
      >> end_dist >> max_jump >> tau >> min_count >> voc_size;
   if ( in.eof() )
      error(ETFatal, "Unexpected EOF in HMMJumpMAP model %s after basic "
            "parameters", stream_name);
   string line;
   if ( !getline(in, line) )
      error(ETFatal, "No vocabulary in HMMJumpMAP model %s", stream_name);
   if ( line != "" )
      error(ETFatal, "Unexpected content after basic parameters in "
            "HMMJumpMAP model %s", stream_name);
   voc.readStream(in, stream_name);
   if ( voc.size() != voc_size )
      error(ETFatal, "Error in %s: Expected voc size %d, got %d",
            stream_name, voc_size, voc.size());
   word_prob.resize(voc_size);
   if ( !getline(in, line) )
      error(ETFatal, "Unexpected EOF in HMMJumpMAP model %s after voc ",
            stream_name);
   if ( line == BinProbMagicString ) {
      for ( Uint i(0); i < voc_size; ++i )
         word_prob[i].readBin(in);
      if ( !getline(in, line) )
         error(ETFatal, "Unexpected EOF in HMMJumpMAP model %s after lexical "
               "probs", stream_name);
      if ( line != string("End of ") + BinProbMagicString )
         error(ETFatal, "Unexpected magic line in HMMJumpMAP model %s after "
               "lexical probs", stream_name);
   } else if ( line == RegProbMagicString ) {
      for ( Uint i(0); i < voc_size; ++i ) {
         Uint read_i;
         in >> read_i;
         if ( i != read_i )
            error(ETFatal, "Expected parameters for index %u (word %s), "
                  "got %u in %s", i, voc.word(i), read_i, stream_name);
         if ( ! word_prob[i].read(in) )
            error(ETFatal, "Error in index %d (word %s) jump parameters in %s",
                  i, voc.word(i), stream_name);
         if ( in.eof() )
            error(ETFatal, "Unexpected EOF in %s after jump parameters for "
                  "index %u (word %s)", stream_name, i, voc.word(i));
      }
   } else {
      error(ETFatal, "Unexpected prob magic sgring in HMMJumpMAP model %s "
            "after voc: %s", stream_name, line.c_str());
   }
   prior = HMMJumpStrategy::CreateAndRead(in, stream_name);
   cerr << "Prior model loaded." << endl;
}

void HMMJumpMAP::writeBinCountsImpl(ostream& os) const {
   using namespace BinIO;

   writebin(os, Uint(word_count.size()));
   for ( Uint i(0); i < word_count.size(); ++i )
      word_count[i].writeBin(os);

   prior->writeBinCounts(os);
}

void HMMJumpMAP::readAddBinCountsImpl(istream& is, const char* stream_name) {
   using namespace BinIO;

   Uint word_count_size(0);
   readbin(is, word_count_size);
   if ( word_count_size != 0 && word_count_size != word_count.size() )
      error(ETFatal, "Wrong vocab size in %s.  Found %u, expected %u",
            stream_name, word_count_size, word_count.size());
   S tmp_s;
   for ( Uint i = 0; i < word_count_size; ++i ) {
      tmp_s.readBin(is);
      word_count[i] += tmp_s;
   }

   prior->readAddBinCounts(is, stream_name);
}

bool HMMJumpMAP::hasSameCounts(const HMMJumpStrategy* s) {
   const HMMJumpMAP* that = dynamic_cast<const HMMJumpMAP*>(s);
   assert(that);
   if ( that == NULL ) return false;
   if ( ! prior->hasSameCounts(that->prior) ) return false;
   if ( !word_count.empty() && !that->word_count.empty() )
      return word_count == that->word_count;
   else {
      const vector<S>* counts;
      if ( !word_count.empty() )
         counts = &word_count;
      else if ( !that->word_count.empty() )
         counts = &(that->word_count);
      else
         return true;
      for ( vector<S>::const_iterator it(counts->begin()), end(counts->end());
            it != end; ++it )
         if ( it->sum() != 0.0 ) return false;
      return true;
   }
}

void HMMJumpMAP::initCounts(const TTable& tt) {
   prior->initCounts(tt);
   if ( voc.empty() ) {
      // The first time we init the counts, we get the vocabulary from the TTable
      // We copy the vocab into a vector and sort it.  This method is
      // inefficient, but guarantees a stable ordering between different
      // instances of train_ibm when used with cat.sh for parallelized
      // training.  Any stable ordering would do, sorting is just the simplest
      // to implement.  Relying on the ordering from the hash table iterator is
      // *not* guaranteed to be stable, so we don't want to do that, even
      // though it might be fine most of the time.
      vector<string> voc_list;
      voc_list.reserve(tt.numSourceWords());
      for ( TTable::WordMapIter it(tt.beginSrcVoc()), end(tt.endSrcVoc());
            it != end; ++it )
         voc_list.push_back(it->first);
      sort(voc_list.begin(), voc_list.end());
      for ( vector<string>::const_iterator it(voc_list.begin()), end(voc_list.end());
            it != end; ++it )
         voc.add((*it).c_str());

      assert(word_count.empty());
      assert(word_prob.empty());
      word_count.resize(voc.size());
      word_prob.resize(voc.size());
   } else {
      assert(word_prob.size() == voc.size());
      if ( word_count.empty() ) {
         word_count.resize(voc.size());
      } else {
         assert(word_count.size() == voc.size());
         for ( Uint i = 0; i < word_count.size(); ++i )
            word_count[i].clear();
      }
   }
}

void HMMJumpMAP::countJumps(const dMatrix& A_counts,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I)
{
   assert(src_toks[0] == IBM1::nullWord());
   prior->countJumps(A_counts, src_toks, tgt_toks, I);

   // Jumps from start position are only in the prior - we only need to count
   // jumps from lexical positions here.
   const Uint max_normal_i = anchor ? (I-1) : I;
   assert(max_normal_i == src_toks.size() - 1);
   const Uint max_reg_jump_to_i = (anchor && end_dist) ? (I-1) : I;
   assert(voc.size() == word_count.size());
   assert(voc.size() == word_prob.size());
   for ( Uint i(1); i <= max_normal_i; ++i ) {
      const Uint word_id = voc.index(src_toks[i].c_str());
      if ( word_id == voc.size() ) continue;
      assert(word_id < word_count.size());

      for ( Uint j(1); j <= max_reg_jump_to_i; ++j ) {
         const double new_count = A_counts(i,j) + A_counts(i + I+1, j);
         S::get_count(word_count[word_id].jump,i,j,I,max_jump) += new_count;
      }
      if ( anchor && end_dist ) {
         const Uint j = I;
         const double new_count = A_counts(i,j) + A_counts(i + I+1, j);
         word_count[word_id].final += new_count;
      }
   }
}

void HMMJumpMAP::estimate() {
   prior->estimate();
   assert(voc.size() == word_count.size());
   if ( min_count > 0.0 ) {
      time_t start_time(time(NULL));
      Voc new_voc;
      Uint new_i = 0;
      for ( Uint i(0); i < word_count.size(); ++i ) {
         const double total_count = word_count[i].sum();
         if ( total_count < min_count ) {
            if ( new_i == i ) {
               // This is the first difference, start preparing new_voc
               for ( Uint j = 0; j < i; ++j )
                  new_voc.add(voc.word(j));
            }
            word_count[i].clear();
         } else {
            if ( new_i < i ) {
               const Uint new_index = new_voc.add(voc.word(i));
               assert(new_index == new_i);
               word_count[new_i].swap(word_count[i]);
            }
            ++new_i;
         }
      }
      cerr << "HMMJumpMAP::min_count pruning at " << min_count << ": from "
           << word_count.size() << " to " << new_i << " words in "
           << (time(NULL) - start_time) << " seconds."
           << endl;
      if ( new_i < word_count.size() ) {
         word_count.resize(new_i);
         voc.swap(new_voc);
      }
   }
   word_prob = word_count;
   assert(voc.size() == word_prob.size());
   assert(voc.size() == word_count.size());
}

void HMMJumpMAP::fillHMMJumpProbs(HMM* hmm,
      const vector<string>& src_toks,
      const vector<string>& tgt_toks,
      Uint I) const
{
   // fill the hmm probs with the prior first, and then go into the lexicalized
   // emission probabilities and modify them according to the MAP formula,
   // leaving alone anything that MAP has no refined opinion for.  (i.e., jump
   // from start position and jump from OOVs or pruned words.)
   prior->fillHMMJumpProbs(hmm, src_toks, tgt_toks, I);

   // Used for validating assumptions mainly
   double p0 = p_zero + uniform_p0/(I+1);
   if ( p0 <= 0.0 ) p0 = 0.0;

   const double remaining_prob_mass = 1.0 - p0;
   const double inv_remaining_prob_mass = double(1.0) / remaining_prob_mass;
   const Uint max_normal_i = anchor ? (I-1) : I;
   assert(max_normal_i == src_toks.size() - 1);
   const Uint max_reg_jump_to_i = (anchor && end_dist) ? (I-1) : I;

   for ( Uint i = 1; i <= max_normal_i; ++i ) {
      assert(hmm->A(i, i + I+1) == p0);
      assert(hmm->A(i + I+1, i + I+1) == p0);
      
      const Uint word_id = voc.index(src_toks[i].c_str());

      if ( word_id == voc.size() ) continue;

      // Here we calculate P_MAP using equation (11) in He (ACL-WMT07)
      assert(word_id < word_prob.size());
      double sum(tau);
      for ( Uint j = 1; j <= max_reg_jump_to_i; ++j )
         sum += S::get_jump_p(word_prob[word_id].jump,i,j,max_reg_jump_to_i,max_jump);
      if ( anchor && end_dist )
         sum += word_prob[word_id].final;
      for ( Uint j = 1; j <= max_reg_jump_to_i; ++j ) {
         assert(hmm->A(i + I+1, j) == hmm->A(i,j));
         const double prior_prob = inv_remaining_prob_mass * hmm->A(i,j);
         const double map_prob = 
            ( S::get_jump_p(word_prob[word_id].jump,i,j,max_reg_jump_to_i,max_jump) 
              + tau * prior_prob ) / sum;
         hmm->A(i + I+1, j) = hmm->A(i,j) = remaining_prob_mass * map_prob;
      }
      if ( anchor && end_dist ) {
         assert(hmm->A(i + I+1, I) == hmm->A(i,I));
         const double prior_prob = inv_remaining_prob_mass * hmm->A(i,I);
         const double map_prob = 
            ( word_prob[word_id].final + tau * prior_prob ) / sum;
         hmm->A(i + I+1, I) = hmm->A(i,I) = remaining_prob_mass * map_prob;
      }
   }
}


