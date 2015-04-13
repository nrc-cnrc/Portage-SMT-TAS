/**
 * @author Eric Joanis
 * @file count_annotation.h
 *
 * Defines the annotation used to store the count field in a phrase table.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#ifndef _COUNT_ANNOTATION_H_
#define _COUNT_ANNOTATION_H_

#include "annotation_list.h"

namespace Portage {

class PhraseTable;

class CountAnnotation : public PhrasePairAnnotation {
   /// Copy constructor, for clone()
   explicit CountAnnotation(const CountAnnotation& c) : joint_counts(c.joint_counts) {}

   /// Parse the count string from the c= field in phrase tables
   void parseCounts(const char* value);

   static bool appendJointCounts;
   static const PhraseTable* phraseTable;

public:
   vector<float> joint_counts;
   static const string name;

   /**
    * Call this static function before loading phrase tables if
    * appendJointCounts mode is desired: i.e., to append joint counts from
    * different incoming tables (default is to add them element-wise).
    * @param _phraseTable  the appendJointCounts mode needs access to the
    *                      phrase table object, so pass it is here.
    */
   static void setAppendJointCountsMode(const PhraseTable* _phraseTable);

   /// Default constructor yields empty counts (needed by getOrCreate())
   explicit CountAnnotation() {}
   /// Constructor initializes from the string found in the c= field in phrase tables
   explicit CountAnnotation(const char* value);
   /// Update the value with a new instance of the same phrase pair.
   virtual void updateValue(const char* value);
   /// Update the value with a vector of instances to tally (from TPPTs)
   template <class CountT> void updateValue(const vector<CountT>& value);

   virtual PhrasePairAnnotation* clone() const { return new CountAnnotation(*this); }
   static CountAnnotation* get(const AnnotationList& list) { return PhrasePairAnnotation::get<CountAnnotation>(name, list); }
   static CountAnnotation* getOrCreate(const AnnotationList& list) { return PhrasePairAnnotation::getOrCreate<CountAnnotation>(name, list); }
   virtual void display(ostream& out) const;
   virtual void write(ostream& phrase_table_file) const;
}; // class CountAnnotation

template <class CountT>
void CountAnnotation::updateValue(const vector<CountT>& value)
{
   if (appendJointCounts)
      error(ETFatal, "-append-joint-count mode does not support TPPTs");
   if (joint_counts.size() < value.size())
      joint_counts.resize(value.size(), 0);
   for (Uint i = 0; i < value.size(); ++i)
      joint_counts[i] += value[i];
}

} // namespace Portage

#endif // _COUNT_ANNOTATION_H_
