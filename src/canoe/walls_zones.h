/**
 * @author Eric Joanis
 * @file walls_zones.h  Wall and zone decoder features.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2014, Her Majesty in Right of Canada
 */

#ifndef WALLS_ZONES_H
#define WALLS_ZONES_H

#include "distortionmodel.h"
#include "alignment_annotation.h"

namespace Portage {

class BasicModel;
static bool debug_walls_zones = false;

/// WallOrZone base class exists to provide a creator function,
/// because they have their own complicated mess of options.
class WallOrZone : public DistortionModel {
protected:
   /// The wall or zone name this feature considers ("" means all)
   string name;

public:
   /// Creator function for all types of walls and zones.
   static WallOrZone* Create(string featureName, string arg, bool fail);
}; // class WallOrZone

/// Return the max link found in sets[start] .. sets[end-1].
/// Returns -1 if no links are found.
static int maxLink(const vector<vector<Uint> >& sets, Uint start, Uint end) {
int max = -1;
for (Uint i = start; i < end; ++i) {
   for (Uint j = 0; j < sets[i].size(); ++j) {
      if (max < int(sets[i][j]))
            max = sets[i][j];
      }
   }
   return max;
}

/// Return the min link found in sets[start] .. sets[end-1].
/// INT_MAX if not links are found
static int minLink(const vector<vector<Uint> >& sets, Uint start, Uint end) {
   int min = INT_MAX;
   for (Uint i = start; i < end; ++i) {
      for (Uint j = 0; j < sets[i].size(); ++j) {
         if (min > int(sets[i][j]))
            min = sets[i][j];
      }
   }
   return min;
}

class StrictWallsFeature : public WallOrZone {
protected:
   /// The walls of the current sentence (only the ones this feature cares
   /// about, if name is non-empty).
   vector<SrcWall> walls;

   /// Return true iff the wall is crossed, i.e., some words before the wall
   /// are not covered yet, while some words after the wall are already covered.
   bool crossesWall(Uint wall_pos, const UintSet& cov) {
      return (!cov.empty() &&
              cov.front().start < wall_pos &&
              (cov.back().start > wall_pos || cov.back().end != sentLength));
   }

public:
   virtual double score(const PartialTranslation& pt) { return partialScore(pt) + precomputeFutureScore(*pt.lastPhrase); }
   virtual Uint computeRecombHash(const PartialTranslation &pt) { return 0; }
   virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) { return true; }
   virtual double futureScore(const PartialTranslation &pt) { return 0.0; }

   virtual void newSrcSent(const newSrcSentInfo& info)
   {
      DistortionModel::newSrcSent(info);
      if (name.empty()) {
         walls = info.walls;
      } else {
         walls.clear();
         for (Uint i = 0; i < info.walls.size(); ++i) {
            if (info.walls[i].name == name)
               walls.push_back(info.walls[i]);
         }
      }

      if (debug_walls_zones) {
         cerr << "Walls";
         if (!name.empty())
            cerr << "(" << name << ")";
         cerr << ": " << join(walls) << endl;
      }
   }

   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      // We implement this test here so violations of the wall constraint
      // are taken into account in the DP future score calculation. Also,
      // this is called even earlier than partialScore(), so pruning happens
      // earlier for the hard wall constraint in cube pruning decoding.
      double result = 0.0;
      for (Uint i = 0; i < walls.size(); ++i) {
         const Uint wall_pos = walls[i].pos;
         // A phrase that straddles the wall is a violation
         if (phrase_info.src_words.start < wall_pos && phrase_info.src_words.end > wall_pos)
            result -= 1.0;
      }
      return result;
   }

   virtual double partialScore(const PartialTranslation& pt)
   {
      // straddling phrases are detected in precomputeFutureScore(), so we
      // don't duplicate the calculation here.

      double result = 0.0;
      const UintSet &cov(pt.sourceWordsNotCovered);
      for (Uint i = 0; i < walls.size(); ++i) {
         // resulting coverage that has holes before the wall and words covered
         // after the wall is a violation
         if (crossesWall(walls[i].pos, cov))
            result -= 1.0;
      }
      return result;
   }
};

// Same implementation as StrictWallsFeature, except we don't look for phrases
// straddling the walls.
class LooseWallsFeature : public StrictWallsFeature {
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) { return 0.0; }
};

// WordStrictWallsFeature also share the same implementation as
// StrictWallsFeature, with precomputeFutureScore() adjusted to pay attention
// to word-alignments when looking at phrases straddling walls.
class WordStrictWallsFeature: public StrictWallsFeature {
public:
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      double result = 0.0;
      for (Uint i = 0; i < walls.size(); ++i) {
         const Uint wall_pos = walls[i].pos;
         if (phrase_info.src_words.start < wall_pos && phrase_info.src_words.end > wall_pos) {
            const Uint src_len = phrase_info.src_words.size();
            const vector<vector<Uint> >* sets =
               AlignmentAnnotation::getSets(phrase_info.annotations, src_len);

            // If there is no alignment annotation, we assume the phrase pair
            // is not compositional and count this as a violation.
            if (!sets || sets->empty()) { result -= 1.0; continue; }
            assert(sets->size() >= src_len);

            Uint boundary = wall_pos - phrase_info.src_words.start;
            if (maxLink(*sets, 0, boundary) >= minLink(*sets, boundary, src_len))
               result -= 1.0;
         }
      }
      return result;
   }
};

class StrictZonesFeature : public WallOrZone {
protected:
   /// The zones of the current sentence (only the ones this feature cares
   /// about, if name is non-empty).
   vector<SrcZone> zones;
   /// p_info->model is the BasicModel for this sentence, to detect future
   /// interactions with the distortion limit
   const newSrcSentInfo* p_info;

   /// Return true iff the zone was previously started, is not completed yet,
   /// and the current phrase has a word outside the zone.
   bool isLeavingIncompleteZone(Range zone, const PartialTranslation& pt) {
      // zone_started is true iff at least one word of the zone was
      // translated up to the *previous* hypothesis
      const bool zone_started = !isSubset(zone, pt.back->sourceWordsNotCovered);
      if (zone_started) {
         // zone_finished is true iff all words in the zone were translated
         // up the the *current* hypothesis
         const bool zone_finished = isDisjoint(zone, pt.sourceWordsNotCovered);
         if (!zone_finished) {
            // cur_out is true iff the current phrase has a word outside the zone
            Range src = pt.lastPhrase->src_words;
            const bool cur_out = (src.start < zone.start || src.end > zone.end);
            if (cur_out) {
               // Working outside of an incomplete zone is a violation
               return true;
            }
         }
      }
      return false;
   }

   /// Return true iff src straddles either end of zone, i.e., has some words
   /// inside zone and some words outside zone.
   bool isStraddlingZoneBoundary(Range zone, Range src) {
      return ((src.start < zone.start && src.end > zone.start) ||
              (src.start < zone.end && src.end > zone.end));
   }

public:
   virtual double score(const PartialTranslation& pt) { return partialScore(pt) + precomputeFutureScore(*pt.lastPhrase); }
   virtual Uint computeRecombHash(const PartialTranslation &pt) { return 0; }
   virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) { return true; }

   virtual void newSrcSent(const newSrcSentInfo& info)
   {
      DistortionModel::newSrcSent(info);
      if (name.empty()) {
         zones = info.zones;
      } else {
         zones.clear();
         for (Uint i = 0; i < info.zones.size(); ++i) {
            if (info.zones[i].name == name)
               zones.push_back(info.zones[i]);
         }
      }

      if (debug_walls_zones) {
         cerr << "Zones";
         if (!name.empty())
            cerr << "(" << name << ")";
         cerr << ": " << join(zones) << endl;
      }

      p_info = &info;
   }

   /// The zone's precomputed future score detects phrases straddling boundaries.
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      double result = 0.0;
      for (Uint i = 0; i < zones.size(); ++i) {
         // a phrase pair straddling either zone boundary is not allowed
         if (isStraddlingZoneBoundary(zones[i].range, phrase_info.src_words))
            result -= 1.0;
      }
      return result;
   }

   /// The zone's partial score detects the case where we're leaving a zone
   /// that was started but not completed yet.
   virtual double partialScore(const PartialTranslation& pt)
   {
      double result = 0.0;
      for (Uint i = 0; i < zones.size(); ++i) {
         // leaving a zone that was started is not allowed
         if (isLeavingIncompleteZone(zones[i].range, pt))
            result -= 1.0;
      }
      return result;
   }

   /// The future score detects the situation where we've entered a zone before
   /// completing the words that precede it, and won't be able to complete that
   /// zone without violation the distortion limit.
   /// Thanks go to Colin for pointing out this was necessary to avoid filling
   /// the decoder stacks with dead-end hypotheses when combining hard zones with
   /// hard distortion limit tests.
   virtual double futureScore(const PartialTranslation &pt);

};

// LooseZonesFeature shares the implementation with StrictZonesFeature, except
// we don't look for straddling phrases in precomputeFutureScore().
class LooseZonesFeature : public StrictZonesFeature {
public:
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) { return 0.0; }
};

// WordStrictZonesFeature shares some implementation with StrictZonesFeature, except
// the straddling phrase test is more complicated and can only partially be
// done in precomputeFutureScore(), so score() is not the sum of partialScore()
// and precomputeFutureScore().
//
// This implementation is ugly, with lots of duplicated code between
// precomputeFutureScore() and score(), but the two are subtly different and
// factoring it out in nice code would be quite complicated.
class WordStrictZonesFeature : public StrictZonesFeature {
   // detects cross links for cases where the src covers the whole zone and
   // some words outside the zone
   bool hasCrossLink(Range src, Range zone, const vector<vector<Uint> >* sets) {
      if (debug_walls_zones) {
         assert(src.start <= zone.start && src.end >= zone.end);
         assert(src != zone);
      }
      const Uint entry = zone.start - src.start;
      const Uint exit = zone.end - src.start;
      const int zoneMaxLink = maxLink(*sets, entry, exit);
      if (zoneMaxLink != -1) {
         const int zoneMinLink = minLink(*sets, entry, exit);
         for (Uint i = 0; i < entry; ++i) {
            for (Uint j = 0; j < (*sets)[i].size(); ++j) {
               const int link = (*sets)[i][j];
               if (link >= zoneMinLink && link <= zoneMaxLink)
                  return true;
            }
         }
      }
      return false;
   }
public:
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      double result = 0.0;
      const Range src = phrase_info.src_words;
      for (Uint i = 0; i < zones.size(); ++i) {
         const Range zone = zones[i].range;
         // a phrase pair straddling either zone boundary has to be checked for
         // compatible alignment
         if (isStraddlingZoneBoundary(zone, src)) {
            const Uint src_len = src.size();
            const vector<vector<Uint> >* sets =
               AlignmentAnnotation::getSets(phrase_info.annotations, src_len);

            // If there is no alignment annotation, we assume the phrase pair
            // is not compositional and count this straddling as a violation.
            if (!sets || sets->empty()) { result -= 1.0; continue; }
            assert(sets->size() >= src_len);

            // case 1: a single phrase covers the whole zone plus some words outside the zone
            if (src.start <= zone.start && src.end >= zone.end) {
               if (hasCrossLink(src, zone, sets))
                  result -= 1.0;
            }
            // case 2: this phrase straddles the beginning of the zone
            else if (src.start < zone.start) {
               if (debug_walls_zones)
                  assert(zone.start < src.end && src.end < zone.end);
               const Uint boundary = zone.start - src.start;
               // in precomp, we can only count the violation if this phrase
               // would be a problem used entering or exiting the zone, because
               // we don't know yet when it will be used.
               if (maxLink(*sets, boundary, src_len) >= minLink(*sets, 0, boundary) && // bad if exiting left
                   maxLink(*sets, 0, boundary) >= minLink(*sets, boundary, src_len))   // bad if entering left
                  result -= 1.0;
            }
            // case 3: this phrase straddles the end of the zone
            else {
               if (debug_walls_zones)
                  assert(zone.start < src.start && src.start < zone.end && zone.end < src.end);
               const Uint boundary = zone.end - src.start;
               if (maxLink(*sets, 0, boundary) >= minLink(*sets, boundary, src_len) && // bad if exiting right
                   maxLink(*sets, boundary, src_len) >= minLink(*sets, 0, boundary))   // bad if entering right
                  result -= 1.0;
            }
         }
      }
      return result;
   }

   virtual double score(const PartialTranslation& pt)
   {
      double result = partialScore(pt); // counts cases of leaving an incomplete zone. 
      const Range src = pt.lastPhrase->src_words;
      for (Uint i = 0; i < zones.size(); ++i) {
         const Range zone = zones[i].range;
         // a phrase pair straddling either zone boundary has to be checked for
         // compatible alignment
         if (isStraddlingZoneBoundary(zone, src)) {
            const Uint src_len = src.size();
            const vector<vector<Uint> >* sets =
               AlignmentAnnotation::getSets(pt.lastPhrase->annotations, src_len);

            // If there is no alignment annotation, we assume the phrase pair
            // is not compositional and count this straddling as a violation.
            if (!sets || sets->empty()) { result -= 1.0; continue; }
            assert(sets->size() >= src_len);

            // case 1: a single phrase covers the whole zone plus some words outside the zone
            if (src.start <= zone.start && src.end >= zone.end) {
               if (hasCrossLink(src, zone, sets))
                  result -= 1.0;
            }
            // case 2: this phrase straddles the beginning of the zone
            else if (src.start < zone.start) {
               if (debug_walls_zones)
                  assert(zone.start < src.end && src.end < zone.end);
               const Uint boundary = zone.start - src.start;
               if (!isDisjoint(zone, pt.sourceWordsNotCovered)) {
                  // entering from left
                  if (maxLink(*sets, 0, boundary) >= minLink(*sets, boundary, src_len))
                     result -= 1.0;
               } else {
                  // exiting left
                  if (maxLink(*sets, boundary, src_len) >= minLink(*sets, 0, boundary))
                     result -= 1.0;
               }
            }
            // case 3: this phrase straddles the end of the zone
            else {
               if (debug_walls_zones)
                  assert(zone.start < src.start && src.start < zone.end && zone.end < src.end);
               const Uint boundary = zone.end - src.start;
               if (isDisjoint(zone, pt.sourceWordsNotCovered)) {
                  // exiting right
                  if (maxLink(*sets, 0, boundary) >= minLink(*sets, boundary, src_len))
                     result -= 1.0;
               } else {
                  // entering from right
                  if (maxLink(*sets, boundary, src_len) >= minLink(*sets, 0, boundary))
                     result -= 1.0;
               }
            }
         }
      }
      return result;
   }
};

class StrictLocalWallsFeature : public WallOrZone {
protected:
   /// The local walls of the current sentence (only the ones this feature cares
   /// about, if name is non-empty).
   vector<SrcLocalWall> local_walls;

   /// Return true iff the wall is crossed, i.e., some words before the wall inside the zone
   /// are not covered yet, while some words after the wall inside the zone are already covered.
   bool crossesLocalWall(Uint wall_pos, Range zone, const UintSet& cov) {
      return (
         // at least one word before the wall inside the zone is not covered yet
         !isDisjoint(Range(zone.start, wall_pos), cov)
         &&
         // at least one word after the wall inside the zone is already covered
         !isSubset(Range(wall_pos,zone.end), cov)
      );
   }

public:
   virtual double score(const PartialTranslation& pt) { return partialScore(pt) + precomputeFutureScore(*pt.lastPhrase); }
   virtual Uint computeRecombHash(const PartialTranslation &pt) { return 0; }
   virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) { return true; }
   virtual double futureScore(const PartialTranslation &pt) { return 0.0; }

   virtual void newSrcSent(const newSrcSentInfo& info)
   {
      DistortionModel::newSrcSent(info);
      if (name.empty()) {
         local_walls = info.local_walls;
      } else {
         local_walls.clear();
         for (Uint i = 0; i < info.local_walls.size(); ++i) {
            if (info.local_walls[i].name == name)
               local_walls.push_back(info.local_walls[i]);
         }
      }

      if (debug_walls_zones) {
         cerr << "LocalWalls";
         if (!name.empty())
            cerr << "(" << name << ")";
         cerr << ": " << join(local_walls) << endl;
      }
   }

   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      // We implement this test here so violations of the wall constraint
      // are taken into account in the DP future score calculation. Also,
      // this is called even earlier than partialScore(), so pruning happens
      // earlier for the hard wall constraint in cube pruning decoding.
      double result = 0.0;
      for (Uint i = 0; i < local_walls.size(); ++i) {
         const Uint wall_pos = local_walls[i].pos;
         // A phrase that straddles the wall is a violation
         if (phrase_info.src_words.start < wall_pos && phrase_info.src_words.end > wall_pos)
            result -= 1.0;
      }
      return result;
   }

   virtual double partialScore(const PartialTranslation& pt)
   {
      // straddling phrases are detected in precomputeFutureScore(), so we
      // don't duplicate the calculation here.

      double result = 0.0;
      const UintSet &cov(pt.sourceWordsNotCovered);
      for (Uint i = 0; i < local_walls.size(); ++i) {
         // resulting coverage that has holes before the wall and words covered
         // after the wall is a violation
         if (crossesLocalWall(local_walls[i].pos, local_walls[i].zone, cov))
            result -= 1.0;
      }
      return result;
   }
};

// Same implementation as StrictLocalWallsFeature, except we don't look for phrases
// straddling the local walls.
class LooseLocalWallsFeature : public StrictLocalWallsFeature {
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) { return 0.0; }
};

// WordStrictLocalWallsFeature also share the same implementation as
// StrictLocalWallsFeature, with precomputeFutureScore() adjusted to pay attention
// to word-alignments when looking at phrases straddling local walls.
class WordStrictLocalWallsFeature: public StrictLocalWallsFeature {
public:
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      double result = 0.0;
      for (Uint i = 0; i < local_walls.size(); ++i) {
         const Uint wall_pos = local_walls[i].pos;
         const Range zone = local_walls[i].zone;
         if (phrase_info.src_words.start < wall_pos && phrase_info.src_words.end > wall_pos) {
            const Uint src_len = phrase_info.src_words.size();
            const vector<vector<Uint> >* sets =
               AlignmentAnnotation::getSets(phrase_info.annotations, src_len);

            // If there is no alignment annotation, we assume the phrase pair
            // is not compositional and count this as a violation.
            if (!sets || sets->empty()) { result -= 1.0; continue; }
            assert(sets->size() >= src_len);

            const Uint start = max(zone.start, phrase_info.src_words.start) - phrase_info.src_words.start;
            const Uint end = min(zone.end, phrase_info.src_words.end) - phrase_info.src_words.start;
            const Uint boundary = wall_pos - phrase_info.src_words.start;
            if (maxLink(*sets, start, boundary) >= minLink(*sets, boundary, end))
               result -= 1.0;
         }
      }
      return result;
   }
};


} // namespace Portage

#endif // WALLS_ZONES_H

