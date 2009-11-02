/**
 * @author George Foster
 * @file parse_pal.h  Parse canoe's phrase-alignment files.
 * 
 * 
 * COMMENTS: 
 *
 * Parse canoe's phrase-alignment files.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#ifndef PARSE_PAL_H
#define PARSE_PAL_H

#include <vector>
#include <portage_defs.h>

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
   /// Empty PhrasePair constructor.
   PhrasePair() : ne_id(-1) {}
   /**
    * Constructor.
    * @param src_beg  first source index
    * @param src_end  last source index
    * @param tgt_beg  first target index
    * @param tgt_end  last target index
    * @param oov      oov flag to indicate if this PhrasePair is an OOv
    * @param ne_id    indicates if it's a Named Entity
    */
   PhrasePair(Uint src_beg, Uint src_end, Uint tgt_beg, Uint tgt_end, bool oov, int ne_id) :
      src_pos(make_pair(src_beg, src_end)), 
      tgt_pos(make_pair(tgt_beg, tgt_end)),
      oov(oov), ne_id(ne_id) {}
};

/**
 * Parse a line from a PAL file into a vector of PhrasePair structs. NB: the
 * ranges specified in the PAL file are of the form [b,e] - in phrase_pairs
 * these are converted to [b,e+1), ie 1 is added to all end markers.
 * @param line          line to be parsed
 * @param phrase_pairs  returned list of parsed PhrasePair
 * @return Returns true if the operation was successful
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
 * Sort phrases in place by source position.
 * @param phrase_pairs  vector to sort
 */
void sortBySource(vector<PhrasePair>& phrase_pairs); 

/**
 * Sort phrases in place by target position.
 * @param phrase_pairs  vector to sort
 */
void sortByTarget(vector<PhrasePair>& phrase_pairs); 

}

#endif
