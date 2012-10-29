/**
 * @author Eric Joanis
 * @file tp_alignment_build.cc - Program to read green alignment and build a
 *       tightly packed, indexed file to store them for fast and compact access.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include "file_utils.h"
#include "arg_reader.h"
#include "word_align_io.h"
#include "tpt_pickler.h"
#include "num_read_write.h"
#include "tp_alignment.h"

#if 0
#define DEBUGTPALIGN(expr) expr
#else
#define DEBUGTPALIGN(expr)
#endif

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
tp_alignment_build [options] INFILE OUTFILE.tpa\n\
\n\
  Convert a green-format, plain-text alignment file into a tightly packed,\n\
  indexed alignment file to be read using memory-mapped IO.\n\
\n\
  Note that a pipe can be used to build the TPA file directly from the output\n\
  of align-words:\n\
    align-words -o green ALIGNMENT_OPTIONS | tp_alignment_build - OUTFILE.tpa\n\
\n\
  OUTFILE.tpa, however, must be a real file name, since some random write\n\
  access is required while creating it.\n\
\n\
Options:\n\
\n\
  -v    Verbose output.\n\
";

static bool verbose = false;
static string infile("-");
static string outfile("-");

static void getArgs(int argc, char* argv[]);

/*
static const string magic_number = "Portage tightly packed alignment track v1.0";
static const string middle_marker = ": end of data, beginning of index";
static const string final_marker = ": end of index and file.";

struct AlignmentLink {
   Uint value; ///< the word this link aligns to in the other sentence
   bool empty; ///< true iff this is "-", i.e., an empty-set indicator
   bool last;  ///< true iff this is the last link for the current word

   /// Create an empty-alignment-set link.
   AlignmentLink() : value(0), empty(true), last(true) {}
   /// Create an alignment link for the given link value
   AlignmentLink(Uint value, bool last) : value(value), empty(false), last(last) {}
   Uint64 pack() const {
      if (empty)
         return 1;
      else
         return ((Uint64(value) + 1) << 1) + (last?1:0);
   }
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
*/

int main(int argc, char* argv[])
{
   getArgs(argc, argv);
   iSafeMagicStream istr(infile);
   ofstream ostr(outfile.c_str());
   if (ostr.fail())
      error(ETFatal, "Unable to open %s for writing%s%s", outfile.c_str(),
            (errno != 0 ? ": " : ""), (errno != 0 ? strerror(errno) : ""));

   Int64 pos = 0;

   // magic number
   ostr << TPAlignment::magic_number;
   pos += TPAlignment::magic_number.size();
   DEBUGTPALIGN(assert(pos == ostr.tellp()));

   // file-global parameter block - dummy values as placeholders for now
   Uint line_count = 0;
   Int64 index_start = 0;
   Int64 index_end = 0;
   Int64 parameter_pos = pos;
   ugdiss::numwrite(ostr, line_count);
   ugdiss::numwrite(ostr, index_start);
   ugdiss::numwrite(ostr, index_end);
   pos += sizeof(line_count) + sizeof(index_start) + sizeof(index_end);
   DEBUGTPALIGN(assert(pos == ostr.tellp()));

   // main data block
   vector<vector<Uint> > sets;
   vector<string> dummy1, dummy2;
   GreenReader alignment_reader;
   vector<Int64> index;
   index.push_back(pos);
   while (alignment_reader(istr, dummy1, dummy2, sets)) {
      ++line_count;
      for (Uint i = 0; i < sets.size(); ++i) {
         if (sets[i].empty()) {
            pos += ugdiss::binwrite(ostr, AlignmentLink().pack());
         } else {
            for (Uint j = 0; j + 1 < sets[i].size(); ++j)
               pos += ugdiss::binwrite(ostr, AlignmentLink(sets[i][j], false).pack());
            pos += ugdiss::binwrite(ostr, AlignmentLink(sets[i].back(), true).pack());
         }
      }
      DEBUGTPALIGN(assert(pos == ostr.tellp()));
      index.push_back(pos);
   }

   // mid-file marker
   ostr << TPAlignment::magic_number << TPAlignment::middle_marker;
   pos += TPAlignment::magic_number.size() + TPAlignment::middle_marker.size();
   DEBUGTPALIGN(assert(pos == ostr.tellp()));

   // index block
   index_start = pos;
   for (Uint i = 0; i < index.size(); ++i) {
      ugdiss::numwrite(ostr, index[i]);
      pos += sizeof(index[i]);
   }
   DEBUGTPALIGN(assert(pos == ostr.tellp()));
   index_end = pos;

   // end of file marker
   ostr << TPAlignment::magic_number << TPAlignment::final_marker;

   // Come back and fill in the parameter block
   ostr.seekp(parameter_pos);
   if (ostr.fail())
      error(ETFatal, "Error seeking to beginning of %s.", outfile.c_str());
   ugdiss::numwrite(ostr, line_count);
   ugdiss::numwrite(ostr, index_start);
   ugdiss::numwrite(ostr, index_end);
}

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
