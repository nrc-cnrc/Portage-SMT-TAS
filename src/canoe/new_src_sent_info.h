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

namespace Portage {

using namespace std;

// Forward declaration
class PhraseInfo;

/**
 * Groups all the information about a source sentence needed by a decoder
 * feature.  This class is to be used by featurefunction::newSrcSent
 */
struct newSrcSentInfo {

   /// A unique index for source sentences as internally processed by this
   /// instance of canoe.  Must correspond sequentially to the order in which
   /// that instance of canoe has processed each sentence.
   Uint internal_src_sent_seq;

   /// The external source sentence ID, typically the line number in the input
   /// file to, say, canoe-parallel.sh.  Used by to create output file names.
   /// May be used by models to select the appropriate line in any data files
   /// line-aligned with the global input.
   Uint external_src_sent_id;

   /// The source sentence.
   vector<string> src_sent;

   /// Marked translation options available.
   vector<MarkedTranslation> marks;

   /// Triangular array of the candidate target phrases.
   /// Filled by BasicModelGenerator::createModel.
   vector<PhraseInfo *>** potential_phrases;

   /// Target sentence (in training).
   /// Optional target sentence.
   /// Provided by the user of canoe.
   const vector<string>* tgt_sent;

   /// Optional uint representation of the target sentence.
   /// Filled by BasicModelGenerator::createModel.
   vector<Uint> tgt_sent_ids;

   /// Optional out-of-vocabulary
   /// Filled by BasicModelGenerator::createAllPhraseInfos.
   /// If not NULL, will be set to true for each position in src_sent that
   /// contains an out-of-vocabulary word.
   vector<bool>* oovs;

   /// Default constructor.
   newSrcSentInfo() { clear(); }

   /// Resets this new source sentence info to an empty state.
   void clear()
   {
      internal_src_sent_seq = 0;
      external_src_sent_id = 0;
      src_sent.clear();
      marks.clear();
      potential_phrases = NULL;
      tgt_sent          = NULL;
      tgt_sent_ids.clear();
      oovs              = NULL;
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

} // ends namespace Portage

#endif // ends __NEW_SOURCE_SENTENCE_INFO__H__

