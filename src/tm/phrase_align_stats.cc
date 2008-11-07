/**
 * @author George Foster
 * @file phrase_align_stats.cc  Program that computes statistics on phrase
 * alignments produced by phrase_tm_align.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <map>
#include <file_utils.h>
#include <gfstats.h>
#include <printCopyright.h>
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
phrase_align_stats [-v]  [alignfile [statsfile]]\n\
\n\
Compute statistics over a set of phrase alignments produced by phrase_tm_align.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string in_file;
static Uint lineno = 0;

static void getArgs(int argc, char* argv[]);

/**
 * Find the next alignment range in a string, in format [beg, end).
 * @param s the string
 * @param pos position in s to start looking
 * @param beg if successful, set to range begin
 * @param end if successful, set to range end
 * @param loc if successful, set to positions of init [ and final )
 * @return true on success
 */
bool findNextAlignRange(const string& s, Uint pos, Uint& beg, Uint& end, pair<Uint,Uint>& loc)
			
{
   Uint i, j, k;
   for (i = pos; i < s.length(); ++i)
      if (s[i] == '[')
	 break;
   if (i == s.length())
      return false;
   
   for (j = i+1; j < s.length(); ++j)
      if (s[j] == ',')
	 break;
   if (j == s.length() || j+1 == s.length())
      return false;

   string begstr = s.substr(i+1, j-(i+1));
   if (!conv(begstr, beg))
      return findNextAlignRange(s, pos+1, beg, end, loc);

   if (s[j+1] != ' ')
      return findNextAlignRange(s, j+1, beg, end, loc);

   for (k = j+2; k < s.length(); ++k)
      if (s[k] == ')')
	 break;
   if (k == s.length())
      return false;
   
   string endstr = s.substr(j+2, k-(j+2));
   if (!conv(endstr, end))
      return findNextAlignRange(s, j+2, beg, end, loc);
   
   loc.first = i;
   loc.second = k;
   return true;
}

/// Represent an aligned target phrase.
struct PhraseInfo
{
   Uint tgt_len;
   Uint src_beg;
   Uint src_end;
   /**
    * Default constructor.
    * @param tgt_len
    * @param src_beg
    * @param src_end
    */
   PhraseInfo(Uint tgt_len, Uint src_beg, Uint src_end) : 
      tgt_len(tgt_len), src_beg(src_beg), src_end(src_end) {}
};

/**
 * Parse alignment format.
 * @param srclen
 * @param line
 * @param phrase_infos
 */
void parseAlignment(Uint srclen, const string& line, vector<PhraseInfo>& phrase_infos)
{
   phrase_infos.clear();

   Uint beg, end, tbeg = 0;
   pair<Uint,Uint> loc;
   vector<string> toks;

   Uint pos = 0;
   while (findNextAlignRange(line, pos, beg, end, loc)) {

      if (phrase_infos.size()) {
	 toks.clear();
	 string phrase = line.substr(tbeg, loc.first-2-tbeg);
	 phrase_infos.back().tgt_len = split(phrase, toks);
      }

      phrase_infos.push_back(PhraseInfo(0, beg, end));
      pos = loc.second+1;
      tbeg = loc.second+3;
   }

   if (phrase_infos.size()) {
      toks.clear();
      string phrase = line.substr(tbeg, loc.first-2-tbeg);
      phrase_infos.back().tgt_len = split(phrase, toks);
   }

}


static map<Uint,Uint> nullcounts_by_srclen;
static map<Uint,Uint> counts_by_srclen;
static vector<Uint> source_phrase_lengths;
static vector<Uint> target_phrase_lengths;
static vector<Uint> word_displacements;
static Uint cnt = 0;
static Uint nullcount = 0;

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "phrase_align_stats");
   getArgs(argc, argv);
   iSafeMagicStream is(in_file.size() ? in_file : "-");

   // collect stats
   
   string line;
   while (getline(is, line)) {
      ++lineno;

      vector<string> toks;
      Uint srclen = split(line, toks);
      ++counts_by_srclen[srclen];
      ++cnt;

      if (!(getline(is, line)))
	 error(ETFatal, "alignment missing at end of file");

      if (line == "") {
	 ++nullcount;
	 ++nullcounts_by_srclen[srclen];
	 continue;
      }
      
      vector<PhraseInfo> phrase_infos;
      parseAlignment(srclen, line, phrase_infos);

      for (vector<PhraseInfo>::const_iterator it = phrase_infos.begin(); 
	   it != phrase_infos.end(); ++it) {

	 source_phrase_lengths.push_back(it->src_end - it->src_beg);
	 target_phrase_lengths.push_back(it->tgt_len);

	 int e = it == phrase_infos.begin() ? 0 : (it-1)->src_end;
	 int b = it->src_beg;
	 
	 word_displacements.push_back(abs(b-e));
      }
   }

   // write stats

   cout << cnt-nullcount << "/" << cnt 
	<< " (" << 100.0*(cnt-nullcount)/(double)cnt << "%) sentences successfully aligned" << endl;
   cout << "number of source phrases = " << source_phrase_lengths.size()
	<< ", number of target phrases = " << target_phrase_lengths.size() << endl;

   cout << "source phrase length: " 
	<< "avg = " << mean(source_phrase_lengths.begin(), source_phrase_lengths.end())
	<< ", sdev = " << sdev(source_phrase_lengths.begin(), source_phrase_lengths.end()) << endl;

   cout << "target phrase length: " 
	<< "avg = " << mean(target_phrase_lengths.begin(), target_phrase_lengths.end())
	<< ", sdev = " << sdev(target_phrase_lengths.begin(), target_phrase_lengths.end()) << endl;

   cout << "word displacement (distortion): " 
	<< "avg = " << mean(word_displacements.begin(), word_displacements.end())
	<< ", sdev = " << sdev(word_displacements.begin(), word_displacements.end()) << endl;

   // for (map<Uint,Uint>::const_iterator it = counts_by_srclen.begin(); 
   //    it != counts_by_srclen.end(); ++it) {
   //       os << it->first << " " << (nullcounts_by_srclen[it->first] / (double)it->second) << endl;
   // }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   
   arg_reader.testAndSet(0, "infile", in_file);
}   
