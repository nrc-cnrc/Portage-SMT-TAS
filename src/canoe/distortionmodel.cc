/**
 * @author George Foster
 * @file distortionmodel.cc  Implementation of all related distortion model decoder feature.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "errors.h"
#include "distortionmodel.h"
#include "str_utils.h"
#include "phrasetable.h"
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace Portage;

/************************** DistortionModel **********************************/

void DistortionModel::splitNameAndArg(const string& name_and_arg,
                                      string& name,
                                      string& arg)
{
   vector<string> arg_split;
   split(name_and_arg, arg_split, "#", 2);
   name.assign(arg_split.empty() ? "" : arg_split[0]);
   arg.assign(arg_split.size() < 2 ? "" : arg_split[1]);
}

DistortionModel* DistortionModel::create(const string& name_and_arg, bool fail)
{
   DistortionModel* m = NULL;

   // Separate the model name and argument, introduced by # if present.
   string name, arg;
   splitNameAndArg(name_and_arg, name, arg);
   if (name == "WordDisplacement")
      m = new WordDisplacement();
   else if (name == "LeftDistance")
      m = new LeftDistance();
   else if (name == "ZeroInfo")
      m = new ZeroInfoDistortion();
   else if (name == "PhraseDisplacement")
      m = new PhraseDisplacement();
   else if (name == "fwd-lex")
      m = new FwdLexDistortion(arg);
   else if (name == "back-lex")
      m = new BackLexDistortion(arg);
   else if (name == "fwd-hlex")
      m = new FwdHierLDM(arg);
   else if (name == "back-hlex")
      m = new BackHierLDM(arg);
   else if (name == "back-fhlex")
      m = new BackFakeHierLDM(arg);
   else if (fail)
      error(ETFatal, "unknown distortion model: " + name);

   return m;
}

bool DistortionModel::respectsDistLimitCons(const UintSet& cov,
      Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
      const UintSet* resulting_cov)
{
   if ( distLimit == NO_MAX_DISTORTION ) return true;

   // distortion limit test 1: start of new phrase can't be too far from end of
   // last phrase
   if ( abs(int(last_phrase.end) - int(new_phrase.start)) > distLimit )
      return false;

   // distortion limit test 2: end of new phrase can't be too far from first
   // non-covered word - this is a conservative version as describe by Aaron's
   // original comment regarding his implementation of this test in
   // BasicModel::computeFutureScore():
   // To prevent distortion limit violations later, it is checked that the
   // distance from the current position to the first untranslated word is at
   // most the distortion limit and that the distance between ranges of
   // untranslated words is at most the distortion limit.  This is not an iff
   // condition - I am being more conservative and penalizing some partial
   // translations for which it is possible to complete without violating the
   // distortion limit.  I am confident however that there is an iff condition
   // that could be checked in O(pt.sourceWordsNotCovered.size()).
   assert (!cov.empty());
   if ( new_phrase.start != cov.front().start &&
        abs(int(new_phrase.end) - int(cov.front().start)) > distLimit )
      return false;

   // After this strict conservative test, we shouldn't need to check that
   // covered blocks are smaller than distLimit, since they can't even end
   // further away than distLimit from cov.front().start!  However, if this
   // test is combined with the phrase swapping test in an OR statement, we
   // still need to validate the jumps.

   UintSet next_cov;
   if ( resulting_cov == NULL ) {
      subRange(next_cov, cov, new_phrase);
      resulting_cov = &next_cov;
   }
   // Complete coverage is necessarily OK.
   if ( resulting_cov->empty() )
      return true;

   // Final case: make sure we can jump through all the covered blocks.
   int last_end = new_phrase.end;
   for ( Uint i(0), end(resulting_cov->size()); i < end; ++i ) {
      if ( abs(int((*resulting_cov)[i].start) - last_end) > distLimit )
         return false;
      last_end = (*resulting_cov)[i].end;
   }
   if ( int(sourceLength) - last_end > distLimit )
      return false;

   return true;
}

bool DistortionModel::respectsDistLimitExt(const UintSet& cov,
      Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
      const UintSet* resulting_cov)
{
   if ( distLimit == NO_MAX_DISTORTION ) return true;

   // distortion limit test 1: start of new phrase can't be too far from end of
   // last phrase
   if ( abs(int(last_phrase.end) - int(new_phrase.start)) > distLimit )
      return false;

   // distortion limit test 2: start of new phrase can't be too far from first
   // non-covered word
   assert (!cov.empty());
   if ( new_phrase.start > cov.front().start + distLimit )
      return false;

   // simple case: it's always OK to cover the first words in cov
   if ( new_phrase.start == cov.front().start )
      return true;

   // distortion limit test 3: when a new phrase starts within the limit but
   // ends past cov.front().start + distLimit, its end must be such that it's
   // still possible to finish covering cov without violating distLimit in
   // subsequent steps.  At least one successfull path must exist: To finish
   // covering cov, we need two non-covered positions within 1 + the distortion
   // limit from the end of the new phrase.
   if ( new_phrase.end > cov.front().start + distLimit ) {
      // case 3a: new phrase is too long, so it's not possible to have 2
      // non-covered postions in the jump back space
      if ( new_phrase.end - new_phrase.start + 1 > Uint(distLimit) )
         return false;
   }

   // resulting_cov is needed both for case 3b and the final case below, so
   // factor it out of the main case 3 if statement.
   UintSet next_cov;
   if ( resulting_cov == NULL ) {
      subRange(next_cov, cov, new_phrase);
      resulting_cov = &next_cov;
   }
   // Complete coverage is necessarily OK.
   if ( resulting_cov->empty() )
      return true;

   // This final case is cheaper to calculate than case 3b, so do it first.
   // Final case: make sure we can jump forward through all the covered blocks.
   Uint last_end = resulting_cov->front().end;
   for ( Uint i(1), end(resulting_cov->size()); i < end; ++i ) {
      if ( (*resulting_cov)[i].start > last_end + distLimit )
         return false;
      last_end = (*resulting_cov)[i].end;
   }

   // And now let's get back and finish distortion limit test 3:
   if ( new_phrase.end > cov.front().start + distLimit ) {
      // Case 3b: we need to actually count the number of free positions in the
      // jump back space.
      UintSet jump_back_space;
      assert(int(new_phrase.end)-distLimit-1 >= 0);
      intersectRange(jump_back_space, *resulting_cov,
                     Range(new_phrase.end-distLimit-1, new_phrase.start));
      if ( jump_back_space.empty() ||
           (jump_back_space.size() == 1 &&
              jump_back_space.front().end - jump_back_space.front().start < 2 ))
         return false;
   }
   if ( int(sourceLength) - int(last_end) > distLimit )
      return false;

   // Whew, we passed all tests.  Yay!
   return true;
}

bool DistortionModel::respectsDistLimitSimple(const UintSet& cov,
      Range new_phrase, int distLimit)
{
   if ( distLimit == NO_MAX_DISTORTION ) return true;
   assert(!cov.empty());
   return new_phrase.start <= cov.front().start + distLimit;
}

bool DistortionModel::respectsITG(int itgLimit, ShiftReducer* sr, Range lastphrase)
{
   assert(sr!=NULL);
   assert(sr->lBound() <= sr->start());
   assert(sr->rBound() >= sr->end());
   return
      lastphrase.start >= sr->lBound()
      && lastphrase.end <= sr->rBound()
      && (itgLimit==NO_MAX_ITG
          || lastphrase.start >= sr->end()
          || int(lastphrase.end) >= int(sr->start()) - itgLimit
          )
      && (itgLimit==NO_MAX_ITG
          || lastphrase.end <= sr->start()
          || int(lastphrase.start) <= int(sr->end()) + itgLimit
          )
      ;
}


/************************** WordDisplacement *********************************/

double WordDisplacement::score(const PartialTranslation& pt)
{
   double result = -(double)
      abs((int)pt.lastPhrase->src_words.start -
          (int)pt.back->lastPhrase->src_words.end);

   // Add distortion cost to end of sentence
   // Note: Other decoders don't do this (I think) but I decided to for
   // symmetry, as this is done for the first word in the sentence.
   // eg. 0 1 2 -> 0 2 1 and 0 1 2 -> 1 0 2 should have the same distortion
   // cost, which it doesn't the way other decoders do things.
   if (pt.sourceWordsNotCovered.empty()) {
      assert(pt.lastPhrase->src_words.end <= sentLength);
      result -= (double)(sentLength - pt.lastPhrase->src_words.end);
   }

   return result;

}

// Note: PhraseDisplacement relies on this implementation of partialScore(),
// so if any change is made here, class PhraseDisplacement will need to
// override partialScore().
double WordDisplacement::partialScore(const PartialTranslation& pt)
{
   // score() only uses the source range of the last phrase, so it does what
   // this function needs to do.
   return score(pt);
}

// Technically, this should capture the end of the source phrase aligned with
// the rightmost target phrase, but it doesn't have to be perfect, so I'm going
// with the way Aaron originally defined it.
// EJJ Nov 2007: it's trivial to do a bit better, so why not!?  I removed
// "return 0" for the hash, and used pt.lastPhrase->src_words.end instead.

Uint WordDisplacement::computeRecombHash(const PartialTranslation &pt)
{
   //return 0;
   return pt.lastPhrase->src_words.end;
}

bool WordDisplacement::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   return pt1.lastPhrase->src_words.end == pt2.lastPhrase->src_words.end;
}

double WordDisplacement::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   return 0;
}

double WordDisplacement::futureScore(const PartialTranslation &pt)
{
   // If the source sentence has been fully covered, we're done, so the jump to
   // the end of sentence has been incurred already, as well as all other
   // distortion costs.
   if (pt.sourceWordsNotCovered.empty()) return 0;

   double distScore = 0;
   Uint lastEnd = pt.lastPhrase->src_words.end;
   for (UintSet::const_iterator it = pt.sourceWordsNotCovered.begin();
        it != pt.sourceWordsNotCovered.end(); it++)
   {
      assert(it->start < it->end);
      if (it->end > sentLength) cerr << it->end << " " << sentLength << endl;
      assert(it->end <= sentLength);
      distScore -= abs((int)lastEnd - (int)it->start);
      lastEnd = it->end;
   } // for
   distScore -= (sentLength - lastEnd);

   return distScore;
}

/************************** LeftDistance ******************************/

double LeftDistance::score(const PartialTranslation& pt)
{
   if (pt.sourceWordsNotCovered.empty()) return 0;
   if (pt.sourceWordsNotCovered[0].start > pt.lastPhrase->src_words.start) return 0;
   return -double(pt.lastPhrase->src_words.start - pt.sourceWordsNotCovered[0].start);
}

double LeftDistance::partialScore(const PartialTranslation& trans)
{
   return score(trans);
}

Uint LeftDistance::computeRecombHash(const PartialTranslation &pt)
{
   return 0;
}

bool LeftDistance::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   // This class does not need to test recombinability, because it depends only
   // on the coverage set being the same, which is already checked by
   // BasicModel::isRecombinable().
   return true;
}

double LeftDistance::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   return 0.0;
}

double LeftDistance::futureScore(const PartialTranslation &trans)
{
   return 0.0;
}

/************************** PhraseDisplacement ******************************/

double PhraseDisplacement::score(const PartialTranslation& pt)
{
   double result = 0.0;
   if ( pt.lastPhrase->src_words.start != pt.back->lastPhrase->src_words.end )
      result += -1.0;

   // Add distortion cost to end of sentence
   if (pt.sourceWordsNotCovered.empty()) {
      assert(pt.lastPhrase->src_words.end <= sentLength);
      if ( pt.lastPhrase->src_words.end != sentLength )
         result += -1.0;
   }

   return result;
}

double PhraseDisplacement::futureScore(const PartialTranslation &pt)
{
   if (pt.sourceWordsNotCovered.empty()) return 0;

   double distScore = 0;
   Uint lastEnd = pt.lastPhrase->src_words.end;
   for (UintSet::const_iterator it = pt.sourceWordsNotCovered.begin();
        it != pt.sourceWordsNotCovered.end(); it++)
   {
      assert(it->start < it->end);
      if (it->end > sentLength) cerr << it->end << " " << sentLength << endl;
      assert(it->end <= sentLength);
      distScore += ( (lastEnd != it->start) ? -1.0 : 0.0 );
      lastEnd = it->end;
   } // for
   distScore += ( (sentLength != lastEnd) ? -1.0 : 0.0 );

   return distScore;
}

/************************** LexicalizedDistortion ******************************/
LexicalizedDistortion::LexicalizedDistortion(const string& arg)
{
   if(arg.empty()) {
      type = Combined;
      offset = 0;
   }

   vector<string> toks;
   split(arg,toks,"#");
   string stype = toks[0];
   if ( stype == "m" )
      type = M;
   else if (stype == "s" )
      type = S;
   else if (stype == "d" )
      type = D;
   else
      type = Combined;

   offset = 0;
   string str_num;
   if(toks.size()==1 && type==Combined) str_num = toks[0];
   if(toks.size()==2) str_num = toks[1];
   Uint uint_num;
   if (conv(str_num, uint_num))
      offset = 6 * uint_num;
}

void LexicalizedDistortion::readDefaults(const char* file_dflt)
{
   iMagicStream in(file_dflt, true);
   string line;
   getline(in, line);

   trim(line, " ");

   vector<string> valueString;

   if (line == "")
      error(ETFatal, "lexicalized distortion model default value file: %s is empty!", file_dflt);
   else
   {
      split(line, valueString, " ");
      if (valueString.size() != 6) {
         error(ETFatal, "wrong number of lexicalized distortion model scores in %s.",file_dflt);
      }

      float prob;
      for ( vector<string>::iterator it=valueString.begin(); it != valueString.end(); ++it){
         if ( ! conv((*it).c_str(), prob))
            error(ETFatal, "Invalid number format (%s) in %s.",(*it).c_str(), file_dflt);

         defaultValues.push_back(log(prob));
      }
   }
}

LexicalizedDistortion::ReorderingType LexicalizedDistortion::determineReorderingType(const PartialTranslation& pt)
{
   ReorderingType type;

   if (pt.lastPhrase->src_words.start == pt.back->lastPhrase->src_words.end)
      type = M;
   else if (pt.lastPhrase->src_words.end == pt.back->lastPhrase->src_words.start)
      type = S;
   else
      type = D;

   return type;
}

vector<float>::const_iterator
LexicalizedDistortion::getLexDisProb(const PhraseInfo& phrase_info) const
{
   const ForwardBackwardPhraseInfo* pi = dynamic_cast<const ForwardBackwardPhraseInfo *>(&phrase_info);

   if ( pi && pi->lexdis_probs.size() % 6 == 0 &&
        pi->lexdis_probs.size()>=offset+6){
      return pi->lexdis_probs.begin()+offset;
   }
   else {
      assert(!pi || pi->lexdis_probs.size() <= offset);
      return defaultValues.begin()+offset;
   }
}

/************************** FwdLexDistortion ******************************/

FwdLexDistortion::FwdLexDistortion(const string& arg)
   : LexicalizedDistortion(arg)
{}

double FwdLexDistortion::scoreHelper(const PartialTranslation& pt)
{
   double result(0);
   ReorderingType Rtype = determineReorderingType(pt);

   const PhraseInfo* ph = pt.back->lastPhrase;

   const vector<float>::const_iterator lexdis_probs = getLexDisProb(*ph);

   switch (Rtype){
      case M:
         result = (type == Combined || type == M) ? *(lexdis_probs+3) : 0;
         break;
      case S:
         result = (type == Combined || type == S) ? *(lexdis_probs+4) : 0;
         break;
      case D:
         result = (type == Combined || type == D) ? *(lexdis_probs+5) : 0;
         break;
      default:
         error(ETFatal, "wrong reordering type: %u",Rtype);
   }

   return result;
}
double FwdLexDistortion::score(const PartialTranslation& pt)
{
   if (pt.sourceWordsNotCovered.empty()) {
      PhraseInfo endPhrase;
      endPhrase.src_words = Range(sentLength, sentLength);
      PartialTranslation endPT(&pt, &endPhrase, &pt.sourceWordsNotCovered);
      return scoreHelper(pt) + scoreHelper(endPT);
   } else {
      return scoreHelper(pt);
   }
}

double FwdLexDistortion::partialScore(const PartialTranslation& pt)
{
   return scoreHelper(pt) -
      FwdLexDistortion::precomputeFutureScore(*pt.lastPhrase);
}

Uint FwdLexDistortion::computeRecombHash(const PartialTranslation& pt)
{
   return pt.lastPhrase->src_words.start;
}

bool FwdLexDistortion::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   return pt1.lastPhrase == pt2.lastPhrase;
}

double FwdLexDistortion::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   if ( type != Combined ) return 0;

   const vector<float>::const_iterator lexdis_probs = getLexDisProb(phrase_info);
   return *max_element(lexdis_probs+3,lexdis_probs+6);
}

double FwdLexDistortion::futureScore(const PartialTranslation& pt)
{
   if (pt.sourceWordsNotCovered.empty()) return 0;

   // EJJ: it is not a mistake that futureScore() exceptionally calls
   // precomputeFutureScore() here: we capture here the score the next
   // phrase will incur relative to the current one, which is not already
   // captured elsewhere in the future score calculation.
   return FwdLexDistortion::precomputeFutureScore(*pt.lastPhrase);
}

/************************** BackLexDistortion ******************************/
BackLexDistortion::BackLexDistortion(const string& arg)
   : LexicalizedDistortion(arg)
{}

double BackLexDistortion::scoreHelper(const PartialTranslation& pt)
{
   double result(0);
   ReorderingType Rtype = determineReorderingType(pt);

   const PhraseInfo* ph = pt.lastPhrase;

   const vector<float>::const_iterator lexdis_probs = getLexDisProb(*ph);

   switch (Rtype){
      case M:
         result = (type == Combined || type == M) ? *(lexdis_probs+0) : 0;
         break;
      case S:
         result = (type == Combined || type == S) ? *(lexdis_probs+1) : 0;
         break;
      case D:
         result = (type == Combined || type == D) ? *(lexdis_probs+2) : 0;
         break;
      default:
         error(ETFatal, "wrong reordering type: %s",Rtype);
   }

   return result;
}

double BackLexDistortion::score(const PartialTranslation& pt)
{
   if (pt.sourceWordsNotCovered.empty()) {
      PhraseInfo endPhrase;
      endPhrase.src_words = Range(sentLength, sentLength);
      PartialTranslation endPT(&pt, &endPhrase, &pt.sourceWordsNotCovered);
      return scoreHelper(pt) + scoreHelper(endPT);
   } else {
      return scoreHelper(pt);
   }
}

double BackLexDistortion::partialScore(const PartialTranslation& pt)
{
   return 0.0;
}

Uint BackLexDistortion::computeRecombHash(const PartialTranslation& pt)
{
   return pt.lastPhrase->src_words.start;
}

bool BackLexDistortion::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   return pt1.lastPhrase->src_words == pt2.lastPhrase->src_words;
}

double BackLexDistortion::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   if ( type != Combined ) return 0;

   const vector<float>::const_iterator lexdis_probs = getLexDisProb(phrase_info);
   return *max_element(lexdis_probs, lexdis_probs+3);
}

double BackLexDistortion::futureScore(const PartialTranslation& pt)
{
   return 0.0;
}

/***************** Utility **************************/

static bool containsRange(const UintSet& set, Range query)
{
   return binary_search(set.begin(),set.end(),query);
}


/********************* HierFwdLDM *********************************/

FwdHierLDM::FwdHierLDM(const string& arg)
   : FwdLexDistortion(arg)
{}

Uint FwdHierLDM::computeRecombHash(const PartialTranslation &pt)
{
   // Right-to-left hierarchical doesn't use the parser
   return pt.lastPhrase->src_words.start;
}

bool FwdHierLDM::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   // Right-to-left hierarchical doesn't use the parser
   return pt1.lastPhrase->src_words == pt2.lastPhrase->src_words;
}

LexicalizedDistortion::ReorderingType FwdHierLDM::determineReorderingType(const PartialTranslation& pt)
{
   // CAC: Okay, this is going to be tricky. Hold on to your butts.
   //
   // Your FwdLDM focuses on back->lastPhrase and uses the "Next" statistics from dmcount.
   // For the non-hierarchical LDMs, the difference between Fwd and Back is just a matter
   // of perspective, do we look up probs for back->lastPhrase or lastPhrase?
   //
   // In the Hierarchical case, we lose that symmetry. Instead, one model determines
   // orientation parsing from left-to-right, while the other determines it from right-to-left.
   // Unfortunately for all of us, the probs for back->lastPhrase correspond to parsing
   // from right to left (back->lastPhrase is the next phrase to pushed onto the hypothetical
   // right-to-left parser). It's really too bad that this was named the FwdLDM, but we're stuck
   // with it.
   //
   // So, this function approximates the orientation that would have been determined, had
   // we been shift-reduce parsing from right to left. That is, if back->lastPhrase had been
   // pushed on to a right-to-left shift-reduce parser, right after lastPhrase,
   // what (probably) would have happened?

   LexicalizedDistortion::ReorderingType type;

   if (pt.lastPhrase->src_words.start == pt.back->lastPhrase->src_words.end)
      type = M; // Easy M
   else if (pt.lastPhrase->src_words.end == pt.back->lastPhrase->src_words.start)
      type = S; // Easy s
   else if (pt.lastPhrase->src_words.start > pt.back->lastPhrase->src_words.end &&
            containsRange(pt.sourceWordsNotCovered, Range(pt.back->lastPhrase->src_words.end,
                                                          pt.lastPhrase->src_words.start)))
      type = M; // No past links get in the way of an M
   else if (pt.back->lastPhrase->src_words.start > pt.lastPhrase->src_words.end &&
            containsRange(pt.sourceWordsNotCovered, Range(pt.lastPhrase->src_words.end,
                                                          pt.back->lastPhrase->src_words.start)))
      type = S; // No past links get in the way of an S
   else
      type = D;

   return type;
}

/***************** BackHierLDM **********************/

BackHierLDM::BackHierLDM(const string& arg)
   : BackLexDistortion(arg)
{}

Uint BackHierLDM::computeRecombHash(const PartialTranslation &pt)
{
   assert(pt.shiftReduce!=NULL);
   return pt.shiftReduce->computeRecombHash();
}

bool BackHierLDM::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   assert(pt1.shiftReduce!=NULL);
   assert(pt2.shiftReduce!=NULL);
   return ShiftReducer::isRecombinable(pt1.shiftReduce, pt2.shiftReduce);
}

LexicalizedDistortion::ReorderingType BackHierLDM::determineReorderingType(const PartialTranslation& pt)
{
   // CAC: This case is much easier. We simply replace references to
   // back->lastPhrase with bounds derived from the shift-reduce parser

   LexicalizedDistortion::ReorderingType type;

   assert(pt.back->shiftReduce!=NULL);
   if (pt.lastPhrase->src_words.start == pt.back->shiftReduce->end())
      type = M;
   else if (pt.lastPhrase->src_words.end == pt.back->shiftReduce->start())
      type = S;
   else
      type = D;

   return type;
}

/*************** BackFakeHierLDM ********************/

BackFakeHierLDM::BackFakeHierLDM(const string& arg)
   : BackLexDistortion(arg)
{}

LexicalizedDistortion::ReorderingType
BackFakeHierLDM::determineReorderingType(const PartialTranslation& pt)
{
   // CAC: Fake the BackHierLDM's re-ordering type check, using adjacency
   // in the coverage vector as a stand-in for constituency in the parser
   // (as is done in FwdHierLDM)

   if (pt.lastPhrase->src_words.start == pt.back->lastPhrase->src_words.end)
      return M; // Easy M
   else if (pt.lastPhrase->src_words.end == pt.back->lastPhrase->src_words.start)
      return S; // Easy s
   else if (pt.lastPhrase->src_words.start > pt.back->lastPhrase->src_words.end)
   {
      UintSet intersect;
      intersectRange(intersect, pt.sourceWordsNotCovered,
                     Range(pt.back->lastPhrase->src_words.end, pt.lastPhrase->src_words.start));
      if(intersect.empty())
         return M; // Can walk from cur to prev using only covered spaces
   }
   else if (pt.back->lastPhrase->src_words.start > pt.lastPhrase->src_words.end)
   {
      UintSet intersect;
      intersectRange(intersect, pt.sourceWordsNotCovered,
                     Range(pt.lastPhrase->src_words.end, pt.back->lastPhrase->src_words.start));
      if(intersect.empty())
         return S; // Can walk from cur to prev using only covered spaces
   }

   return D;
}
