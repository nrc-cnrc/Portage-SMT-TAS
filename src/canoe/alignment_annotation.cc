/**
 * @author Eric Joanis
 * @file alignment_annotation.cc
 *
 * Implementation for AlignmentAnnotation.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "alignment_annotation.h"
#include "alignment_freqs.h"

Voc AlignmentAnnotation::alignmentVoc;
Uint AlignmentAnnotation::emptyAlignmentID = AlignmentAnnotation::alignmentVoc.add("");
const string AlignmentAnnotation::name = "a";

void AlignmentAnnotation::display(ostream& out) const
{
   if (alignmentID != emptyAlignmentID)
      out << "\talignment             "
          << alignmentVoc.word(alignmentID)
          << endl;
}

void AlignmentAnnotation::write(ostream& phrase_table_file) const
{
   if (alignmentID != emptyAlignmentID)
      write_helper(phrase_table_file, name, alignmentVoc.word(alignmentID));
}

void AlignmentAnnotation::updateValue(const char* value)
{
   assert(value);
   static bool warningDisplayed = false;
   if (!warningDisplayed) {
      warningDisplayed = true;
      error(ETWarn, "Duplicate alignment information found.  Combining alignment information from multiple phrase tables is not supported; retaining only the last alignment read.  (This message will only be printed once even if there are many cases.)");
   }
   alignmentID = alignmentVoc.add(value);
}

const vector<vector<Uint> >* AlignmentAnnotation::getAlignmentSets(Uint alignmentID, Uint src_len)
{
   static vector< vector<Uint> > sets;
   static Uint prev_alignmentID = Uint(-1);
   static Uint prev_src_len = Uint(-1);

   // Cache the last query to this function, because it's likely to be called
   // multiple times in a row for the same decoder state, i.e., with the same
   // arguments.
   if (alignmentID == prev_alignmentID && src_len == prev_src_len)
      return &sets;

   AlignmentFreqs<float> alignment_freqs;
   const char* alignment_string = alignmentVoc.word(alignmentID);
   parseAndTallyAlignments(alignment_freqs, alignmentVoc, alignment_string);
   if (alignment_freqs.empty()) {
      sets.clear();
   }  else if (!alignment_freqs.empty()) {
      const char* top_alignment_string =
         alignmentVoc.word(alignment_freqs.max()->first);
      GreenReader('_').operator()(top_alignment_string, sets);
      if (!sets.empty() && sets.size() < src_len)
         sets.resize(src_len); // pad with empty sets if some are missing.
   }

   prev_alignmentID = alignmentID;
   prev_src_len = src_len;

   return &sets;
}

const vector<vector<Uint> >* AlignmentAnnotation::getSets(const AnnotationList& list, Uint src_len) {
   AlignmentAnnotation* ann = get(list);
   if (ann)
      return getAlignmentSets(ann->getAlignmentID(), src_len);
   else
      return NULL;
}
