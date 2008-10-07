/**
 * @author Eric Joanis
 * @file multi_voc.cc MultiVoc implementation
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "portage_defs.h"
#include "multi_voc.h"
#include "errors.h"
#include "file_utils.h"
#include <numeric>

using namespace Portage;

MultiVoc::MultiVoc(Voc& base_voc, Uint num_vocs)
   : base_voc(base_voc)
   , num_vocs(num_vocs)
{}


void MultiVoc::write(ostream& file) const
{
   typedef vector< boost::dynamic_bitset<> >::const_iterator IT;
   for (IT it(membership.begin()); it!=membership.end(); ++it)
      file << *it << endl;
}

void MultiVoc::_synch_size_to_base_voc() {
   if ( membership.size() < base_voc.size() )
      membership.insert(membership.end(),
                        base_voc.size() - membership.size(),
                        boost::dynamic_bitset<>(num_vocs));
}

void MultiVoc::add(Uint index, Uint voc_num) {
   assert(index < base_voc.size());
   assert(voc_num < num_vocs);
   if ( index >= membership.size() ) _synch_size_to_base_voc();
   assert ( membership[index].size() == num_vocs );
   membership[index][voc_num] = true;
}

void MultiVoc::add(Uint index, const boost::dynamic_bitset<>& voc_set) {
   assert(index < base_voc.size());
   assert(voc_set.size() == num_vocs);
   if ( index >= membership.size() ) _synch_size_to_base_voc();
   assert ( membership[index].size() == num_vocs );
   membership[index] |= voc_set; // set union
}

const boost::dynamic_bitset<>& MultiVoc::get_vocs(Uint index) {
   assert(index < base_voc.size());
   if ( index >= membership.size() ) _synch_size_to_base_voc();
   assert ( membership[index].size() == num_vocs );
   return membership[index];
}

double MultiVoc::averageVocabSizePerSentence() const
{
   typedef vector<boost::dynamic_bitset<> >::const_iterator IT;
   
   vector<Uint> counts(get_num_vocs(), 0);
   for (IT it(membership.begin()); it!=membership.end(); ++it)
   {
      assert(it->size() == get_num_vocs());
      for (Uint i(0); i<it->size(); ++i) {
         if (it->test(i)) {
            ++counts[i];
         }
      }
   }

   return double(std::accumulate(counts.begin(), counts.end(), 0.0f)) / double(counts.size());
}

