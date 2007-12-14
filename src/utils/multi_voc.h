/**
 * @author Eric Joanis
 * @file multi_voc.h Multiple vocabularies held in one structure designed for
 *                   efficient intersection calculations.
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef MULTI_VOC_H
#define MULTI_VOC_H

#include "portage_defs.h"
#include "voc.h"
#include "errors.h"
#include <boost/dynamic_bitset.hpp>

namespace Portage {

/// Multi vocabulary : one class holding many vocabularies at once.
/// E.g., one for each of a set of sentences to translate.
///
/// Instead of inheriting from Voc, we have a base voc provided in the
/// constructor, so that the MultiVoc can be deleted indenpendently from the
/// Voc itself.  This design was chosen because of the intended application,
/// which will clear the (potentially quite large) MultiVoc much sooner than
/// the (fairly small) Voc.
class MultiVoc {

   /// Base vocabulary object
   Voc& base_voc;

   /// Membership information: element "index" belongs to vocabulary "voc_num"
   /// iff membership[index][voc_num] == true.
   vector<boost::dynamic_bitset<> > membership;

   /// Number of sub vocabularies stored in this object
   const Uint num_vocs;

   /// Resize the membership vector to match base_voc's size.
   void _synch_size_to_base_voc();

  public:

   /// Constructor
   /// @param base_voc associated regular vocabulary object
   /// @param num_vocs number of vocabularies stored in this object
   MultiVoc(Voc& base_voc, Uint num_vocs);

   /// Outputs the content of membership.  Mainly for debugging purpouses.
   /// @param file  a stream to output membership
   void write(ostream& file) const;

   /// Add element index to vocabulary voc_num
   /// @pre index < base_voc.size() and voc_num < num_vocs
   void add(Uint index, Uint voc_num);

   /// Add element index to a set of vocabularies voc_set
   /// @pre index < base_voc.size() and voc_set.size() == num_vocs
   void add(Uint index, const boost::dynamic_bitset<>& voc_set);

   /// Get the set of vocabularies index belongs to
   /// @pre index < base_voc.size()
   const boost::dynamic_bitset<>& get_vocs(Uint index);

   /// Return the number of vocabs in this object
   Uint get_num_vocs() const { return num_vocs; }

   /// Clear the whole multi-voc
   void clear() { membership.clear(); }

   /// Calculates the average Vocabulary size of all Sentences
   double averageVocabSizePerSentence() const;

}; // MultiVoc

} // Portage

#endif
