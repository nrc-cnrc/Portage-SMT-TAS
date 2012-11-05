/**
 * @author Eric Joanis
 * @file line_index_file.cc - Structure to keep track of a line-indexed file
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include "errors.h"
#include "line_indexed_file.h"
#include "file_utils.h"

using namespace Portage;

void LineIndexedFile::open(const string& filename) {
   error_unless_exists(filename, false, "line indexed");
   try {
      file.open(filename);
   } catch (std::exception e) {
      error(ETFatal, "Can't open file %s as memmap: %s; maybe it's not seekable?", filename.c_str(), e.what());
   }
   if (!file.is_open())
      error(ETFatal, "Can't open file %s", filename.c_str());
   index.clear();
   const char* data = file.data();
   size_t size = file.size();
   index.push_back(data);
   while (true) {
      const void* next_newline = memchr(data, '\n', size);
      if (next_newline) {
         const char* next_data = reinterpret_cast<const char*>(next_newline) + 1;
         index.push_back(next_data);
         if (data + size <= next_data) break;
         size -= (next_data - data);
         data = next_data;
      } else {
         data += size;
         index.push_back(data);
         break;
      }
   }
}

void LineIndexedFile::close() {
   file.close();
   index.clear();
}

Uint LineIndexedFile::size() const {
   assert(!index.empty());
   return index.size() - 1;
}

string LineIndexedFile::get(Uint line_id) const {
   assert(line_id + 1 < index.size());
   string line(index[line_id], (index[line_id+1] - index[line_id]));
   if (!line.empty() && *(line.rbegin()) == '\n')
      line.resize(line.size()-1);
   return line;
}
