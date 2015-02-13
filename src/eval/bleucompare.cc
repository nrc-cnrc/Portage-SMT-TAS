/**
 * @author George Foster
 * @file bleucompare.cc 
 * @brief Program that compare BLEU scores of different test files.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include <ext/numeric> // iota
#include "exception_dump.h"
#include "arg_reader.h"
#include "bootstrap.h"
#include "bleu.h"
#include "PERstats.h"
#include "WERstats.h"
#include "perSentenceStats.h"
#include "printCopyright.h"
#include "basic_data_structure.h"
#include "referencesReader.h"

using namespace Portage;
using namespace std;

namespace Portage {
namespace bleuCompare {

/// Program bleuCompare usage.
static char help_message[] = "\n\
bleucompare [-v][-n n] [-PsBLEU | -bleu | -per | -wer]\n\
    testfile1 .. testfile_n REFS ref1 .. ref_m\n\
\n\
Compare scores over a set of testfiles using bootstrap resampling.\n\
List the test files first, followed by the REFS keyword, followed by the\n\
reference files.\n\
\n\
Options:\n\
\n\
-v      Write progress reports to cerr.\n\
-n      Use n bootstrapped samples for comparison. [1000]\n\
-bleu   train using bleu as the scoring metric [do]\n\
-wer    train using wer as the scoring metric [don't]\n\
-per    train using per as the scoring metric [don't]\n\
-PsBLEU train using perSentence BLEU as the scoring metric [don't]\n\
";


enum TRAINING_TYPE {
   NONE,
   BLEU,
   WER,
   PsBLEU,
   PER
};

// globals
static bool verbose = false;
static Uint num_iters = 1000;
static vector<string> filenames;
static vector<istream*> testfiles;
static vector<istream*> reffiles;
static TRAINING_TYPE training_type = BLEU;

static void getArgs(int argc, const char* const argv[]);

/// Callable entity for booststrap confidence interval.
/// Compute BLEU for each test text over a selected set of indexes into the
/// bleustats array
template<class ScoreStats>
class computer
{
private:
   /// Disable constructor.
   computer();

public:
   /// A reference on the ScoreStats for all translations.
   const vector<vector<ScoreStats> >& stats;

   /// Default constructor.
   /// @param stats  ScoreStats for all translations.
   computer(const vector<vector<ScoreStats> >& stats)
   : stats(stats)
   {}

   /// What is an iterator in the context of computer.
   typedef vector<Uint>::const_iterator iterator;

   /**
    * Compute BLEU for each test text over a selected set of indexes into the
    * bleustats array.
    * @param beg  start iterator
    * @param end  end iterator
    * @return 
    */
   vector<double> operator()(iterator beg, iterator end) {
      vector<double> res(stats.size());
      for (Uint i = 0; i < stats.size(); ++i) {
	 ScoreStats total;
	 for (iterator it = beg; it != end; ++it)
	    total += stats[i][*it];
	 res[i] = total.score();
      }
      return res;
   }
};
} // ends namespace bleuCompare
} // ends namespace Portage

using namespace bleuCompare;

/// Forward declaration.
template<class ScoreStats>
void compute();

typedef perSentenceStats<BLEUstats> psBLEU;

// main
int MAIN(argc, argv)
{
   printCopyright(2006, "bleucompare");
   getArgs(argc, argv);

   switch (training_type) {
   case PsBLEU:
      compute<psBLEU>();
      break;
   case BLEU:
      compute<BLEUstats>();
      break;
   case PER:
      compute<PERstats>();
      break;
   case WER:
      compute<WERstats>();
      break;
   default:
      cerr << "Not implemented yet" << endl;
   }
} END_MAIN

/// Computes the N wise comparison of N systems (N sets of translation).
template<class ScoreStats>
void compute()
{
   cerr << "Comparing using " << ScoreStats::name() << endl;
   if (verbose) 
      cerr << "analyzing " 
	   << testfiles.size() << " test files, " 
	   << reffiles.size() << " reference files" << endl;
   
   Sentence translation;
   References references(reffiles.size());
   vector< vector<ScoreStats> > stats(testfiles.size());

   // Compute each translation's score according to the references.
   while (getline(*reffiles[0], references[0])) {
      for (Uint i = 1; i < reffiles.size(); ++i) {
         references[i].clearTokens();
         if (!getline(*reffiles[i], references[i]))
            error(ETFatal, "reference file %s too short", filenames[i+testfiles.size()+1].c_str());
      }
      for (Uint i = 0; i < testfiles.size(); ++i) {
         translation.clearTokens();
         if (!getline(*testfiles[i], translation))
            error(ETFatal, "test file %s too short", filenames[i].c_str());
         stats[i].push_back(ScoreStats(translation, references));
      }
      references[0].clearTokens();
   }

   // Check test and reference files if they were all read aka all of the same length.
   for (Uint i = 0; i < reffiles.size(); ++i)
      if (getline(*reffiles[i], references[0])) 
         error(ETFatal, "reference file %s too long", filenames[i+testfiles.size()+1].c_str());
   for (Uint i = 0; i < testfiles.size(); ++i)
      if (getline(*testfiles[i], translation))
         error(ETFatal, "test file %s too long", filenames[i].c_str());

   vector<Uint> indexes(stats.front().size());
   iota(indexes.begin(), indexes.end(), Uint(0));

   computer<ScoreStats> bc(stats);
   vector<Uint> results(testfiles.size());
   
   if (verbose) cerr << "bootstrapping with " << num_iters << " iterations" << endl;
   
   bootstrapNWiseComparison(indexes.begin(), indexes.end(), bc, results, num_iters, 0);

   for (Uint i = 0; i < results.size(); ++i)
      cout << filenames[i] << " got max " << ScoreStats::name() << " score in " 
	   << 100.0 * results[i] / num_iters << "% of samples" << endl;

}

// arg processing

void Portage::bleuCompare::getArgs(int argc, const char* const argv[])
{
   const char* const switches[] = {"v", "n:", "PsBLEU", "bleu", "per", "wer"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n:", num_iters);

   if (arg_reader.getSwitch("PsBLEU")) training_type = PsBLEU;
   if (arg_reader.getSwitch("bleu")) training_type = BLEU;
   if (arg_reader.getSwitch("per"))  training_type = PER;
   if (arg_reader.getSwitch("wer"))  training_type = WER;

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
