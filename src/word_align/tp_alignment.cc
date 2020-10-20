/**
 * @author Eric Joanis
 * @file tp_alignment.cc - Implementation for TPAlignment
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include "tp_alignment.h"
#include "num_read_write.h"
#include "tpt_pickler.h"
#include "errors.h"
#include "file_utils.h"
#include <fstream>

using namespace Portage;

const string TPAlignment::magic_number = "Portage tightly packed alignment track v1.0";
const string TPAlignment::middle_marker = ": end of data, beginning of index";
const string TPAlignment::final_marker = ": end of index and file.";

bool TPAlignment::isTPAlignmentFile(const string& filename)
{
   return matchMagicNumber(filename, magic_number);
}

TPAlignment::TPAlignment(const string& filename)
   : AlignmentFile(filename)
   , line_count(0), index_start(0), index_end(0)
{
   error_unless_exists(filename, false, "tightly packed alignment");
   try {
      file.open(filename);
   } catch (const std::exception& e) {
      error(ETFatal, "Can't open file %s as memmap: %s; maybe it's not seekable?", filename.c_str(), e.what());
   }
   if (!file.is_open())
      error(ETFatal, "Can't open file %s: unknown error", filename.c_str());
   base = file.data();
   const char* data = base;
   if (0 != strncmp(data, magic_number.c_str(), magic_number.size()))
      error(ETFatal, "File %s is not a valid %s", magic_number.c_str());
   data += magic_number.size();
   data = ugdiss::numread(data, line_count);
   data = ugdiss::numread(data, index_start);
   data = ugdiss::numread(data, index_end);
   if (PosType((line_count+1) * sizeof(PosType)) + index_start != index_end)
      error(ETFatal, "File %s seems to have been corrupted: index start (%lu) + count ((%u+1)*%u) != index end (%lu)",
            filename.c_str(), index_start, line_count, sizeof(PosType), index_end);
   if (index_end + magic_number.size() + final_marker.size() != file.size())
      error(ETFatal, "File %s seems to have been corrupted: wrong size", filename.c_str());
   if (0 != strncmp(base+index_end, (magic_number+final_marker).c_str(), magic_number.size() + final_marker.size()))
      error(ETFatal, "File %s seems to have been corrupted: bad final marker", filename.c_str());
   PosType middle_marker_size = magic_number.size() + middle_marker.size();
   if (index_start < middle_marker_size ||
       0 != strncmp(base+(index_start-middle_marker_size), (magic_number+middle_marker).c_str(), middle_marker_size))
      error(ETFatal, "File %s seems to have been corrupted: bad middle marker", filename.c_str());
   PosType first_line;
   ugdiss::numread(base+index_start, first_line);
   if (base+first_line != data)
      error(ETFatal, "File %s seems to have been corrupted: the first line is not where it is expected", filename.c_str());
}

Uint TPAlignment::size() const
{
   assert(file.is_open());
   assert(index_start < index_end);
   assert(line_count > 0);
   return line_count;
}

bool TPAlignment::get(Uint i, vector< vector<Uint> >& sets) const
{
   if (i > line_count) return false;
   PosType index_offset = index_start + i * sizeof(PosType);
   if (index_offset + PosType(sizeof(PosType)) >= index_end) return false;
   PosType data_offset, next_offset;
   const char* next_index_offset =
      ugdiss::numread(base+index_offset, data_offset);
   ugdiss::numread(next_index_offset, next_offset);
   if (data_offset <= 0 ||
       next_offset < data_offset ||
       index_start < next_offset)
      error(ETFatal, "Corrupt %s file: index for %u points outside file", magic_number.c_str(), i);

   if (data_offset == next_offset) {
      // handle empty sets.
      sets.clear();
      return true;
   }

   const char* data = base + data_offset;
   const char* end = base + next_offset;
   Uint j = 0;
   if (0) {
      // Old suboptimal code, keep for reference because it is easier to read.
      sets.resize(1);
      sets[0].clear();
      while (true) {
         Uint64 packed;
         data = ugdiss::binread(data, packed);
         AlignmentLink link(packed);
         if (!link.empty)
            sets[j].push_back(link.value);
         if (data >= end) break;
         if (link.last) {
            sets.push_back(vector<Uint>());
            ++j;
         }
      }
   } else {
      // Reuse the memory in sets as much as possible; this significantly
      // speeds up dynamic phrase-pair extraction.
      Uint sets_size = sets.size();
      if (sets_size <= j) {
         sets.push_back(vector<Uint>());
         ++sets_size;
      } else
         sets[0].clear();
      while (true) {
         Uint64 packed;
         data = ugdiss::binread(data, packed);
         AlignmentLink link(packed);
         if (!link.empty)
            sets[j].push_back(link.value);
         if (data >= end) break;
         if (link.last) {
            ++j;
            if (sets_size <= j) {
               sets.push_back(vector<Uint>());
               ++sets_size;
            } else
               sets[j].clear();
         }
      }
      sets.resize(j+1);
   }
   if (data != end)
      error(ETFatal, "Corrupt %s file: packed entry for %u does not end at entry for %u",
            magic_number.c_str(), i, i+1);
   return true;
}
