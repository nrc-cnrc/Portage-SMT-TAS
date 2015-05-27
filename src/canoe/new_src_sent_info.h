/**
 * @author Samuel Larkin
 * @file new_src_sent_info.h
 * Groups all the information about a source sentence needed by a decoder feature.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef __NEW_SOURCE_SENTENCE_INFO__H__
#define __NEW_SOURCE_SENTENCE_INFO__H__

#include "portage_defs.h"
#include "marked_translation.h"
#include "voc.h"
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>


namespace Portage {

using namespace std;

// Forward declaration
class PhraseInfo;
class BasicModel;

/// A SrcWall describes an occurrence of <wall name="name"/> in the input sentence.
struct SrcWall {
   Uint pos;
   string name;
   SrcWall() : pos(0) {}
   explicit SrcWall(Uint pos, const string& name = "") : pos(pos), name(name) {}
};
/// A SrcZone describes a section of an input sentence marked with <zone name="name"> blah </zone>
struct SrcZone {
   Range range;
   string name;
   SrcZone() {}
   explicit SrcZone(Range range, const string& name = "") : range(range), name(name) {}
};
/// A SrcLocalWall describes an occurrence of <localwall name="name"/> inside a zone.
struct SrcLocalWall {
   Uint pos;
   Range zone;
   string name;
   SrcLocalWall() {}
   explicit SrcLocalWall(Uint pos, const string& name = "") :
      pos(pos), zone(0,0), name(name) {}
};

inline ostream& operator<<(ostream& o, const SrcWall& w) { o << w.pos; return o; }
inline ostream& operator<<(ostream& o, const SrcZone& z) { o << z.range; return o; }
inline ostream& operator<<(ostream& o, const SrcLocalWall& lw) {
   o << "[" << lw.zone.start << "|" << lw.pos << "|" << lw.zone.end << ")";
   return o;
}

/**
 * Groups all the information about a source sentence needed by a decoder
 * feature.  This class is to be used by featurefunction::newSrcSent
 */
struct newSrcSentInfo {

   /// The BasicModel for this source sentence, as soon as it's known.
   BasicModel* model;

   /// A unique index for source sentences as internally processed by this
   /// instance of canoe.  Must correspond sequentially to the order in which
   /// that instance of canoe has processed each sentence.
   Uint internal_src_sent_seq;

   /// The external source sentence ID, typically the line number in the input
   /// file to, say, canoe-parallel.sh.  Used to create output file names.
   /// May be used by models to select the appropriate line in any data files
   /// line-aligned with the global input. Zero-based.
   Uint external_src_sent_id;

   /// The source sentence.
   vector<string> src_sent;

   /// Marked translation options available.
   vector<MarkedTranslation> marks;

   /// Optional tags for src sent (either empty or one per token in src_sent).
   vector<string> src_sent_tags;

   /// Triangular array of the candidate target phrases.
   /// Filled by BasicModelGenerator::createModel.
   vector<PhraseInfo *>** potential_phrases;

   /// Optional target sentence, for Levenshtein and NGramMatch features.
   /// Provided by the user of canoe.
   const vector<string>* tgt_sent;

   /// Optional Uint representation of the target sentence.
   /// Filled by BasicModelGenerator::createModel.
   vector<Uint> tgt_sent_ids;

   /// Optional out-of-vocabulary
   /// Filled by BasicModelGenerator::createAllPhraseInfos.
   /// If not NULL, will be set to true for each position in src_sent that
   /// contains an out-of-vocabulary word.
   vector<bool>* oovs;

   /// List of walls
   vector<SrcWall> walls;
   /// List of zones
   vector<SrcZone> zones;
   /// List of local walls
   vector<SrcLocalWall> local_walls;

   /// Default constructor initializes everything to NULL/0/empty.
   newSrcSentInfo() { clear(); }
   /// Destructor NULLs all the pointers, just to be safe
   ~newSrcSentInfo() { clear(); }

   /// Resets this new source sentence info to an empty state.
   void clear()
   {
      model             = NULL;
      internal_src_sent_seq = 0;
      external_src_sent_id = 0;
      src_sent.clear();
      marks.clear();
      potential_phrases = NULL;
      tgt_sent          = NULL;
      tgt_sent_ids.clear();
      oovs              = NULL;
      walls.clear();
      zones.clear();
      local_walls.clear();
   }

   /**
    * Converts the string representation of the target sentence to a uint
    * representation of that target sentence.
    * @param voc  the vocabulary to use to convert the target sentence.
    */
   void convertTargetSentence(const Voc& voc)
   {
      if (tgt_sent != NULL) {
         voc.index(*tgt_sent, tgt_sent_ids);
      }
   }

   /// Prints the content of the newSrcSentInfo.
   /// @param out  stream to output the content of newSrcSentInfo.
   void print(ostream& out = cerr) const 
   {
      out << "internal_src_sent_seq: " << internal_src_sent_seq << endl;
      out << "external_src_sent_id: " << external_src_sent_id << endl;
      out << join(src_sent) << endl;
      if (tgt_sent != NULL)
         out << join(*tgt_sent) << endl;
      if (!tgt_sent_ids.empty())
         out << join(tgt_sent_ids) << endl;
      if (oovs != NULL)
         out << join(*oovs) << endl;
   }
}; // ends newSrcSentInfo

typedef boost::shared_ptr<newSrcSentInfo> PSrcSent;

// this is like a typedef, but it allows forward declarations.
struct VectorPSrcSent : public vector<PSrcSent> {};
//equivalent to: typedef vector<PSrcSent> VectorPSrcSent;

} // ends namespace Portage

#endif // ends __NEW_SOURCE_SENTENCE_INFO__H__

