/**
 * @author Samuel Larkin
 * @file wordClass2bin.cc
 * @brief Given a file with a list of word[\t ]tag, binarize its unordered_map's representation.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include "binio.h"
#include "binio_maps.h"
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
#include "canoe_utils.h"
#include <string>
#include <map>
#include <tr1/unordered_map>

using namespace std;
using namespace Portage;

static char help_message[] = "\n\
wordClass2bin [options] [INFILE [OUTFILE]]\n\
\n\
  Binarizes a list of of word[\\t ]tag for canoe.\n\
\n\
Options:\n\
\n\
  -m    Use a map instead of an unordered_map.\n\
";

static bool useMap = false;
static string infile("-");
static string outfile("-");


static void getArgs(int argc, char* argv[]);

template <class Tags>
void writeTags(const string& infile, const string& outfile) {
   Tags tags;  // srcword -> tag ; optional alterative to canoe -srctags option
   loadClasses(tags, infile);
   oSafeMagicStream os(outfile);
   BinIO::writebin(os, tags);
}


int main(int argc, char* argv[]) {
   printCopyright(2015, "wordclasses2bin");
   getArgs(argc, argv);

   if (useMap)
      writeTags<map<string, string> >(infile, outfile);
   else
      writeTags<std::tr1::unordered_map<string, string> >(infile, outfile);

   return 0;
}


// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"m"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("m", useMap);

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
