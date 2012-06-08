/**
 * @author Uli Germann and Eric Joanis
 * @file mmtsa.find.cc
 * @brief Look for a string in a TPSA.
 */
#include <sstream>

#include <boost/program_options.hpp>

#include "ug_mm_tsa.h"
#include "tpt_tokenindex.h"
#include "ug_get_options.h"
#include "ug_corpus_token.h"
#include "ug_mm_ttrack.h"

using namespace std;
using namespace ugdiss;

string cfgFile;
string tsaFile;
string tdxFile;
string mctFile;
string tpsaDir;
string searchMe;

size_t skip;       // number of occurrences to skip (for "next page" option)
size_t maxSamples; // number of occurrences to return
bool random_sample;
bool count_mode;          
bool verbose;
bool show_token_id;

void
interpret_args(int ac, char* av[])
{
  namespace po=boost::program_options;
  po::variables_map vm;
  po::options_description o("Options");
  po::options_description h("Hidden Options");
  po::positional_options_description a;

  o.add_options()
    ("help,h",    "print this message")
    ("cfg,f", po::value<string>(&cfgFile), "config file")
    ("tdx",   po::value<string>(&tdxFile), "vocab file")
    ("tsa",   po::value<string>(&tsaFile), "tsa file")
    ("mct",   po::value<string>(&mctFile), "mct file")
    ("tpsa",  po::value<string>(&tpsaDir), "TPSA directory (replaces tdx, tsa and mct)")
    ("skip",  po::value<size_t>(&skip)->default_value(0), "number of occurrences to skip")
    ("max,n", po::value<size_t>(&maxSamples)->default_value(100), "maximum number of occurrences to return (0=no max)")
    ("tokenid",   "also show the token ID for each match")
    ("random",    "take a random sample if more than max occ. are found")
    ("count",     "only print a count of matching occurrences (estimate if > ~10*max) ")
    ("verbose,v", "verbose output")
    ;

  h.add_options()
    ("search",    po::value<string>(&searchMe),"search phrase")
    ;
  a.add("search",1);
  get_options(ac,av,h.add(o),a,vm,"cfg");

  if (vm.count("help"))
    {
      cout << "usage:\n\t" << av[0] << " --tpsa <TPSA dir> \"<search expression>\"\n" << endl;
      cout << o << endl;
      exit(0);
    }

  random_sample = vm.count("random");
  count_mode = vm.count("count");
  verbose = vm.count("verbose");
  show_token_id = vm.count("tokenid");
}

typedef L2R_Token<SimpleWordId>     Token;
typedef mmTSA<Token>::tree_iterator titer;
mmTSA<Token>    T;
TokenIndex      V;
mmTtrack<Token> C;
int main(int argc, char* argv[])
{
  interpret_args(argc,argv);

  if (tpsaDir != "") {
    open_mm_tsa(tpsaDir, V, C, T);
  } else {
    V.open(tdxFile);
    C.open(mctFile);
    T.open(tsaFile,&C);
  }

  tsa::ArrayEntry I;
  char const *start, *stop;

  if(searchMe == "") {
    I.next = start = T.arrayStart();
    stop = T.arrayEnd();
  } else {
    titer m(&T);
    size_t q(0), p(0);
    while (true)
      {
        p = searchMe.find_first_not_of(" \t", q);
        if (p == string::npos) break;
        q = searchMe.find_first_of(" \t", p);
        if (!m.extend(V[searchMe.substr(p,q-p)]))
          exit(1); // not found
        if (q == string::npos) break;
      }

    I.next = start = m.lower_bound(-1);
    stop = m.upper_bound(-1);
  }

  const Token* corpus_start = C.sntStart(0);

  if (count_mode) {
    if (uint64_t(T.approxCnt(I.next,stop)) >= 10 * uint64_t(maxSamples)) {
      // There are more than maxSamples occurrences: estimate
      cout << T.approxCnt(I.next,stop) << endl;
    } else {
      // There are few enough occurrences: count them all
      cout << T.rawCnt(I.next,stop) << endl;
    }
  } else if (random_sample && maxSamples != 0) {
    srand(time(NULL));
    // Guesstimate how many times the search string occurs.
    if (uint64_t(T.approxCnt(I.next,stop)) >= 10 * uint64_t(maxSamples)) {
      // There are at least 10 times as many occurrences as we want samples:
      // sample randomly with replacement.  At a 10x ratio, sampling with
      // replacement is an acceptable approximation of sampling without
      // replacement, and it is much faster to compute.
      if (verbose) {
         cout << "Estimating there are " << T.approxCnt(I.next,stop) << " occurrences." << endl;
         cout << "Token index; sentence index; sentence offset" << endl;
      }
      for (unsigned int i = 0; i < maxSamples; ++i) {
        T.readEntry(T.random_sample(start,stop),I);
        if (show_token_id) {
          const Token* token = C.sntStart(I.sid)+I.offset;
          size_t token_index = token - corpus_start;
          cout << token_index << ' ';
        }
        cout << I.sid << ' ' << I.offset;
        if (verbose)
          cout << ' ' << T.suffixAt(I, &V);
        cout << endl;
      }
    } else {
      // There are few enough occurrences: enumerate them all, sampling
      // "online" without replacement
      vector<tsa::ArrayEntry> keep;
      keep.reserve(maxSamples);

      unsigned int n = 0;
      while (I.next < stop) {
        ++n;
        T.readEntry(I.next,I);
        if (keep.size() < maxSamples)
          keep.push_back(I);
        else
          if ((rand() % n) < maxSamples)
            keep[rand()%maxSamples] = I;
      }

      if (verbose) {
         cerr << "Found exactly " << n << " occurrences." << endl;
         cerr << "Token index; sentence index; sentence offset" << endl;
      }
      for (unsigned int i = 0; i < keep.size(); ++i) {
        if (show_token_id) {
          const Token* token = C.sntStart(keep[i].sid)+keep[i].offset;
          size_t token_index = token - corpus_start;
          cout << token_index << ' ';
        }
        cout << keep[i].sid << ' ' << keep[i].offset;
        if (verbose)
          cout << ' ' << T.suffixAt(keep[i], &V);
        cout << endl;
      }
    }
  } else {
    size_t ctr=0;
    size_t maxCtr=skip+maxSamples;
    if (maxSamples == 0)
      maxCtr = size_t(-1);
    do {
      T.readEntry(I.next,I);
      if (ctr++ < skip) continue;
      if (show_token_id) {
        const Token* token = C.sntStart(I.sid)+I.offset;
        size_t token_index = token - corpus_start;
        cout << token_index << ' ';
      }
      cout << I.sid << " " << I.offset;
      if (verbose)
         cout << " " << T.suffixAt(I, &V);
      cout << endl;
    } while (I.next != stop && ctr < maxCtr);
  }
}
