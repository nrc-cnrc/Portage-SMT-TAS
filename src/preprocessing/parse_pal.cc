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
   }

   return true;
}

string& Portage::unParsePhraseAlign(const vector<PhrasePair>& phrase_pairs, string& s)
{
   ostringstream oss;
   for (Uint i = 0; i < phrase_pairs.size(); ++i)
      oss << i+1 << ':' 
	  << phrase_pairs[i].src_pos.first << '-' << phrase_pairs[i].src_pos.second-1 << ':' 
	  << phrase_pairs[i].tgt_pos.first << '-' << phrase_pairs[i].tgt_pos.second-1 << ':'
	  << (phrase_pairs[i].oov ? 'O' : 'I') << (i+1 < phrase_pairs.size() ? " " : "");
   return s = oss.str();
}

/// Callable entity to sort on source position in increasing order 
struct CompareSourcePositions {
   /**
    * Compares two PhrasePair
    * @param p1  left-hand side operand
    * @param p2  right-hand side operand
    * @return Returns true if the source position of p1 is before p2
    */
   bool operator()(const PhrasePair& p1, const PhrasePair& p2) {
      return p1.src_pos.first < p2.src_pos.first;
   }
};

/// Callable entity to sort on target position in increasing order
struct CompareTargetPositions {
   /**
    * Compares two PhrasePair
    * @param p1  left-hand side operand
    * @param p2  right-hand side operand
    * @return Returns true if the target position of p1 is before p2
    */
   bool operator()(const PhrasePair& p1, const PhrasePair& p2) {
      return p1.tgt_pos.first < p2.tgt_pos.first;
   }
};

void Portage::sortBySource(vector<PhrasePair>& phrase_pairs)
{
   sort(phrase_pairs.begin(), phrase_pairs.end(), CompareSourcePositions());
}


void Portage::sortByTarget(vector<PhrasePair>& phrase_pairs)
{
   sort(phrase_pairs.begin(), phrase_pairs.end(), CompareTargetPositions());
}
