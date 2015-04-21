/**
 * @author Eric Joanis
 * @file alignment_annotation.h
 *
 * Defines the annotation used to store the word-alignment field in a phrase table.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#ifndef _ALIGNMENT_ANNOTATION_H_
#define _ALIGNMENT_ANNOTATION_H_

#include "annotation_list.h"
#include "voc.h"

namespace Portage {

class AlignmentAnnotation : public PhrasePairAnnotation {
   /// vocabulary of bi-phrase elements
   static Voc alignmentVoc;
   /// ID of the empty alignment
   static Uint emptyAlignmentID;

   /// ID of the stored alignment, w.r.t. alignmentVoc
   Uint alignmentID;

   /// Cloning constructor
   AlignmentAnnotation(Uint alignmentID) : alignmentID(alignmentID) {}

public:
   /// Name of this annotation type
   static const string name;

   /// Constructor for storing an alignment
   /// @param alignment  the alignment in green format with _ separator
   AlignmentAnnotation(const char* value) : alignmentID(alignmentVoc.add(value)) { assert(value); }

   /// "Update" the value; for when we see the alignment a second time for the
   /// same phrase pair: issues a warning, since we can't combine different values.
   virtual void updateValue(const char* value);

   /// Clone self.
   virtual PhrasePairAnnotation* clone() const { return new AlignmentAnnotation(alignmentID); }

   /// Display self for debugging
   virtual void display(ostream& out) const;

   /// Write self to a phrase table
   virtual void write(ostream& phrase_table_file) const;

   /// Default constructor, for getting an AlignmentAnnotation handler
   AlignmentAnnotation() : alignmentID(0) {}

   /// Get the alignment string from this annotation
   const char* getAlignment() const { return alignmentVoc.word(alignmentID); }

   /// Get the alignment ID from this annotation
   Uint getAlignmentID() const { return alignmentID; }

   /**
    * Get the set representation of the alignment
    * @param src_len      length of the source phrase having this alignment
    */
   static const vector<vector<Uint> >* getAlignmentSets(Uint alignID, Uint src_len);

   /// Get an alignment ID from a possibly NULL annotation
   static Uint getID(const AlignmentAnnotation* value) {
      return value ? value->getAlignmentID() : AlignmentAnnotation::emptyAlignmentID;
   }
   
   /// Get the alignment vocabulary for external use
   static Voc& getAlignmentVoc() { return alignmentVoc; }

   /// Find the alignment annotation in AnnotationList list
   static AlignmentAnnotation* get(const AnnotationList& list) { return PhrasePairAnnotation::get<AlignmentAnnotation>(name, list); }

   /// Convenient wrapper around get() and getAlignmentSets()
   /// Return NULL if the list has no AlignmentAnnotation or does not
   /// define an alignment.
   static const vector<vector<Uint> >* getSets(const AnnotationList& list, Uint src_len);
}; // class AlignmentAnnotation

} // namespace Portage

#endif // _ALIGNMENT_ANNOTATION_H_
