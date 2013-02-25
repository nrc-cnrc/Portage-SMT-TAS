/**
 * @author George Foster
 * @file parse_pal.cc  Implementation that parses canoe's phrase-alignment files.
 *
 *
 * COMMENTS:
 *
 * Parse canoe's phrase-alignment files.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include <sstream>
#include <str_utils.h>
#include "parse_pal.h"
#include <algorithm>

using namespace Portage;

/**
 * Parses a string to extract a range
 * @param range_string  string to parse
 * @param range         returned parsed range
 * @return Returns true if the operation was successful
 */
static bool parseRange(const string& range_string, pair<Uint,Uint>& range)
{
   vector<string> range_nums;
   if (split(range_string, range_nums, "-") != 2) {
      error(ETWarn, "expecting exactly 2 items in range: " + range_string);
      return false;
   }
   if (!conv(range_nums[0], range.first))
      error(ETWarn, "can't convert %s to int in range %s", range_nums[0].c_str(), range_string.c_str());
   if (!conv(range_nums[1], range.second))
      error(ETWarn, "can't convert %s to int in range %s", range_nums[1].c_str(), range_string.c_str());
   ++range.second;		// convert to end+1 convention

   return true;
}

bool Portage::parsePhraseAlign(const string& line, vector<PhrasePair>& phrase_pairs)
{
   phrase_pairs.clear();
   vector<string> phrase_pair_strings;
   split(line, phrase_pair_strings);
   phrase_pairs.resize(phrase_pair_strings.size());

   vector<string> pp_parts;
   for (Uint i = 0; i < phrase_pair_strings.size(); ++i) {
      pp_parts.clear();
      // index:sbeg-send:tbeg-tend:oov
      if (split(phrase_pair_strings[i], pp_parts, ":") < 3) {
	 error(ETWarn, "expecting at least 3 fields in phrase alignment: " + phrase_pair_strings[i]);
	 return false;
      }
      if (!parseRange(pp_parts[1], phrase_pairs[i].src_pos))
	 return false;
      if (!parseRange(pp_parts[2], phrase_pairs[i].tgt_pos))
	 return false;
      phrase_pairs[i].oov = pp_parts.size() >= 4 && pp_parts[3] == "O";
      phrase_pairs[i].alignment = (pp_parts.size() >= 5 ? pp_parts[4] : "");
   }

   return true;
}

string& Portage::unParsePhraseAlign(const vector<PhrasePair>& phrase_pairs, string& s)
{
   ostringstream oss;
   for (Uint i = 0; i < phrase_pairs.size(); ++i)
      oss << i+1
          << ':' << phrase_pairs[i].src_pos.first << '-' << phrase_pairs[i].src_pos.second-1
          << ':' << phrase_pairs[i].tgt_pos.first << '-' << phrase_pairs[i].tgt_pos.second-1
          << ':' << (phrase_pairs[i].oov ? 'O' : 'I')
          << ':' << phrase_pairs[i].alignment
          << (i+1 < phrase_pairs.size() ? " " : "");
   return s = oss.str();
}

struct CompareSourcePositions {
   bool operator()(const PhrasePair& p1, const PhrasePair& p2) {
      return p1.src_pos.first < p2.src_pos.first;
   }
};

struct CompareTargetPositions {
   bool operator()(const PhrasePair& p1, const PhrasePair& p2) {
      return p1.tgt_pos.first < p2.tgt_pos.first;
   }
};

void Portage::sortBySource(vector<PhrasePair>::iterator beg,
                           vector<PhrasePair>::iterator end)
{
   sort(beg, end, CompareSourcePositions());
}

void Portage::sortByTarget(vector<PhrasePair>::iterator beg,
                           vector<PhrasePair>::iterator end)
{
   sort(beg, end, CompareTargetPositions());
}

bool Portage::sourceContig(vector<PhrasePair>::iterator beg,
                           vector<PhrasePair>::iterator end)
{
   for (vector<PhrasePair>::iterator p = beg+1; p < end; ++p)
      if (p->src_pos.first != (p-1)->src_pos.second)
         return false;
   return true;
}

bool Portage::targetContig(vector<PhrasePair>::iterator beg,
                           vector<PhrasePair>::iterator end)
{
   for (vector<PhrasePair>::iterator p = beg+1; p < end; ++p)
      if (p->tgt_pos.first != (p-1)->tgt_pos.second)
         return false;
   return true;
}

PalReader::PalReader(const string& srcfile, const string& outfile, const string& palfile,
                     bool verbose) :
   srcfile(srcfile), outfile(outfile), palfile(palfile),
   src(srcfile), out(outfile), pal(palfile),
   output_is_nbest(false), n(1), i(0)
{
   if (srcfile == "-" || outfile == "-" || palfile == "-")
      error(ETFatal, "error: can't read src/out/pal files from stdin");

   Uint slines = countFileLines(srcfile);
   Uint olines = countFileLines(outfile);
   Uint plines = countFileLines(palfile);
   n = plines / slines;
   if (n * slines != plines)
      error(ETFatal, "number of lines in palfile %s must be an integer multiple of lines in srcfile %s",
            palfile.c_str(), srcfile.c_str());
   if (olines == plines)
      output_is_nbest = true;
   else if (olines != slines)
      error(ETFatal, "number of lines in outfile %s must match either srcfile %s or palfile %s",
            outfile.c_str(), srcfile.c_str(), palfile.c_str());

   if (verbose) {
      cerr << "reading source file " << srcfile << endl;
      cerr << "reading output file " << outfile << ", with "
           << (output_is_nbest ? n : 1) << "-best hyps " << endl;
      cerr << "reading phrase-alignment file " << palfile << ", with "
           << n << "-best alignments" << endl;
   }
}

bool PalReader::readNext(string& srcline, string& outline, string &palline)
{
   if (i == n) i = 0;
   if (i == 0)
      if (!getline(src, srcline))
         return false;
   if (i == 0 || output_is_nbest)
      if (!getline(out, outline))
         error(ETFatal, "outfile %s is too short", outfile.c_str());
   if (!getline(pal, palline))
      error(ETFatal, "palfile %s is too short", palfile.c_str());

   ++i;

   return true;
}
