/**
 * @author George Foster
 * @file uniq_nbest 
 * @brief Remove duplicates from an nbest list.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
uniq_nbest [-v][-kout KOUT] K [nbest [nbestout]]\n\
\n\
Remove duplicate entries from input nbest lists. K is the size of each list;\n\
the number of lines in the file <nbest> must be an integer multiple of K. The\n\
output lists will be padded with blank lines if necessary.\n\
\n\
Options:\n\
\n\
-v     Write progress reports to cerr.\n\
-kout  Write output lists having at most KOUT best candidates from the input\n\
       lists. [10]\n\
";

// globals

static bool verbose = false;
static Uint K;
static Uint KOUT = 10;
static string nbests("-");
static string nbestsout("-");
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2008, "uniq_nbest");

   getArgs(argc, argv);

   if (KOUT > K) {
      error(ETWarn, "KOUT must be <= K: setting KOUT to %d", K);
      KOUT = K;
   }

   iSafeMagicStream nbestsfile(nbests);
   oSafeMagicStream nbestsoutfile(nbestsout);

   vector<string> nbout;
   string line;

   while (true) {		// for each nbest list

      nbout.clear();

      Uint i = 0;
      for (; i < K; ++i) {	// for each hyp

	 if (!getline(nbestsfile, line))
	    break;

	 if (nbout.size() != K) { // try to add this hyp

	    bool dup = false;
	    for (vector<string>::reverse_iterator p = nbout.rbegin(); p != nbout.rend(); ++p)
	       if (line == *p) {
		  dup = true;
		  break;
	       }
	    if (!dup)
	       nbout.push_back(line);
	 }
      }

      if (i == 0)
	 break;
      else if (i != K)
	 error(ETFatal, "nbest size not a multiple of %d", K);

      for (Uint i = 0; i < KOUT; ++i) {
         if (i < nbout.size())
            nbestsoutfile << nbout[i];
         nbestsoutfile << "\n";
      }
   }
  
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "kout:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("kout", KOUT);

   arg_reader.testAndSet(0, "K", K);
   arg_reader.testAndSet(1, "nbests", nbests);
   arg_reader.testAndSet(2, "nbestsout", nbestsout);
}
