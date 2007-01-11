/**
 * @author Matthew Arnold
 * @file alignedphrase.cc  Utility that houses structs and classes for phrase alignments
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / 
 * Copyright 2005, National Research Council of Canada
 */

#include "alignedphrase.h"
#include <decoder.h>
#include <phrasedecoder_model.h>
#include <canoe_general.h>
#include <utility>
#include <iostream>
#include <sstream>

namespace Portage {

namespace {
  /// boolean whether to output debug statements or not
  const bool DEBUG = false;
}

//Default constructor
AlignedPhrase::AlignedPhrase()
  :myScore(0),
   myPhraseScores(vector<double>()),
   myLine(0)
{ myPhraseScores.clear(); }

//non-default contructor (build phrase alignment from phraseInfo)
AlignedPhrase::AlignedPhrase(PhraseInfo *pi, const double score)
  :myScore(score),
   myPhraseScores(vector<double>(1,score)),
   myLine(1)
{
  myLine[0] = make_pair(&(pi->src_words), &(pi->phrase));
}

//returns the line alignment
void AlignedPhrase::line(string &str, PhraseDecoderModel *model, const vector<double> &submodelScores, 
                         bool incScore, bool onlyScore) {
  stringstream ss;//to optimize, pass ss by ref instead of copying string
  			
  if (! onlyScore)
    for (Uint i = 0; i < myLine.size(); ++i) 
      if (myLine[i].first->start != myLine[i].first->end) {
        //only print out non-empty ranges
        ss << RANGEOUT(*(myLine[i].first)) << " ";
        string str;
        model->getStringPhrase(str, *(myLine[i].second));
        ss << "(" << str << ") ";
      }// if, for, if
  if (incScore) {
    ss << "\tphrase scores: ";
    for (vector<double>::const_iterator itr=myPhraseScores.begin()+1; itr!=myPhraseScores.end(); itr++)
      ss << " " << *itr;
    ss << "\ttotal score: " << myScore << "\t submodel (feature) scores: ";
    for (vector<double>::const_iterator itr=submodelScores.begin(); itr!=submodelScores.end(); itr++)
      ss << " " << *itr;
  }
  str = ss.str();
}//end line

//Augment this alignment (add in the next alignment)
void AlignedPhrase::add(PhraseInfo *pi, const double score) {
  if (DEBUG)
     cerr << "Adding translation" << endl;
  myLine.push_back( make_pair(&(pi->src_words), &(pi->phrase)) );
  myScore += score;
  myPhraseScores.push_back(score);
}

//Print out the top n alignments (padded with blank lines
void printAlignedList(ostream &out, alignList *list, PhraseDecoderModel *model,
                      Uint n, bool incScore, bool onlyScore) {

  Uint count = 0;
  string s;
  while (count < n && list) {    
    list->val.line(s, model, list->submodelScores, incScore, onlyScore);
    out << s << endl;
    count++;
    list = list->next;
  }
  for (;count < n; count++)
    out << endl;
}


//returns the higher scoring alignment
//true for param1, false for param 2
bool betterAlignment(alignList *list1, alignList *list2) {
  return (list1 && (!list2 || list1->val.myScore >= list2->val.myScore));    
}

//merges list1 and list2 and returns a list containing the n best translations
alignList* mergeAligned(alignList *list1, alignList *list2, Uint n) {
  Uint nf;
  double ms;
  return mergeAligned(list1, list2, n, nf, ms);
}

//merges list1 and list2 and returns a list containing the n best translations
//as well as returning the number of translations and the lowest score, 
//for optimization reasons
alignList* mergeAligned(alignList *list1, alignList *list2, Uint n, Uint &numFinal, double &minScore) {
//returns NULL if n <=0, or no L1 and no L2

  alignList *final;
  if (betterAlignment(list1, list2)) {
    if (DEBUG) {
      cerr << "List 1: total score: " << list1->val.myScore << " phrase scores: " << endl;
      for (vector<double>::const_iterator itr=list1->val.myPhraseScores.begin()+1; itr!=list1->val.myPhraseScores.end(); itr++)
        cerr << " " << *itr;
      cerr << " submodel (feature) scores: ";
      for (vector<double>::const_iterator itr=list1->submodelScores.begin(); itr!=list1->submodelScores.end(); itr++)
        cerr << " " << *itr;
      cerr << endl << " List 2: total score: " << list2->val.myScore << " phrase scores: ";
      for (vector<double>::const_iterator itr=list2->val.myPhraseScores.begin()+1; itr!=list2->val.myPhraseScores.end(); itr++)
        cerr << " " << *itr;
      cerr << " submodel (feature) scores: ";
      for (vector<double>::const_iterator itr=list2->submodelScores.begin(); itr!=list2->submodelScores.end(); itr++)
        cerr << " " << *itr;
      cerr << endl << " Selected list1" << endl;
    }
    //list 1 has the better element
    final = list1;
    list1 = list1->next;
  } else {
    //for some reason both list1 and list2 are empty, return
    if (!list2)
      return 0;
    if (DEBUG)
      cerr << "List 1: " << list1->val.myScore << " List 2: " << list2->val.myScore << endl
        << " Selected list2" << endl;
    //list 2 has the better first alignment
    final = list2;
    list2 = list2->next;
  }
  if (DEBUG)
    cerr << " First added. Continuing " << (list1?1:0) << " " << (list2?1:0) << endl;
  //now we add to the back of the list all the other translations (<=n of them in total)
  alignList *back = final;
  for (Uint num = 1; num < n; num++) {
    if (DEBUG)
      cerr << num << " added. Continuing " << (list1?1:0) << " " << (list2?1:0) << endl;
        
    if (betterAlignment(list1, list2)) {
      if (DEBUG)
        cerr << "List 1: " << list1->val.myScore << " List 2: " << (list2?list2->val.myScore:1) << endl
             << " Selected list1" << endl;
      //list 1 better
      back->next = list1;
      list1 = list1->next;
      back = back->next;
    } else {
    
      if (DEBUG)
        cerr << " Selected list2" << endl;
      //list 2 better, or ran out of items
      if (!list2) {
        if (DEBUG)
          cerr << "Empty Lists" << endl;
        //out of items in total (incomplete list)
        back->next = 0;
        numFinal = num+1;
        minScore = back->val.myScore;
        return final;
        //ran out of both items so just quit here.
      }
      
      if (DEBUG)
        cerr << "List 1: " << (list1?list1->val.myScore:1) << " List 2: " << list2->val.myScore << endl
             << " Selected list2" << endl;
      //list 2 is better
      back->next = list2;
      list2 = list2->next;
      back = back->next;
    }//end if else
  }//end for
  //finished reading items.  We now have n items in the list, and either list may ne non-empty

  //clean up unused items
  if (list1)
    delete list1;
  if (list2)
    delete list2;

  //update stats for possible short-circuit later on
  numFinal = n;
  minScore = back->val.myScore;
  back->next = 0;
  return final;
} 

/*Given a state, calculates all possible alignments of it and returns to the vector of
alignments all of them in no order*/
alignList* makeAlignments(DecoderState *state, PhraseDecoderModel *model, Uint n, Uint tlen) {
  assert(state != NULL);
  assert(n > 0);
  
  alignList *final;

  if (!state->back) {
    if (DEBUG)
      cerr << "Initial translation" << endl;
      
    //no previous state -> first translation (empty translation)
    vector<double> submodelScores;
    model->getFeatureFunctionVals(submodelScores, *(state->trans));
    final = new alignList(AlignedPhrase(state->trans->lastPhrase, state->score), submodelScores);
    
  } else {
    double scoreDiff = state->score - state->back->score;
    if (DEBUG)
      cerr << "Score Diff: " << scoreDiff << " old: " << state->back->score << " new: " << state->score << endl;
    vector<double> submodelScoreDiff;
    model->getFeatureFunctionVals(submodelScoreDiff, *(state->trans));

    //there is a previous translation, so make all possible alignments (<= n)

    final = makeAlignments(state->back, model, n, tlen);

    //now we have the list of all previous translations, must add current one to those
    //previous ones in order to bring them all to the current stage
    double minscore = INFINITY;
    Uint count = 0;
    for (alignList *temp = final; temp; temp = temp->next) {
      //iterate through list and add current in.  minscore and count act 
      //as possible shortcuts for later
      temp->val.add(state->trans->lastPhrase, scoreDiff);
      temp->add(submodelScoreDiff);
      minscore = (minscore <= temp->val.myScore? minscore : temp->val.myScore);
      count++;
    }

    //short circuit: if we only want one translation, then we just take the direct 
    //path back to the start only
    if (n == 1) {
      vector<double> submodelScores;
      return final;
    } 
      
    if (DEBUG)
      cerr << "Adding Recombined to list" << endl;
    //iterate through recombined lists 
    for (Uint i = 0; i < state->recomb.size(); i++) {
      if (DEBUG)
        cerr << "\trecomb " << i;
      if (count != n || minscore < state->recomb[i]->score) {
      //if we already maxed on numbers, and this new one won't yield anything better
      //we don't need to calculate all branches of it.
        if (DEBUG)
          cerr << "calculating";

        alignList *prev = makeAlignments(state->recomb[i], model, n, tlen);
        final = mergeAligned(final, prev, n, count, minscore);
      }
      if (DEBUG)
        cerr << endl;
    }//end for  */
    if (DEBUG)
      cerr << "Done recombined" << endl;
  }//end if
    
  return final;
}//end makeAlignments

} // ends namespace Portage
