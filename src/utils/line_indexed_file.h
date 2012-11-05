/**
 * @author Eric Joanis
 * @file line_index_file.h - Structure to keep track of a line-indexed file
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#ifndef _LINE_INDEXED_FILE_H_
#define _LINE_INDEXED_FILE_H_

#include "portage_defs.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <vector>

namespace Portage {

class LineIndexedFile {
   boost::iostreams::mapped_file_source file;
   vector<const char*> index;

public:
   explicit LineIndexedFile() {};
   explicit LineIndexedFile(const string& filename) { open(filename); }
   /// Open a plain text file, which will be indexed on the fly.
   void open(const string& filename);
   /// Close the file
   void close();
   /// Returns the number of lines in the file
   Uint size() const;
   /// Get line line_id, without its newline character.
   string get(Uint line_id) const;
};


} // namespace Portage

#endif // _LINE_INDEXED_FILE_H_

