/**
 * @author Eric Joanis
 * @file unal_feature.cc  Feature counting unaligned words
 *
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include "unal_feature.h"
#include "phrasetable.h"
#include "alignment_annotation.h"

Uint UnalFeature::count_unal_words(const PhraseInfo& phrase_info)
{
   const AlignmentAnnotation* a_ann = AlignmentAnnotation::get(phrase_info.annotations);
   if (!a_ann) return 0;

   const Uint src_len = phrase_info.src_words.size();
   const Uint tgt_len = phrase_info.phrase.size();

   const vector<vector<Uint > >* sets = AlignmentAnnotation::getAlignmentSets
      (a_ann->getAlignmentID(),src_len);
   if (sets->empty()) return 0;

   Uint result(0);
   for (Uint i = 0; i < type.size(); ++i)
      result += count_unal_words(src_len, tgt_len, *sets, type[i]);
   return result;
} // count_unal_words(phrase_info)

Uint UnalFeature::count_unal_words(Uint src_len, Uint tgt_len,
      const vector<vector<Uint> >& sets, UnalType cur_type)
{
   assert(src_len <= sets.size());
   Uint count(0);
   switch (cur_type)
   {
      case SrcAny:
         for ( Uint i = 0; i < src_len; ++i )
            if ( sets[i].empty() )
               ++count;
         return count;

      case SrcEdges: {
         Uint left_plus_right = 
            count_unal_words(src_len, tgt_len, sets, SrcLeft) +
            count_unal_words(src_len, tgt_len, sets, SrcRight);
         // In case all src words are unaligned, in which case Any == Edges ==
         // Left == Right == src_len.
         return min(left_plus_right, src_len);
      }

      case SrcLeft:
         for ( Uint i = 0; i < src_len; ++i )
            if ( sets[i].empty() )
               ++count;
            else break;
         return count;

      case SrcRight:
         for ( int i = src_len-1; i >= 0; --i )
            if ( sets[i].empty() )
               ++count;
            else break;
         return count;

      case TgtAny: {
         bool found[tgt_len+1];
         for ( Uint i = 0; i < tgt_len; ++i )
            found[i] = false;
         for ( Uint i = 0; i < src_len; ++i )
            for ( vector<Uint>::const_iterator it(sets[i].begin()), end(sets[i].end());
                  it != end; ++it ) {
               assert(*it <= tgt_len);
               found[*it] = true;
            }
         for ( Uint i = 0; i < tgt_len; ++i )
            if ( found[i] )
               ++count;
         return tgt_len - count;
      }

      case TgtEdges: {
         Uint left_plus_right = 
            count_unal_words(src_len, tgt_len, sets, TgtLeft) +
            count_unal_words(src_len, tgt_len, sets, TgtRight);
         // in case all tgt words are unaligned, in which case Any == Edges ==
         // Left == Right == tgt_len.
         return min(left_plus_right, src_len);
      }

      case TgtLeft: {
         Uint min(tgt_len);
         for ( Uint i = 0; i < src_len; ++i )
            for ( vector<Uint>::const_iterator it(sets[i].begin()), end(sets[i].end());
                  it != end; ++it )
               if ( *it < min ) min = *it;
         return min;
      }

      case TgtRight: {
         int max(-1);
         for ( Uint i = 0; i < src_len; ++i )
            for ( vector<Uint>::const_iterator it(sets[i].begin()), end(sets[i].end());
                  it != end; ++it )
               if ( int(*it) > max && *it < tgt_len ) max = *it;
         int count = int(tgt_len) - 1 - max;
         assert(count >= 0);
         assert(count <= int(tgt_len));
         return Uint(count);
      }

      default:
         assert(false);
   } // switch(cur_type)
} // count_unal_words(src_len,tgt_len,sets)


double UnalFeature::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   const AlignmentAnnotation* a_ann = AlignmentAnnotation::get(phrase_info.annotations);
   if (!a_ann) return 0;

   const Uint src_len = phrase_info.src_words.size();
   const Uint tgt_len = phrase_info.phrase.size();
   const Uint alignment = a_ann->getAlignmentID();
   /*
   if ( cache.size() <= alignment ) {
      cache.resize(alignmentVoc->size(), cache_not_set);
      assert(alignment < cache.size());
   }
   if ( cache[alignment] == cache_not_set )
      cache[alignment] = count_unal_words(phrase_info);
   */

   Cache::iterator res = cache.find(CacheKey(src_len, tgt_len, alignment));
   if ( res == cache.end() ) {
      Uint count = count_unal_words(phrase_info);
      res = cache.insert(make_pair(CacheKey(src_len, tgt_len, alignment), count)).first;
      assert(res->second == count);
   }
   return 0.0 - double(res->second);
}

UnalFeature::UnalFeature(const string& name)
{
   vector<string> types;
   split(name, types, "+");
   if ( types.empty() )
      error(ETFatal, "Empty unal feature type.");
   for ( Uint i = 0; i < types.size(); ++i ) {
      if ( types[i] == "any" ) {
         type.push_back(SrcAny);
         type.push_back(TgtAny);
      }
      else if ( types[i] == "edges" ) {
         type.push_back(SrcEdges);
         type.push_back(TgtEdges);
      }
      else if ( types[i] == "left" ) {
         type.push_back(SrcLeft);
         type.push_back(TgtLeft);
      }
      else if ( types[i] == "right" ) {
         type.push_back(SrcRight);
         type.push_back(TgtRight);
      }
      else if ( types[i] == "srcany" )
         type.push_back(SrcAny);
      else if ( types[i] == "srcedges" ) 
         type.push_back(SrcEdges);
      else if ( types[i] == "srcleft" ) 
         type.push_back(SrcLeft);
      else if ( types[i] == "srcright" ) 
         type.push_back(SrcRight);
      else if ( types[i] == "tgtany" )
         type.push_back(TgtAny);
      else if ( types[i] == "tgtedges" )
         type.push_back(TgtEdges);
      else if ( types[i] == "tgtleft" )
         type.push_back(TgtLeft);
      else if ( types[i] == "tgtright" )
         type.push_back(TgtRight);
      else
         error(ETFatal, "Unknown unal feature type: %s",
            types.size() > 1 ? (types[i] + " in " + name).c_str() : types[i].c_str());
   }
}
