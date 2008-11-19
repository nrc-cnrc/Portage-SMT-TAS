/**
 * @author Samuel Larkin
 * @file rule_feature.h  Definition of a feature that handles a particular
 *                       class of rule.
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef __RULE_FEATURE_H__
#define __RULE_FEATURE_H__

#include "decoder_feature.h"
#include <iterator> // for ostream_iterator

namespace Portage {

class RuleFeature : public DecoderFeature
{
   private:
      /**
       * Structure that holds all target phrases / probs for a particular source phrase.
       */
      struct ruleInfo {
         /**
          * Defines the candidate target phrase and its prob.
          * For each rule, multiple candidate can be defined.
          */
         struct tgtPhraseInfo {
            Phrase target_phrase;  ///< target phrase prob
            double log_prob;       ///< target phrase log_prob

            /**
             * Default constructor.
             * @param target_phrase  target phrase
             * @param log_prob       log_prob for this target phrase
             */
            tgtPhraseInfo(const Phrase& target_phrase, const double log_prob)
            : target_phrase(target_phrase)
            , log_prob(log_prob)
            { }

            /**
             * Allows to search for a tgtPhraseInfo based on the target phrase.
             * @param other  other target phrase to compare with.
             * @return Returns true if the target phrases are the same.
             */
            bool operator==(const Phrase& other) const {
               return target_phrase == other;
            }
         };

         /// This rule is for source phrase X.
         Range srcPhrase;
         /// What are the possible target phrases/probs for X.
         vector<tgtPhraseInfo> tgt_phrase_info;

         /**
          * Default constructor.
          * @param srcPhrase  source phrase
          */
         ruleInfo(const Range& srcPhrase)
         : srcPhrase(srcPhrase)
         {}

         /**
          * Helper function to add a target phrase info to this source phrase.
          * @param target_phrase  a target phrase
          * @param log_prob       log_prob for this target phrase
          */
         void addTgtPhraseInfo(const Phrase& target_phrase, const double log_prob) {
            tgt_phrase_info.push_back(tgtPhraseInfo(target_phrase, log_prob));
         }

         /**
          * Allows to search for a ruleInfo based on the source phrase.
          * @param other  other source phrase to compare with.
          * @return Returns true if the source phrases are the same.
          */
         bool operator==(const Range& other) const {
            return srcPhrase == other;
         }

         /// Prints the rule.
         /// For debugging purpous.
         void print() const {
            cerr << "-+- " << srcPhrase.toString() << endl;
            for (Uint i(0); i<tgt_phrase_info.size(); ++i) {
               cerr << " +- (" << tgt_phrase_info[i].log_prob << ") - ";
               const Phrase& p = tgt_phrase_info[i].target_phrase;
               copy(p.begin(), p.end(), ostream_iterator<Uint>(cerr, " "));
               cerr << endl;
            }
         }
      };

      /// Structure to keep track of all related rules for the current source sentence.
      vector<ruleInfo> rule_infos;
      /// name of the class handled by this decoder feature.
      string class_name;
      /// Epsilon value when we have no opinion.
      double log_zero;
      /// We need it to convert the string tgt phrase to Uint tgt phrase.
      const BasicModelGenerator* const bmg;

   public:
      static const char* const name;

   private:
      void print_rules(Uint src_id) const;

   public:
      /**
       * Default constructor.
       * @param bmg
       * @param args  arguments to this decoder feature which sould be of the form
       *              class_name:epsilon
       */
      RuleFeature(const BasicModelGenerator* const bmg, const string& args);

      virtual void newSrcSent(const newSrcSentInfo& new_src_sent_info);
      virtual double precomputeFutureScore(const PhraseInfo& phrase_info);
      virtual double futureScore(const PartialTranslation &trans);
      virtual double score(const PartialTranslation& pt);
      virtual Uint computeRecombHash(const PartialTranslation &pt);
      virtual bool isRecombinable(const PartialTranslation &pt1,
                                  const PartialTranslation &pt2);
};  // ends calss rule

} // ends namespace Portage

#endif // ends __RULE_FEATURE_H__

