/**
 * @author Eric Joanis
 * @file tp_alignment_dump.cc - Program to dump a tp alignment file back in green
 *       format.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include "file_utils.h"
#include "arg_reader.h"
#include "str_utils.h"
#include "alignment_file.h"
#include "word_align_io.h"
#include <string>

using namespace Portage;

static char help_message[] = "\n\
tp_alignment_dump [options] ALIGNMENTFILE [RANGE1...RANGEn]\n\
\n\
  Convert a tightly packed, indexed alignment file back into green format.\n\
  Results are written to stdout.\n\
\n\
  If ranges are provided, in the format start[-end], only lines in the given\n\
  0-based ranges are produced.  Lines from start up to (and including) end are\n\
  printed.\n\
\n\
Options:\n\
\n\
  -v    Verbose output.\n\
";


static bool verbose = false;
static string alignment_file;
const char* switches[] = {"v"};
ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
static void getArgs(int argc, char* argv[]);

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   AlignmentFile* file = AlignmentFile::create(alignment_file);
   if (!file)
      error(ETFatal, "Can't open alignment file %s", alignment_file.c_str());

   vector<vector<Uint> > sets;
   vector<string> dummy;
   GreenWriter greenwriter;

   if (arg_reader.numVars() <= 1) {
      // dump whole tpa file
      Uint end = file->size();
      for (Uint i = 0; i < end; ++i) {
         bool rc = file->get(i, sets);
         assert(rc);
         greenwriter(cout, dummy, dummy, sets);
      }
   } else {
      for (Uint arg = 1; arg < arg_reader.numVars(); ++arg) {
         string range = arg_reader.getVar(arg);
         vector<Uint> r;
         split(range, r, "-", 2);
         if (r.empty())
            error(ETFatal, "bad argument (%s): can't specify empty range", range.c_str());
         Uint begin = r[0];
         Uint end = (r.size() > 1 ? r[1] + 1 : begin + 1);
         //cerr << "dumping [" << begin << "," << end << ")" << endl;
         if (end > file->size())
            error(ETFatal, "%s out of range: file has %u lines", range.c_str(), file->size());
         // dump [begin,end)
         for (Uint i = begin; i < end; ++i) {
            bool rc = file->get(i, sets);
            assert(rc);
            greenwriter(cout, dummy, dummy, sets);
         }
      }
   }
   
   delete file;

   return 0;
}

static void getArgs(int argc, char* argv[])
{
   arg_reader.read(argc-1, argv+1);
   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet(0, "alignment_file", alignment_file);
}
