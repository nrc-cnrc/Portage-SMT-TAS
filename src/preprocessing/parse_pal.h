/**
 * @author George Foster
 * @file parse_pal.h  Parse canoe's phrase-alignment files.
 * 
 * 
 * COMMENTS: 
 *
 * Parse canoe's phrase-alignment files, and manipulate the results. Includes
 * the PalReader class to facilitate coordination of source, output, and
 * alignment files.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#ifndef PARSE_PAL_H
#define PARSE_PAL_H

#include <vector>
#include <portage_defs.h>
#include <file_utils.h>

namespace Portage {

/**
 * Represent a phrase pair, and, as a special bonus, also an ne_id thingy which
 * has nothing to do with the phrase pair. Ugh.
 */
struct PhrasePair {
   pair<Uint,Uint> src_pos;	///< first, last+1
   pair<Uint,Uint> tgt_pos;	///< first, last+1
   bool oov;                    ///< oov flag set for the phrase
   int ne_id;                   ///< -1 means not an NE
   string alignment;            ///< optional green-format (with _ separator) alignment string
   /// Empty PhrasePair constructor.
   PhrasePair() : ne_id(-1) {}
   /**
    * Constructor.
    * @param src_beg  first source index
    * @param src_end  last source index
    * @param tgt_beg  first target index
    * @param tgt_end  last target index
    * @param oov      oov flag
    * @param ne_id    indicates if it's a named entity
    */
   PhrasePair(Uint src_beg, Uint src_end, Uint tgt_beg, Uint tgt_end, bool oov, int ne_id) :
      src_pos(make_pair(src_beg, src_end)), 
      tgt_pos(make_pair(tgt_beg, tgt_end)),
      oov(oov), ne_id(ne_id) {}

   /// string rep of source phrase.
   string& src(const vector<string>& src_toks, string& s) {
      s.clear();
      for (Uint i = src_pos.first; i < src_pos.second; ++i)
         s += src_toks[i] + (i + 1 == src_pos.second ? "" : " ");
      return s;
   }

   /// string rep of target phrase.
   string& tgt(const vector<string>& tgt_toks, string& s) {
      s.clear();
      for (Uint i = tgt_pos.first; i < tgt_pos.second; ++i)
         s += tgt_toks[i] + (i + 1 == tgt_pos.second ? "" : " ");
      return s;
   }
};

/**
 * Parse a line from a PAL file into a vector of PhrasePair structs. NB: the
 * ranges specified in the PAL file are of the form [b,e] - in phrase_pairs
 * these are converted to [b,e+1), ie 1 is added to all end markers.
 * @param line          line to be parsed
 * @param phrase_pairs  returned list of parsed PhrasePair
 * @return true iff the operation was successful
 */
bool parsePhraseAlign(const string& line, vector<PhrasePair>& phrase_pairs);

/**
 * Unparse a vector of PhrasePair structs into a string.
 * Reconstructs into s the phrase pairs relation
 * @param phrase_pairs  phrase pairs to reconstruct from
 * @param s  returned reconstructed phrase pair alignment
 * @return Returns s the reconstructed 
 */
string& unParsePhraseAlign(const vector<PhrasePair>& phrase_pairs, string& s);

/**
 * Sort phrases in given range in place by source position.
 * @param beg start index
 * @param end end index
 */
void sortBySource(vector<PhrasePair>::iterator beg, 
                  vector<PhrasePair>::iterator end);

/**
 * Sort phrases in given range in place by target position.
 * @param beg start index
 * @param end end index
 */
void sortByTarget(vector<PhrasePair>::iterator beg, 
                  vector<PhrasePair>::iterator end);

/**
 * Sort phrases in place by source position.
 * @param phrase_pairs  vector to sort
 */
inline void sortBySource(vector<PhrasePair>& phrase_pairs) {
   sortBySource(phrase_pairs.begin(), phrase_pairs.end());
}

/**
 * Sort phrases in place by target position.
 * @param phrase_pairs  vector to sort
 */
inline void sortByTarget(vector<PhrasePair>& phrase_pairs) {
   sortByTarget(phrase_pairs.begin(), phrase_pairs.end());
}

/**
 * Check contiguity of source phrases in given range. This assumes that
 * SortBySource() has been called on the range.
 * @param beg start index
 * @param end end index
 * @return true if no gaps exist between words in these phrases
 */
bool sourceContig(vector<PhrasePair>::iterator beg,
                  vector<PhrasePair>::iterator end);

/**
 * Check contiguity of source phrases. This assumes that
 * SortBySource() has been called on the vector.
 * @param beg start index
 * @param end end index
 * @return true if no gaps exist between words in these phrases
 */
inline bool sourceContig(vector<PhrasePair>& phrase_pairs) {
   if (phrase_pairs.size() && phrase_pairs[0].src_pos.first != 0) return false;
   return sourceContig(phrase_pairs.begin(), phrase_pairs.end());
}

/**
 * Check contiguity of target phrases in given range. This assumes that
 * SortByTarget() has been called on the range.
 * @param beg start index
 * @param end end index
 * @return true if no gaps exist between words in these phrases
 */
bool targetContig(vector<PhrasePair>::iterator beg,
                  vector<PhrasePair>::iterator end);

/**
 * Check contiguity of source phrases. This assumes that
 * SortByTarget() has been called on the vector.
 * @param beg start index
 * @param end end index
 * @return true if no gaps exist between words in these phrases
 */
inline bool targetContig(vector<PhrasePair>& phrase_pairs) {
   if (phrase_pairs.size() && phrase_pairs[0].tgt_pos.first != 0) return false;
   return targetContig(phrase_pairs.begin(), phrase_pairs.end());
}

/**
 * Class for reading src, output, phrase-alignment file triples.
 */
class PalReader 
{
   string srcfile;
   string outfile;
   string palfile;

   iSafeMagicStream src;
   iSafeMagicStream out;
   iSafeMagicStream pal;

   bool output_is_nbest;        // true iff out aligns to pal
   Uint n;                      // size of pal n-best lists

   Uint i;                      // next position in current nbest list

public:

   /**
    * Open source, output, phrase-alignment file triple for reading.
    * @param srcfile source file, one sentence per line
    * @param outfile output file; may contain one line per source line, or one
    * line per pal line (these differ if pal contains nbest lists).
    * @param palfile phrase-alignment file; may contain n-best lists, padded
    * with blanks to a fixed size - this is detected automatically.
    * @param verbose write verbose output to stderr if true
    */
   PalReader(const string& srcfile, const string& outfile, const string& palfile,
             bool verbose);

   /*
    * Destructor.
    */
   ~PalReader() {}

   /**
    * Read next triple, or return false if done. The contents of outline and
    * srcline may not be changed if reading n-best alignments.
    */
   bool readNext(string& srcline, string& outline, string&palline);

   /**
    * Return position in n-best list [1,n] of most-recently read triple (0 if
    * nothing has been read yet). 
    */
   Uint pos() {return i;}

   /**
    * Return size of n-best lists.
    */
   Uint nb() {return n;}
};

}

#endif
