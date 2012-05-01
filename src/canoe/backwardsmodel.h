/**
 * @author Aaron Tikuisis
 * @file backwardsmodel.h  This file contains the declaration of the
 * BackwardsModelGenerator class, used to create BasicModel objects to decode
 * phrases backwards.
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

#include "basicmodel.h"
#include <string>
#include <vector>

using namespace std;

namespace Portage
{
   /**
    * See BasicModelGenerator for documentation.
    */
   class BackwardsModelGenerator: public BasicModelGenerator
   {
      protected:
         virtual string getStringPhrase(const Phrase &uPhrase) const;
         virtual vector<PhraseInfo *>** createAllPhraseInfos(
               const newSrcSentInfo& info,
               bool alwaysTryDefault);

      public:
         /**
          * Constructor.
          * @param c  Global canoe configuration object.
          */
         BackwardsModelGenerator(const CanoeConfig& c);

         /**
          * Constructor.
          * @param c          Global canoe configuration object.
          * @param src_sents  The source sentences, used to determine the phrases to limit to (if c.loadFirst is false).
          * @param marks      All the marked translations.
          */
         BackwardsModelGenerator(const CanoeConfig& c,
                                 const vector< vector<string> > &src_sents,
                                 const vector< vector<MarkedTranslation> > &marks);

         /// Destructor.
         virtual ~BackwardsModelGenerator();
   }; // BackwardsModelGenerator
} // Portage
