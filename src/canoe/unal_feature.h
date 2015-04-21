/**
 * @author Eric Joanis
 * @file unal_feature.h  Feature counting unaligned words
 *
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2010, Her Majesty in Right of Canada
 */

#ifndef _UNAL_FEATURE_H_
#define _UNAL_FEATURE_H_

#include "decoder_feature.h"
#include <boost/unordered_map.hpp>

namespace Portage {

   class UnalFeature : public DecoderFeature {

   public:  // open up for use by SparseModel's lexicalized unal features

      /// Cache for the results, since they depend only on the alignments,
      /// which are expected to be frequently repeated.  The cache id, i.e.,
      /// offset into this vector is the alignment id from
      /// AlignmentAnnotation::alignmentVoc.
      struct CacheKey {
         Ushort src_len; Ushort tgt_len; Uint alignmentId;
         CacheKey(Ushort src_len, Ushort tgt_len, Uint alignmentId)
            : src_len(src_len), tgt_len(tgt_len), alignmentId(alignmentId) {}
         bool operator==(const CacheKey& x) const {
            return alignmentId == x.alignmentId &&
                   src_len == x.src_len &&
                   tgt_len == x.tgt_len;
         }
      };
      struct CacheKeyHash {
         std::size_t operator()(CacheKey const& k) const {
            return (std::size_t(k.alignmentId) << 0) +
                   (std::size_t(k.tgt_len)     << 22) +
                   (std::size_t(k.src_len)     << 27);
         }
      };

   private:

      typedef unordered_map<CacheKey, Uint, CacheKeyHash> Cache;
      Cache cache;
      //vector<Uint> cache;

      /// value signifying uninitialized cache entry
      //static const Uint cache_not_set = Uint(-1);

      /// The valid minimal types (others are composed by adding these together)
      enum UnalType { SrcAny, SrcEdges, SrcLeft, SrcRight,
                      TgtAny, TgtEdges, TgtLeft, TgtRight,
                      none };

      /// the type of this unal feature - multiple types mean add the values of each
      vector<UnalType> type;

      /// This method does the real calculations and depends on the type of unal feature
      /// selected.
      Uint count_unal_words(const PhraseInfo& phrase_info);

      /// Helper used once we parsed the alignment info from string to sets
      Uint count_unal_words(Uint src_len, Uint tgt_len,
            const vector<vector<Uint> >& sets, UnalType cur_type);

   public:
      /// constructor
      /// @param name  the name of the unal feature variant to instantiate
      ///              see canoe -h for documentation of valid names
      UnalFeature(const string& name);

      // the score and precomputed future score are the same, since they both
      // depend solely on the last phrase pair added.
      virtual double score(const PartialTranslation& pt) {
         return precomputeFutureScore(*(pt.lastPhrase));
      }

      // everything is recombinable since this feature only depends on the last phrase
      virtual Uint computeRecombHash(const PartialTranslation &pt) { return 0; }
      virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) {
         return true;
      }

      // the score and precomputed future score are the same, since they both
      // depend solely on the last phrase pair added.
      // this method gets the resutls from the subclass and caches them.
      virtual double precomputeFutureScore(const PhraseInfo& phrase_info);

      // the whole future score is computed phrase by phrase, so only
      // precomputeFutureScore() has to do any calculations.
      virtual double futureScore(const PartialTranslation &trans) { return 0; }

   }; // class UnalFeature

} // namespace Portage

#endif // _UNAL_FEATURE_H_
