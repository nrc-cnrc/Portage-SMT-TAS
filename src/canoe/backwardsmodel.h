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
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
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
         virtual void getStringPhrase(string &s, const Phrase &uPhrase);
         virtual vector<PhraseInfo *> 
            **createAllPhraseInfos(const vector<string> &src_sent, 
            const vector<MarkedTranslation> &marks, 
            bool alwaysTryDefault,
            vector<bool>* oovs = NULL);
	    
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
