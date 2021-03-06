/**
 * @author Aaron Tikuisis
 * @file phrasefinder.h  This file contains a declaration of the PhraseFinder
 * class, which abstracts the finding of a set of phrases which can be added to
 * a partial translation.  It also contains the declaration of
 * RangePhraseFinder, which in practice is the only PhraseFinder we should be
 * using.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef PHRASEFINDER_H
#define PHRASEFINDER_H

#include "canoe_general.h"
#include <vector>

using namespace std;

namespace Portage
{
   class BasicModel;
   class PhraseInfo;
   class PartialTranslation;

   /**
    * An abstraction of the finding of phrases that can be added to a partial
    * translation.
    */
   class PhraseFinder
   {
      public:
         /**
          * Destructor.
          */
         virtual ~PhraseFinder() {};

         /**
          * Finds all the phrase options that may be added to the given
          * partial translation.
          * @param p  A vector into which all the pointers to phrases that
          *           can be added to t are placed.
          * @param t  The partial translation to find phrase options for.
          */
         virtual void findPhrases(vector<PhraseInfo *> &p,
                                  PartialTranslation &t) = 0;
   }; // PhraseFinder

   /**
    * Finds phrases using a set of available ranges; this is the only
    * PhraseFinder that we should be using in practice.
    */
   class RangePhraseFinder: public PhraseFinder
   {
      private:

         BasicModel* model;

         /**
          * The phrases, organized by the range that they cover.
          * The (i, j)-th entry of phrases should contain all the phrases
          * whose range is [i, i + j + 1).
          */
         vector<PhraseInfo *> **phrases;

         /// The length of the source sentence.
         Uint sentLength;

         /// The maximum distortion distance.
         int distLimit;

         /// The maximum itg distortion distance.
         int itgLimit;

         /// Whether to use the Simple distortion limit definition
         bool distLimitSimple;

         /// Whether to use the extended distortion limit definition
         bool distLimitExt;

         /// Whether to allow swapping continguous phrases, on top of whatever
         /// distLimit allows.
         bool distPhraseSwap;

         /// Whether to enforce an ITG constraint
         bool distLimitITG;

      public:

         /**
          * Creates a new RangePhraseFinder using the given phrases.
          * @param phrases       A triangular array of all the phrase
          *                      options available organized by source range
          *                      covered.  The (i, j)-th entry of the array
          *                      contains all the translation options for
          *                      the source range [i, i + j + 1).
          * @param model         The current BasicModel used by the decoder
          * @param model.getSourceLength()  The length of the source sentence that
          *                      phrases are coming from.
          * @param model.c->distLimit     The maximum distortion distance allowed
          *                      between two phrases.  NO_MAX_DISTORTION
          *                      (default) indicates no limit.
          * @param model.c->itgLimit      The maximum distance we can travel from the
          *                      top of the shift-reduce stack in ITG mode
          * @param model.c->distLimitSimple Whether to use the simple distortion limit
          *                      definition
          * @param model.c->distLimitExt  Whether to use the extended distortion limit
          *                      definition
          * @param model.c->distPhraseSwap Whether to allow swapping continguous
          *                      phrases, on top of whatever distLimit allows.
          * @param model.c->distLimitITG  Whether to use ITG constraints
          */
         RangePhraseFinder(vector<PhraseInfo *> **phrases, BasicModel& model);

         virtual void findPhrases(vector<PhraseInfo *> &p,
                                  PartialTranslation &t);
   }; // RangePhraseFinder

} // Portage

#endif // PHRASEFINDER_H
