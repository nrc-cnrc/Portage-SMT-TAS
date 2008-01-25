/**
 * @author George Foster
 * @file bleucompare.cc  Program that compare BLEU scores of different test files.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <exception_dump.h>
#include <arg_reader.h>
#include <bootstrap.h>
#include "bleu.h"
#include <printCopyright.h>

using namespace Portage;
using namespace std;

namespace Portage {
namespace bleuCompare {

/// Program bleuCompare usage.
static char help_message[] = "\n\
bleucompare [-v][-n n] testfile1 .. testfile_n REFS ref1 .. ref_m\n\
\n\
Compare BLEU scores over a set of testfiles using bootstrap resampling.\n\
List the test files first, followed by the REFS keyword, followed by the\n\
reference files.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-n  Use n bootstrapped samples for comparison. [1000]\n\
";

// globals

static bool verbose = false;
static Uint num_iters = 1000;
static vector<string> filenames;
static vector<istream*> testfiles;
static vector<istream*> reffiles;

static vector< vector<BLEUstats> > bleustats;

static void getArgs(int argc, const char* const argv[]);

/// Callable entity for booststrap confidence interval.
/// Compute BLEU for each test text over a selected set of indexes into the
/// bleustats array
struct BLEUcomputer
{
   /**
    * Compute BLEU for each test text over a selected set of indexes into the
    * bleustats array.
    * @param beg  start iterator
    * @param end  end iterator
    * @return 
    */
   vector<double> operator()(vector<Uint>::const_iterator beg,
			     vector<Uint>::const_iterator end) {
      vector<double> res(bleustats.size());
      for (Uint i = 0; i < bleustats.size(); ++i) {
	 BLEUstats total;
	 vector<Uint>::const_iterator it;
	 for (it = beg; it != end; ++it)
	    total = total + bleustats[i][*it];
	 res[i] = total.score();
      }
      return res;
   }
};
} // ends namespace bleuCompare
} // ends namespace Portage
using namespace bleuCompare;



// main

int MAIN(argc, argv)
{
   printCopyright(2006, "bleucompare");
   getArgs(argc, argv);

   string testline;
   vector<string> reflines(reffiles.size());
   bleustats.resize(testfiles.size());

   if (verbose) 
      cerr << "analyzing " 
	   << testfiles.size() << " test files, " 
	   << reffiles.size() << " reference files" << endl;
   
   while (getline(*reffiles[0], reflines[0])) {
      for (Uint i = 1; i < reffiles.size(); ++i)
	 if (!getline(*reffiles[i], reflines[i]))
	    error(ETFatal, "reference file %s too short", filenames[i+testfiles.size()+1].c_str());
      for (Uint i = 0; i < testfiles.size(); ++i) {
	 if (!getline(*testfiles[i], testline))
	    error(ETFatal, "test file %s too short", filenames[i].c_str());
	 bleustats[i].push_back(BLEUstats(testline, reflines, 1));
      }
   }

   for (Uint i = 0; i < reffiles.size(); ++i)
      if (getline(*reffiles[i], testline)) 
	 error(ETFatal, "reference file %s too long", filenames[i+testfiles.size()+1].c_str());
   for (Uint i = 0; i < testfiles.size(); ++i)
      if (getline(*testfiles[i], testline))
	 error(ETFatal, "test file %s too long", filenames[i].c_str());

   vector<Uint> indexes(bleustats[0].size());
   for (Uint i = 0; i < indexes.size(); ++i)
      indexes[i] = i;

   BLEUcomputer bc;
   vector<Uint> results(testfiles.size());
   
   if (verbose) cerr << "bootstrapping with " << num_iters << " iterations" << endl;
   
   bootstrapNWiseComparison(indexes.begin(), indexes.end(), bc, results, num_iters, 0);

   for (Uint i = 0; i < results.size(); ++i)
      cout << filenames[i] << " got max bleu score in " 
	   << 100.0 * results[i] / num_iters << "% of samples" << endl;

} END_MAIN

// arg processing

void Portage::bleuCompare::getArgs(int argc, const char* const argv[])
{
   const char* const switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n:", num_iters);
   arg_reader.getVars(0, filenames);

   bool got_refs = false;
   for (Uint i = 0; i < filenames.size(); ++i)
      if (!got_refs) {
	 if (filenames[i] == "REFS") {got_refs = true; continue;}
	 testfiles.push_back(new iSafeMagicStream(filenames[i]));
      } else 
	 reffiles.push_back(new iSafeMagicStream(filenames[i]));
   if (!testfiles.size())
      error(ETFatal, "no test files specified");
   if (!reffiles.size())
      error(ETFatal, "no ref files specified");
}   
