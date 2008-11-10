/**
 * @author George Foster
 * @file join_phrasetables.cc 
 * @brief Perform a unix join-like operation on a set of input phrasetables.
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <str_utils.h>
#include <arg_reader.h>
#include "basicmodel.h"
#include "phrase_table_reader.h"
#include <printCopyright.h>
#include <logging.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
join_phrasetables [-v][-z v] pt1 pt2 ...\n\
\n\
Perform a unix join-like operation on a set of input phrasetables. For each\n\
phrase pair found in the tables, write it to stdout along with all values\n\
associated with it in the tables. Missing values are represented by zero\n\
(by default). Multicolumn tables are handled correctly; for example, if\n\
table 1 contains values 'b1 f1', and table 2 contains 'b2 f2', then the\n\
output will be 'b1 b2 f1 f2'. If one table is multicolumn, all must be.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-z  Use value v instead of 0 for missing values.\n\
";

// globals

static const char* sep = PHRASE_TABLE_SEP;
static Uint seplen;

static bool verbose = false;
static string zero = "0";

vector<string> pts;

static void getArgs(int argc, char* argv[]);


template <class T>
static void parseLine(const string& pt, Uint linenum, const string& line,
		      string::size_type& pos, vector<T>& probs);
static string getTempName();

// main

int main(int argc, char* argv[])
{
   printCopyright(2007, "join_phrasetables");

   Logging::init();		// @#$$%%^&**^%$#

   seplen = strlen(sep);
   assert(sep[0] == ' ');
   assert(sep[seplen-1] == ' '); // assumed in line !! below

   getArgs(argc, argv);
   assert(pts.size() != 0);

   vector<Uint> ncols(pts.size()); // phrasetable index -> # columns
   string line;
   string::size_type pos;
   vector<string> probs;
   
   // write temporary copies of files with tagged values
   
   vector<string> tmp_pt_list;
   for (Uint i = 0; i < pts.size(); ++i) {

      iSafeMagicStream ifs(pts[i]);
      tmp_pt_list.push_back(getTempName());
      oSafeMagicStream ofs(tmp_pt_list.back());
      
      Uint linenum = 0;
      while (getline(ifs, line)) {
         ++linenum;
         parseLine(pts[i], linenum, line, pos, probs);
	 if (ncols[i] == 0) {
	    ncols[i] = probs.size();
	    if (i > 0 && ncols[i] % 2 != ncols[0] % 2)
	       error(ETFatal, "all phrasetables must single column or all must be multicolumn");
	 }
	 if (ncols[i] != probs.size())
	    error(ETFatal, "variable number of columns in phrasetable %s", pts[i].c_str());

         ofs << line.substr(0, pos+seplen) << i; // add pt index tag
	 for (vector<string>::iterator it = probs.begin(); it != probs.end(); ++it)
	    ofs << ' ' << *it;
         ofs << endl;
      }
      if (verbose)
	 cerr << "Wrote tagged tmp file " << tmp_pt_list.back() << endl;
   }

   // cat and sort the temp files, so that identical phrase pairs are next to
   // each other in a single merged file (another temporary)

   string tmp_pts;
   join(tmp_pt_list.begin(), tmp_pt_list.end(), tmp_pts);
   string tmp = getTempName();
   string command = "zcat -f " + tmp_pts + "| li-sort.sh > " + tmp;
   system(command.c_str());
   for (Uint i = 0; i < pts.size(); ++i)
      unlink(tmp_pt_list[i].c_str());
   if (verbose)
      cerr << "Wrote global sorted temp file " << tmp << endl;

   // figure out where each pt's columns should go in final joined table

   bool multi_col = ncols[0] % 2 == 0;
   Uint tot_cols = 0;
   Uint psum = 0;
   for (Uint i = 0; i < ncols.size(); ++i) {
      tot_cols += ncols[i];
      if (multi_col) ncols[i] /= 2;
      Uint tmp = ncols[i];
      ncols[i] = psum;
      psum += tmp;
   }

   // aggregate probabilities associated with each phrase pair, and write to stdout

   iSafeMagicStream ifs(tmp);
   string prev_phrase_pair;
   vector<string> tot_probs(tot_cols, zero);
   Uint linenum = 0;
   while (getline(ifs, line)) {
      ++linenum;
      parseLine(tmp, linenum, line, pos, probs);
      pos += seplen - 1;    // !!
      if (prev_phrase_pair == "") 
	 prev_phrase_pair = line.substr(0,pos);
      if (prev_phrase_pair != line.substr(0,pos)) {
	 cout << prev_phrase_pair;
         for (vector<string>::iterator it = tot_probs.begin(); it != tot_probs.end(); ++it)
            cout << ' ' << *it;
         cout << endl;
	 prev_phrase_pair = line.substr(0,pos);
	 tot_probs.assign(tot_cols, zero);
      }
      Uint index = conv<Uint>(probs[0]);
      Uint col = ncols[index];
      for (Uint i = 1; i < probs.size(); ++i) {
	 tot_probs[col++] = probs[i];
	 if (multi_col && i == (probs.size() - 1) / 2)
	    col = ncols[index] + tot_cols / 2;
      }
   }
   if (prev_phrase_pair != "") {
      cout << prev_phrase_pair;
      for (vector<string>::iterator it = tot_probs.begin(); it != tot_probs.end(); ++it)
	 cout << ' ' << *it;
      cout << endl;
   }
   unlink(tmp.c_str());

   if (verbose)
      cerr << "Done" << endl;
}

// parse a line from a text phrasetable, extracting probs into given array
template<class T> 
static void parseLine(const string& pt, Uint linenum, const string& line, 
               string::size_type& pos, vector<T>& probs)
{
   pos = line.rfind(sep);
   if (pos == string::npos)
      error(ETFatal, "Bad format at line %d in phrasetable %s", linenum, pt.c_str());
   if (!splitZ(line.substr(pos+seplen), probs))
      error(ETFatal, "Bad probability format at line %d in phrasetable %s", linenum, pt.c_str());
}

static string getTempName()
{
   char* tmp = tmpnam(NULL);
   if (!tmp)
      error(ETFatal, "Unable to get a temp file name using tmpnam");
   return tmp;
}



// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "z:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("z", zero);

   arg_reader.getVars(0, pts);
}
