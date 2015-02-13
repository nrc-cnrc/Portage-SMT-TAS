/**
 * @author Eric Joanis
 * @file alignment_file.h - Structure to keep track of a word-alignment file
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#ifndef _ALIGNMENT_FILE_H_
#define _ALIGNMENT_FILE_H_

#include "portage_defs.h"
#include <vector>
#include <string>

namespace Portage {

/**
 * Generic class to encapsulate accessing a word alignment file
 */
class AlignmentFile {
private:
   /// Non-copyable.
   AlignmentFile(const AlignmentFile&);
   /// Non-copyable.
   AlignmentFile& operator=(const AlignmentFile&);

protected:
   /// Name of the file loaded, for debugging purposes.
   string name;

   /// Constructor for subclasses to call
   AlignmentFile(const string& name) : name(name) {}

public:
   /// Get the word alignment for sentence pair i.
   /// Returns false, without modifying sets, if i is out of range.
   virtual bool get(Uint i, vector< vector<Uint> >& sets) const = 0;

   /// Get matrix-format word alignment for sentence pair i.
   /// Returns false, without modifying links, if i is out of range.
   virtual bool get(Uint i, vector< vector<float> >& links) const {assert(false);}

   /// Return true if this class instantiates the matrix version of get().
   virtual bool isMatrix() {return false;}

   virtual Uint matrixFormat() {return 0;}

   /// Get the number of alignments in this file
   virtual Uint size() const = 0;

   /// Destructor is virtual since we expect subclasses
   virtual ~AlignmentFile() {}

   /// Factory method: open filename with the appropriate subclass of this
   /// class, returning the created specialized AlignmentFile object.
   static AlignmentFile* create(const string& filename);

   /// Get the original name of the file, for debugging messages
   string getName() const { return name; }
};

class LineIndexedFile;
/**
 * This implementation opens the green alignment file with memory-mapped IO,
 * then builds an index into it so it can get to each sentence's alignment in
 * constant time.
 */
class GreenAlignmentFile : public AlignmentFile {
private:
   LineIndexedFile* file;

public:
   virtual bool get(Uint i, vector< vector<Uint> >& sets) const;
   virtual Uint size() const;

   // Open filename and index it
   GreenAlignmentFile(const string& filename);
   // Desctructor
   ~GreenAlignmentFile();
};

} // namespace Portage

#endif // _ALIGNMENT_FILE_H_
