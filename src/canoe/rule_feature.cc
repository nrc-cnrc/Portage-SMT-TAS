/**
 * @author Samuel Larkin
 * @file rule_feature.cc Implementation of a feature that handles a particular
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

#include "rule_feature.h"
#include "str_utils.h"
#include "basicmodel.h" // BasicModelGenerator*
#include <sstream>

const char* const RuleFeature:: name = "RuleFeature";

RuleFeature::RuleFeature(const BasicModelGenerator* const bmg, const string& args)
: bmg(bmg)
{
   cerr << "RuleFeature::RuleFeature: " << args << endl;
   vector<string> vargs;
   if (split(args, vargs, ":") != 2)  {
      error(ETFatal, "You must provide a class and a value for log zero when using a rule decoder feature");
   }
   class_name = vargs[0];
   if (!convT<double>(vargs[1].c_str(), log_zero)) {
      error(ETFatal, "The log zero value must be a real value");
   }

   ostringstream oss;
   oss << "RuleFeature for class: " << class_name << " with e: " << log_zero;
   description = oss.str();
}


void RuleFeature::newSrcSent(const newSrcSentInfo& nss_info)
{
   typedef vector<MarkedTranslation>::const_iterator MARK_IT;
   typedef vector<ruleInfo>::iterator  RULE_IT;

   rule_infos.clear();

   // Keep a copy of the marks that belongs to this feature's class
   const vector<MarkedTranslation>& marks = nss_info.marks;
   for (MARK_IT mark(marks.begin()); mark!=marks.end(); ++mark) {
      // This mark belongs to this feature function
      if (mark->class_name == class_name) {
         Phrase tgt_phrase;
         bmg->get_voc().index(mark->markString, tgt_phrase);

         RULE_IT rule = find(rule_infos.begin(), rule_infos.end(), mark->src_words);
         if (rule == rule_infos.end()) {
            rule_infos.push_back(ruleInfo(mark->src_words));
            rule_infos.back().addTgtPhraseInfo(tgt_phrase, mark->log_prob);
         }
         else {
            rule->addTgtPhraseInfo(tgt_phrase, mark->log_prob);
         }
      }
   }

   //print_rules(nss_info.external_src_sent_id);  // for debugging
}


double RuleFeature::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   typedef vector<ruleInfo>::iterator  RULE_IT;
   typedef vector<ruleInfo::tgtPhraseInfo>::iterator TGT_IT;

   // find the source phrase in our marked translations.
   RULE_IT rule = find(rule_infos.begin(), rule_infos.end(), phrase_info.src_words);
   if (rule != rule_infos.end()) {
      // now find a matching target phrase in our marked translations.
      TGT_IT tgt_info = find(rule->tgt_phrase_info.begin(), rule->tgt_phrase_info.end(), phrase_info.phrase);
      if (tgt_info != rule->tgt_phrase_info.end()) {
         return tgt_info->log_prob;
      }
      else {
         return log_zero;
      }
   }

   // If this phrase is not even marked, we have no opinion.
   return 0.0f;  // log(1.0)
}


double RuleFeature::futureScore(const PartialTranslation &trans)
{
   return 0.0f;
}


double RuleFeature::score(const PartialTranslation& pt)
{
   return precomputeFutureScore(*pt.lastPhrase);
}


Uint RuleFeature::computeRecombHash(const PartialTranslation &pt)
{
   return 0;
}


bool RuleFeature::isRecombinable(const PartialTranslation &pt1,
                                 const PartialTranslation &pt2)
{
   return true;
}


void RuleFeature::print_rules(Uint src_id) const
{
   typedef vector<ruleInfo>::const_iterator  RULE_IT;

   if (!rule_infos.empty()) {
      fprintf(stderr, "%s rules for sentence %d are: \n", class_name.c_str(), src_id);
      for_each(rule_infos.begin(), rule_infos.end(), mem_fun_ref(&ruleInfo::print));
   }
   else {
      fprintf(stderr, "No %s rule for sentence %d\n", class_name.c_str(), src_id);
   }
}

