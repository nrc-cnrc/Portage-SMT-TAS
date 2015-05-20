/**
 * @author George Foster
 * @file palstats.cc 
 * @brief Generate stats for a pal file.
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inist. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <gfstats.h>
#include <parse_pal.h>
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
palstats [-vs][-n n] src out pal\n\
\n\
Generate phrase alignment statistics for given source, output, and pal files,\n\
as produced by canoe -palign. <pal> and <out> may specify multiple alignments\n\
and output hypotheses for each source line in <src>; in this case, statistics\n\
are collected separately for each (source-sentence, alignment, output triple).\n\
See palview -h for more info about n-best formats.\n\
\n\
Note: canoe -palign produces files in which the output and alignment\n\
information are mixed. These need to be separated for input to palstats, which\n\
can be done by piping canoe's output through nbest2rescore.pl like this:\n\
\n\
   canoe -f canoe.cow -palign < src | nbest2rescore.pl -canoe -palout=pal > out\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-s    Write per-source-sentence stats instead of standard summary: for each\n\
      sentence triple, the number of src tokens, and the number of alignments\n\
      found.\n\
-n n  Analyze at most <n> alignments per source sentence, no matter how many\n\
      are available in <pal> [0 -> do all alignments].\n\
";

// globals

static bool verbose = false;
static bool src_stats = false;
static string srcfile;
static string outfile;
static string palfile;
static Uint n = 0;

static void getArgs(int argc, char* argv[]);

static void count(vector<Uint>& counts, Uint len)
{
   if (len >= counts.size())
      counts.resize(len+1);
   ++counts[len];
}

static double avgCounts(vector<Uint>& counts)
{
   Uint s = 0;
   Uint n = 0;
   for (Uint i = 0; i < counts.size(); ++i) {
      s += i * counts[i];
      n += counts[i];
   }
   return s / (double)n;
}

static void histout(const vector<Uint>& counts)
{
   cout << "histogram (count/len):";
   Uint last_non_zero = 0;
   for (Uint i = 0; i < counts.size(); ++i)
      if (counts[i]) last_non_zero = i;
   for (Uint i = 1; i < last_non_zero + 1; ++i)
      cout << " " << counts[i] << "/" << i;
   cout << endl;
}

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   PalReader reader(srcfile, outfile, palfile, verbose);

   vector<PhrasePair> pps;
   vector<Uint> src_len_counts(10, 0); // length -> # phrases with that length
   vector<Uint> tgt_len_counts(10, 0); // ""
   vector<Uint> distortions;    // distortion for each sentence
   vector<string> srctoks;
   Uint numfails = 0;

   Uint linenum = 0, hypcount = 0;
   string srcline, outline, palline;

   while (reader.readNext(srcline, outline, palline)) {

      ++linenum;
      if (n != 0 && reader.pos() > n)
         continue;
      ++hypcount;               // non-empty hyps

      if (!parsePhraseAlign(palline, pps))
         error(ETFatal, "bad format in pal file at line %d", linenum);

      if (pps.empty()) ++numfails;

      if (src_stats) {
         if ((n == 0 && reader.pos() == reader.nb()) || reader.pos() == n) {
            cout << splitZ(srcline, srctoks) << ' ' << reader.pos() - numfails << endl;
            numfails = 0;
         }
         continue;
      }

      sortByTarget(pps);

      distortions.push_back(0);
      Uint last_src_pos = 0, num_src_toks = 0;

      vector<PhrasePair>::iterator p;
      for (p = pps.begin(); p != pps.end(); ++p) {
         count(src_len_counts, p->src_pos.second - p->src_pos.first);
         count(tgt_len_counts, p->tgt_pos.second - p->tgt_pos.first);
         distortions.back() += p->src_pos.first >= last_src_pos ? 
            p->src_pos.first - last_src_pos : last_src_pos - p->src_pos.first;
         last_src_pos = p->src_pos.second;
         num_src_toks = max(num_src_toks, last_src_pos);
      }
      if (!pps.empty()) distortions.back() += num_src_toks - last_src_pos;
   }

   if (!src_stats) {
      cout << "align failures = " << numfails << "/" << hypcount << " (" 
           << numfails * 100.0 / hypcount << "%)" << endl;
      cout << "avg source phrase len = " << avgCounts(src_len_counts) << ", ";
      histout(src_len_counts);
      cout << "avg target phrase len = " << avgCounts(tgt_len_counts) << ", ";
      histout(tgt_len_counts);
      cout << "avg per-sentence total distortion cost = " 
           << mean(distortions.begin(), distortions.end())
           << ", sdev = " << sdev(distortions.begin(), distortions.end()) 
           << endl;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "s", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("s", src_stats);
   arg_reader.testAndSet("n", n);

   arg_reader.testAndSet(0, "srcfile", srcfile);
   arg_reader.testAndSet(1, "outfile", outfile);
   arg_reader.testAndSet(2, "palfile", palfile);
}
