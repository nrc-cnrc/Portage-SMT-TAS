/**
 * @author Samuel Larkin
 * @file wordClass2tpmap.cc
 * @brief Converts a word class to its memory mapped representation.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include "mm_map.h"
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
wordClass2tpmap INFILE [OUTFILE]\n\
\n\
  Converts a word class file (word\\tclass) to its memory mapped representation.\n\
\n\
";

static string infile("-");
static string outfile("-");
static void getArgs(int argc, const char* const argv[]);


int MAIN(argc, argv) {
   printCopyright(2015, "classes2voc");
   getArgs(argc, argv);

   iSafeMagicStream is(infile);
   oSafeMagicStream os(outfile);
   ugdiss::mkMemoryMappedMap(is, os);

   return 0;
}
END_MAIN

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet(0, "infile", infile);
   arg_reader.testAndSet(1, "outfile", outfile);
}
