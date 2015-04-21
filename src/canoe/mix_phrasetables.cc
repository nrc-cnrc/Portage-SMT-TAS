/**
 * @author George Foster
 * @file mix_phrasetables.cc 
 * @brief Produce an interpolated combination of any kind of (text)
 * phrasetable, using given weights.
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
#include "file_utils.h"
#include "str_utils.h"
#include "arg_reader.h"
#include "basicmodel.h"
#include "inputparser.h"
#include "logging.h"

using namespace Portage;

static char help_message[] = "\n\
mix_phrasetables [-vn][-s s][-w wts|-wf wtsfile][-m len][-f src] pt1 pt2 ...\n\
\n\
Produce an interpolated combination of any kind of (text) phrasetable, using\n\
given weights. The input tables <pt1>, <pt2>, etc, must have the same number of\n\
probability columns. The output table is written to stdout. It has one entry for\n\
each phrase pair that occurs in any input table. The probabilities in each\n\
column are of the form p = sum_i w_i p_i, where p_i is the entry in that column\n\
in the ith phrasetable, and w_i is the weight on that phrasetable.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-n  Normalize (integer) frequencies in phrasetables before combining them. This\n\
    is done before filtering.\n\
-s  Add smoothing count <s> to total count when normalizing [1]\n\
-w  Specify a weight on each phrasetable: <wts> is a string of weights w1 w2 ...\n\
    for phrasetables pt1 pt2 ... Weights can be separated by blanks or colons\n\
    (don't forget quotes in the former case). [uniform values]\n\
-wf Same as -w, but read weights from file <wtsfile>.\n\
-m  Apply weights only to left-column phrases of length at most <len> [no limit]\n\
-f  Filter input phrasetables prior to mixing by retaining only the phrase pairs\n\
    needed to translate a given source text. This assumes that source phrases\n\
    are in the left column of the input phrasetables.\n\
";

// globals

static const char* sep = PHRASE_TABLE_SEP;
static Uint seplen;

static bool verbose = false;
static bool calc_norm = false;
static Uint norm_smooth = 1;
static const Uint PHRASE_LENGTH_INFINITY = 10000;
static Uint max_phrase_len = PHRASE_LENGTH_INFINITY;
static string srcname;

vector<string> pts;
vector<double> wts;

static void getArgs(int argc, char* argv[]);


template <class T>
static void parseLine(const string& pt, Uint linenum, const string& line, 
		      string::size_type& pos, vector<T>& probs);
static void parseSourcePhrase(const string& pt, Uint linenum, const string& line,
			      Uint bufsize, char buf[], vector<char*>& toks);
static string getTempName();

// main

int main(int argc, char* argv[])
{
   Logging::init();

   seplen = strlen(sep);
   assert(sep[0] == ' ');	// assumed in line !!! of parseSourcePhrase()
   assert(sep[seplen-1] == ' '); // assumed in line !! below

   getArgs(argc, argv);

   Uint num_probs = 0;
   string line;
   string::size_type pos;

   // calculate normalization factors if normalizing

   vector<Uint> freqs;
   vector< vector<Uint> > norm_sums(pts.size()); // phrasetable,column -> sum of vals in column
   if (calc_norm) {
      for (Uint i = 0; i < pts.size(); ++i) {
	 iSafeMagicStream ifs(pts[i]);
	 norm_sums[i].assign(num_probs, norm_smooth);
	 Uint linenum = 0;
	 while (getline(ifs, line)) {
	    ++linenum;
	    parseLine(pts[i], linenum, line, pos, freqs);
	    if (num_probs == 0) { 
	       num_probs = freqs.size();
	       norm_sums[i].assign(num_probs, norm_smooth);
	    }
	    if (freqs.empty() || freqs.size() != num_probs)
	       error(ETFatal, "Missing or inconsistent number of frequency columns in phrasetable %s", 
		     pts[i].c_str());
	    vector<Uint>::iterator it_norm = norm_sums[i].begin();
	    for (vector<Uint>::iterator it = freqs.begin(); it != freqs.end(); ++it, ++it_norm)
	       *it_norm += *it;
	 }
	 if (verbose)
	    cerr << "Normalized phrasetable " << pts[i] << endl;
      }
   }

   // read in source file for filtering purposes

   PhraseTable* srcphrases = NULL;
   if (srcname != "") {
      VectorPSrcSent sents;
      CanoeConfig c;
      c.loadFirst = false;
      iSafeMagicStream input(srcname);
      InputParser reader(input);
      PSrcSent nss;
      while (nss = reader.getMarkedSent())
         sents.push_back(nss);

      // NB: impossible to delete bmg, but not necessary anyway
      BasicModelGenerator* bmg = new BasicModelGenerator(c, sents);
      srcphrases = &bmg->getPhraseTable();
      if (verbose)
	 cerr << "Filtering with source file " << srcname << endl;

   }

   // write temporary copies of files with scaled probabilities, filtered and
   // normalized if called for
   
   vector<string> tmp_pt_list;
   const Uint linebufsize = 10000;
   char linebuf[linebufsize];
   vector<char*> srctoks;
   vector<double> probs;

   for (Uint i = 0; i < pts.size(); ++i) {

      iSafeMagicStream ifs(pts[i]);
      tmp_pt_list.push_back(getTempName());
      oSafeMagicStream ofs(tmp_pt_list.back());
      ofs.precision(9);
      
      Uint linenum = 0;
      while (getline(ifs, line)) {
         ++linenum;
	 if (srcphrases) {	// see if this src phrase contained in src file
	    parseSourcePhrase(pts[i], linenum, line, linebufsize, linebuf, srctoks);
	    if (!srcphrases->containsSrcPhrase(srctoks.size(), &srctoks[0]))
	       continue;
	 } else if (max_phrase_len < PHRASE_LENGTH_INFINITY) {
	    // need to parse to determine source length
	    parseSourcePhrase(pts[i], linenum, line, linebufsize, linebuf, srctoks);
	 }
         parseLine(pts[i], linenum, line, pos, probs);
         if (num_probs == 0) num_probs = probs.size();
         if (probs.empty() || probs.size() != num_probs)
            error(ETFatal, "Missing or inconsistent number of probability columns in phrasetable %s", 
                  pts[i].c_str());
         ofs << line.substr(0, pos+seplen-1);   // !!
	 double wt = srctoks.size() <= max_phrase_len ? wts[i] : 1.0 / pts.size();
         if (calc_norm) {
            vector<Uint>::iterator it_norm = norm_sums[i].begin();
            for (vector<double>::iterator it = probs.begin(); it != probs.end(); ++it, ++it_norm)
               ofs << ' ' << *it * wt / *it_norm;
         }  else
            for (vector<double>::iterator it = probs.begin(); it != probs.end(); ++it)
               ofs << ' ' << *it * wt;
         ofs << endl;
      }
      if (verbose)
	 cerr << "Wrote weighted tmp file " << tmp_pt_list.back() << endl;
   }

   // cat and sort the temp files, so that identical phrase pairs are next to
   // each other in a single merged file (another temporary)

   string tmp_pts = join(tmp_pt_list);
   string tmp = getTempName();
   string command = "zcat -f " + tmp_pts + "| li-sort.sh > " + tmp;
   int ret = system(command.c_str());
   if ( ret != 0 )
      error(ETFatal, "exit status %d from: %s", ret, command.c_str());
   for (Uint i = 0; i < pts.size(); ++i) {
      unlink(tmp_pt_list[i].c_str());
   }
   if (verbose)
      cerr << "Wrote global sorted temp file " << tmp << endl;

   // sum up the probabilities associated with each phrase pair, and write
   // result to stdout

   iSafeMagicStream ifs(tmp);
   string prev_phrase_pair;
   vector<double> tot_probs(probs.size(), 0.0);
   Uint linenum = 0;
   while (getline(ifs, line)) {
      ++linenum;
      parseLine(tmp, linenum, line, pos, probs);
      pos += seplen - 1;    // !!
      if (prev_phrase_pair == "") 
	 prev_phrase_pair = line.substr(0,pos);
      if (prev_phrase_pair != line.substr(0,pos)) {
	 cout << prev_phrase_pair;
         for (vector<double>::iterator it = tot_probs.begin(); it != tot_probs.end(); ++it)
            cout << ' ' << *it;
         cout << endl;
	 prev_phrase_pair = line.substr(0,pos);
	 tot_probs.assign(probs.size(), 0.0);
      }
      for (Uint i = 0; i < probs.size(); ++i)
	 tot_probs[i] += probs[i];
   }
   if (prev_phrase_pair != "") {
      cout << prev_phrase_pair;
      for (vector<double>::iterator it = tot_probs.begin(); it != tot_probs.end(); ++it)
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

// parse a line from a text phrase table, extracting source phrase into
// given buffer and toks array (pointers into buffer)
static void parseSourcePhrase(const string& pt, Uint linenum, const string& line,
			      Uint bufsize, char buf[], vector<char*>& toks)
{
   const char* linep = line.c_str();
   char* bufp = buf;
   toks.assign(1, buf);

   for (; *linep && bufp < buf+bufsize; ++linep, ++bufp) {
      if (*linep == ' ') {
	 *bufp = 0;
	 if (isPrefix(PHRASE_TABLE_SEP, linep)) // !!! (see assert in main() above)
	    return;		// separator starts here, so we're done
	 toks.push_back(bufp+1);
      } else
	 *bufp = *linep;
   }

   if (!*linep)
      error(ETFatal, "Format error at line %d in phrase table %s - no separator found", linenum, pt.c_str());
   if (bufp == buf+bufsize)
      error(ETFatal, "Line %d too long for buffer size in phrase table %s", linenum, pt.c_str());
}

static string getTempName()
{
   static const char* tmpdir = getenv("TMPDIR");
   static const string path = tmpdir ? tmpdir : "/tmp";
   static const string name = "mix_phrasetables.XXXXXX";
   char tmp[path.size()+name.size()+2];
   strcpy(tmp,(path+"/"+name).c_str());
   if ( close(mkstemp(tmp)) )
      error(ETFatal, "Unable to get a temp file name using mkstemp(%s)", tmp);
   return tmp;
}



// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n", "s:", "w:", "wf:", "m:", "f:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   string wts_string, wts_file;

   if (arg_reader.getSwitch("w") && arg_reader.getSwitch("wf"))
      error(ETFatal, "Can't specify both -w and -wf");

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n", calc_norm);
   arg_reader.testAndSet("s", norm_smooth);
   arg_reader.testAndSet("w", wts_string);
   arg_reader.testAndSet("wf", wts_file);
   arg_reader.testAndSet("m", max_phrase_len);
   arg_reader.testAndSet("f", srcname);

   if (wts_file != "")
      gulpFile(wts_file.c_str(), wts_string);

   arg_reader.getVars(0, pts);

   split(wts_string, wts, " :\n");
   if (wts.empty())
      wts.assign(pts.size(), 1.0f / pts.size());

   if (wts.size() != pts.size())
      error(ETFatal, "Number of weights in weights string doesn't match number of phrasetables");
}
