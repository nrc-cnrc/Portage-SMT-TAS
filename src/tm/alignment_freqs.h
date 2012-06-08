/**
 * @author Eric Joanis
 * @file alignment_freqs.h  Utility functions to work with alignment information
 *                          in phrase tables, factored so it can be used in tm/
 *                          and in canoe/
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#ifndef _ALIGNMENT_FREQS_H_
#define _ALIGNMENT_FREQS_H_

#include "vector_map.h"
#include "word_align_io.h"
#include "voc.h"

namespace Portage {

/// Alignment frequencies - maps a Uint (indirectly referring to an alingment
/// via a vocabulary) to some frequency type FreqT.
template <typename FreqT> class AlignmentFreqs : public vector_map<Uint,FreqT> {};

/**
 * Convert alignments to a string for printing
 * @param out output
 * @param alignments the alignments to display
 * @param alignment_voc  voc mapping alignment IDs to their string representation
 * @param p1_len the length of the original toks1 sequence in lang1
 * @param p2_len the length of the original toks2 sequence in lang2
 * @param reverse  if true, reverse all alignments before displaying (expensive!)
 * @param top_only  if true, only show the most frequent alignment, keeping
 *                  the one occurring first if there is a tie; if false, show
 *                  all alignments, with counts.
 */
template <typename FreqT>
void displayAlignments(string& out, const AlignmentFreqs<FreqT>& alignments,
                       const Voc& alignment_voc,
                       Uint p1_len, Uint p2_len, bool reverse, bool top_only)
{
   if ( alignments.empty() ) {
      out = "";
      return;
   }
   if ( top_only ) {
      const char* alignment = alignment_voc.word(alignments.max()->first);
      if ( reverse )
         GreenWriter::reverse_alignment(out, alignment, p1_len, p2_len, '_');
      else
         out = alignment;
   } else {
      string reverse_alignment;
      ostringstream oss;
      for ( typename AlignmentFreqs<FreqT>::const_iterator it(alignments.begin()),
                                                           end(alignments.end());
            it != end; /* increment happens inside loop */ ) {
         const Uint alignment_index = it->first;
         const char* alignment = alignment_voc.word(alignment_index);

         if ( reverse ) {
            //cerr << "pre-rev: " << alignment << " p1_len=" << p1_len << " p2_len=" << p2_len << endl;
            GreenWriter::reverse_alignment(reverse_alignment, alignment, p1_len, p2_len, '_');
            //cerr << "post-rev: " << reverse_alignment << endl;
            oss << reverse_alignment;
         } else {
            oss << alignment;
         }

         // Output the count, unless it's 1 and there is only one alignment
         if ( !(it->second == FreqT(1)) || alignments.size() != 1 )
            oss << ":" << it->second;
         if ( ++it != end ) oss << ";";
      }
      out = oss.str();
   }
}


/**
 * Parse and tally alignments as printed using displayAlignments()
 * Side effect: alignments found in s are added to alignment_voc.
 * @param[in/out] alignments  tally parsed alignments and counts here,
 *                            adding to existing counts if any
 * @param alignment_voc  voc mapping alignment IDs to their string representation;
 *                       if any new alignments are seen, they are added to alignment_voc
 * @param in  alignments in display format
 */
template <typename FreqT>
void parseAndTallyAlignments(AlignmentFreqs<FreqT>& alignments, Voc& alignment_voc, const char* in)
{
   vector<string> all_alignments;
   split(in, all_alignments, ";");
   if ( alignments.size() < all_alignments.size() )
      alignments.reserve(all_alignments.size());
   for ( Uint i = 0; i < all_alignments.size(); ++i ) {
      string::size_type colon_pos = all_alignments[i].find(':');
      if ( colon_pos == string::npos ) {
         if ( all_alignments.size() > 1 ) {
            static bool warning_displayed = false;
            if ( !warning_displayed ) {
               warning_displayed = true;
               error(ETWarn, "alignments without counts are treated as 1's");
            }
         }
         const Uint alignment_id = alignment_voc.add(all_alignments[i].c_str());
         alignments[alignment_id] += FreqT(1);
      } else {
         FreqT count;
         if ( !convT(all_alignments[i].substr(colon_pos+1).c_str(), count) ) {
            error(ETFatal, "Count is not a number: %s in %s",
                  all_alignments[i].substr(colon_pos+1).c_str(),
                  in);
         } else {
            const Uint alignment_id = alignment_voc.add(all_alignments[i].substr(0,colon_pos).c_str());
            alignments[alignment_id] += count;
         }
      }
   }
}

template <typename FreqT>
void parseAndTallyAlignments(AlignmentFreqs<FreqT>& alignments, Voc& alignment_voc, const string& in)
{
   parseAndTallyAlignments(alignments, alignment_voc, in.c_str());
}

/*
template <typename FreqT>
void parseAndTallyAlignments(PTrie<FreqT>::iterator alignments, Voc& alignment_voc, const string& in)
{
   AlignmentFreqs<FreqT> alignments_old_style;
   parseAndTallyAlignments(alignments_old_style, alignment_voc, in);
   for ( AlignmentFreqs<FreqT>::iterator it(alignments_old_style.begin()),
                                         end(alignments_old_style.end());
         it != end; ++it ) {
      FreqT* val_p;
      alignments.find_or_insert(it->first, val_p);
      val_p += it->second;
   }
}
*/

} // namespace Portage



#endif // _ALIGNMENT_FREQS_H_
