/**
 * @author George Foster
 * @file bag_trans.cc  Program that Bag-of-words translation, using GIZA M1
 * 'actual.ti' ttable, with lines of form src_word tgt_word
 * p(tgt_word|src_word).
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <ext/hash_map>
#include <map>
#include <algorithm>
#include <arg_reader.h>
#include <str_utils.h>
#include <printCopyright.h>
#include "ttable.h"

using namespace Portage;
using namespace __gnu_cxx;

static char help_message[] = "\n\
bag_trans [-vp][-maxw w][-max t] ttable [infile [outfile]]\n\
\n\
Bag-of-words translation, using GIZA M1 'actual.ti' ttable, with lines of form\n\
src_word tgt_word p(tgt_word|src_word).\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-p    Work with named pipes (both infile & outfile must be explicit).\n\
-maxw Set max translations per word to w [0 = all]\n\
-max  Set max translations output to t [0 = all]\n\
";

// globals

static bool verbose = false;
static bool persist = false;
static string ttable_file;
static string input_file;
static string output_file;
static Uint max_trans_per_word = 0;
static Uint max_trans_in_bag = 0;
static ifstream ifs;
static ofstream ofs;
static void getArgs(int argc, char* argv[]);

bool regetline(istream& is, string& line) 
{
   if (!persist || is == cin)
      return false;
   ifs.clear();
   ifs.close();
   ifs.open(input_file.c_str());
   return getline(is, line);
}

int main(int argc, char* argv[])
{
   printCopyright(2005, "bag_trans");
   getArgs(argc, argv);

   istream& is = ifs.is_open() ? ifs : (istream&)cin;
   ostream& os = ofs.is_open() ? ofs : (ostream&)cout;

   TTable ttable(ttable_file);

   cerr << "- loaded -" << endl;

   map<Uint,double> tword_probs;
   map<double,Uint,greater<double> > twords_by_desc_prob;
   vector<TTable::TIndexAndProb> sdist;
   vector<string> toks;

   while (true) {
      
      string line;
      while (getline(is, line)) {
	 
	 tword_probs.clear();
	 split(line, toks);
	 if (verbose) cerr << line << endl;
	 
	 // compile bag
	 Uint num_toks_with_translations = 0;
	 for (Uint i = 0; i < toks.size(); ++i) {
	    ttable.getSourceDistnByDecrProb(toks[i], sdist);
	    for (Uint i = 0; i < sdist.size() && (max_trans_per_word==0 || i<max_trans_per_word); ++i)
	       tword_probs[sdist[i].first] += sdist[i].second;
	    if (sdist.size()) 
	       ++num_toks_with_translations;
	 }

	 // sort words & establish probs
	 twords_by_desc_prob.clear();
	 map<Uint,double>::const_iterator p;      
	 for (p = tword_probs.begin(); p != tword_probs.end(); ++p)
	    twords_by_desc_prob[p->second / (double) num_toks_with_translations] = p->first;

	 // output
	 map<double,Uint,greater<double> >::const_iterator pp;
	 Uint bag_size = 0;
	 for (pp = twords_by_desc_prob.begin(); pp != twords_by_desc_prob.end() && 
		 (max_trans_in_bag==0 || bag_size < max_trans_in_bag); ++pp, ++bag_size)
	    os << ttable.targetWord(pp->second) << "/" << pp->first << " ";
	 os << endl;

	 toks.clear();
      }

      if (!persist || is == cin || os == cout)
	 break;
      if (verbose) cerr << "packet done" << endl;

      ifs.clear(); ifs.close(); ifs.open(input_file.c_str());
      ofs.clear(); ofs.close(); ofs.open(output_file.c_str());
   }


}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "p", "maxw:", "max:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("p", persist);
   arg_reader.testAndSet("maxw", max_trans_per_word);
   arg_reader.testAndSet("max", max_trans_in_bag);

   arg_reader.testAndSet(0, "ttable file", ttable_file);
   arg_reader.testAndSet(1, "infile", ifs);
   arg_reader.testAndSet(2, "outfile", ofs);

   if (arg_reader.numVars() > 1)
      input_file = arg_reader.getVar(1);
   if (arg_reader.numVars() > 2)
      output_file = arg_reader.getVar(2);

}   
