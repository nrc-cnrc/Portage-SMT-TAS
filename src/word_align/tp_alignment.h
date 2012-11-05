/**
 * @author Eric Joanis
 * @file tp_alignment.h - Interface to get random access to tightly-packed,
 *                        indexed alignments.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#ifndef _TP_ALIGNMENT_H_
#define _TP_ALIGNMENT_H_

#include <vector>
#include "portage_defs.h"
#include "alignment_file.h"
#include <boost/iostreams/device/mapped_file.hpp>

namespace Portage {

/// Utility class for converting representations of an alignment link.
struct AlignmentLink {
   Uint value; ///< the word this link aligns to in the other sentence
   bool empty; ///< true iff this is "-", i.e., an empty-set indicator
   bool last;  ///< true iff this is the last link for the current word

   /// Create an empty-alignment-set link.
   explicit AlignmentLink() : value(0), empty(true), last(true) {}

   /// Create an alignment link for the given link value
   explicit AlignmentLink(Uint value, bool last)
      : value(value), empty(false), last(last) {}

   /// Create an alignment link from its packed representation
   explicit AlignmentLink(Uint64 packed) { unpack(packed); }

   /// Return the packed representation for this link
   Uint64 pack() const {
      if (empty)
         return 1;
      else
         return ((Uint64(value) + 1) << 1) + (last?1:0);
   }

   /// Initialize self from the given packed representation.
   void unpack(Uint64 packed) {
      assert(packed != 0);
      if (packed == 1) {
         value = 0;
         empty = last = true;
      } else {
         empty = false;
         last = packed & 1;
         value = (packed >> 1) - 1;
      }
   }
};


/// Class giving random access to tightly packed alignments
class TPAlignment : public AlignmentFile {
   typedef Int64 PosType; ///< type for file positions and index entries
   boost::iostreams::mapped_file_source file;
   /// Base position of the memory-mapped data
   const char* base;
   Uint line_count;
   PosType index_start;
   PosType index_end;

public:
   static const string magic_number;
   static const string middle_marker;
   static const string final_marker;

   /// Test whether a given file is a TPAlignment file.
   static bool isTPAlignmentFile(const string& filename);

   /// Constructor opens the given tightly packed alignment file
   TPAlignment(const string& filename);

   virtual bool get(Uint i, vector< vector<Uint> >& sets) const;
   virtual Uint size() const;

}; // class TPAlignment
   


} // namespace Portage

#endif // _TP_ALIGNMENT_H_
