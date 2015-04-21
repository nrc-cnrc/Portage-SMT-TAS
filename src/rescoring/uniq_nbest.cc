/**
 * @author George Foster
 * @file uniq_nbest.cc
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
uniq_nbest [-v][-kout KOUT][-p palfile] K [nbest [nbestout]]\n\
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
-p     Given phrase-alignment file <pal> corresponding to <nbest>, write the\n\
       alignments for the selected hypotheses to <pal>.uniq. This works with\n\
       any file that is line-aligned with <nbest> It may be repeated multiple\n\
       times with different files.\n\
";

// globals

static bool verbose = false;
static Uint K;
static Uint KOUT = 10;
static string nbests("-");
static string nbestsout("-");
static vector<string> pals;
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

   vector<istream*> pals_in(pals.size());
   vector<ostream*> pals_out(pals.size());
   for (Uint i = 0; i < pals.size(); ++i) {
      pals_in[i] = new iSafeMagicStream(pals[i]);
      pals_out[i] = new oSafeMagicStream(addExtension(pals[i], ".uniq"));
   }

   vector<string> nbout;
   vector< vector<string> > palouts(pals_in.size());  // pal index, k -> hyp side info
   string line;
   vector<string> pallines(pals_in.size()); // pal index -> curr line

   while (true) {		// for each nbest list

      nbout.clear();
      for (Uint i = 0; i < pals_in.size(); ++i) palouts[i].clear();

      Uint i = 0;
      for (; i < K; ++i) {	// for each hyp

	 if (!getline(nbestsfile, line))
	    break;

         for (Uint j = 0; j < pals_in.size(); ++j)
            if (!getline((*pals_in[j]), pallines[j]))
	       error(ETFatal, "pal file %s too short", pals[j].c_str());

	 if (nbout.size() != K) { // try to add this hyp

	    bool dup = false;
	    for (vector<string>::reverse_iterator p = nbout.rbegin(); p != nbout.rend(); ++p)
	       if (line == *p) {
		  dup = true;
		  break;
	       }
	    if (!dup) {
	       nbout.push_back(line);
	       for (Uint j = 0; j < pals_in.size(); ++j)
                  palouts[j].push_back(pallines[j]);
	    }
	 }
      }

      if (i == 0)
	 break;
      else if (i != K)
	 error(ETFatal, "nbest size not a multiple of %d", K);

      for (Uint i = 0; i < KOUT; ++i) {
         if (i < nbout.size()) {
            nbestsoutfile << nbout[i];
            for (Uint j = 0; j < pals_in.size(); ++j)
               (*pals_out[j]) << palouts[j][i];
	 }
         nbestsoutfile << "\n";
	 for (Uint j = 0; j < pals_in.size(); ++j)
            (*pals_out[j]) << "\n";
      }
   }

   // close output files (& flush buffer)
   for (Uint i = 0; i < pals_in.size(); ++i) {
      delete pals_in[i];
      delete pals_out[i];
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "kout:", "p:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("kout", KOUT);
   arg_reader.testAndSet("p", pals);

   arg_reader.testAndSet(0, "K", K);
   arg_reader.testAndSet(1, "nbests", nbests);
   arg_reader.testAndSet(2, "nbestsout", nbestsout);
}
