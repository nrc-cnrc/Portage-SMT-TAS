/**
 * @author Matthew Arnold
 * @file alignedphrase.h  Utility that houses structs and classes for phrase alignments
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * This will attempt to uncombine decoder states to produce listings
 * of alignments
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <decoder.h>
#include <phrasedecoder_model.h>
#include <canoe_general.h>
#include <utility>
#include <boost/noncopyable.hpp>


namespace Portage {

/// Keeps track of alignments
struct AlignedPhrase {
   /// the score of this translation
   double myScore;

   /// the individual scores of the phrase pairs in this translation
   vector<double> myPhraseScores;

   /// sequence of alignment pairs (Range refers to source, Phrase
   /// refers to target
   vector<pair<Range*, Phrase*> > myLine;

   /**
    * Constructor: Initialise with nothing (not used).
    */
   AlignedPhrase();
   /**
    * non-default Constructor.
    * @param pi
    * @param score
    */
   AlignedPhrase(PhraseInfo *pi, const double score);

   /**
    * line function: returns the string that this function refers to.
    * @param str: the return string
    * @param model: the decoding model to get the target words
    * @param submodelScores submodel scores
    * @param incScore: whether to print out the score with this line or not
    * @param onlyScore: print only score and no sentence
    */
   void line(string &str,
             PhraseDecoderModel *model,
             const vector<double> &submodelScores,
             bool incScore = true,
             bool onlyScore = false);

   /**
    * Adds in a new translation to this alignment.
    * @param pi: phrase associated with the next translation in the model
    * @param score: score associated with pi (scores are additive)
    */
   void add(PhraseInfo *pi, const double score);

};//end struct AlignedPhrase

/// Ordered linked list of alignments.
struct alignList : private boost::noncopyable {
  /// current alignment sentence
  AlignedPhrase val;
  /// scores of this translation according to the different submodels/features
  vector<double> submodelScores;
  /// next in list
  alignList *next;

  /**
   * Constructor
   * @param a: current alignment
   * @param scores
   */
  alignList(const AlignedPhrase &a, const vector<double> &scores)
    : val(a)
    , submodelScores(scores)
    , next(0)
  {}

  /**
   * Destructor
   */
  virtual ~alignList() {
    if (next) delete next;
  }

  /**
   * Add partial score
   */
  void add(const vector<double> &scores) {
    assert(scores.size()==submodelScores.size());
    for (uint i=0; i<scores.size(); i++)
      submodelScores[i] += scores[i];
  }

};

/**
 * betterAlignment: returns true if list1 contains the better alignment than
 * list 2, false if list 2 is better or if both are null
 * @param list1: first alignment option
 * @param list2: second alignment option
 * @return Returns the higher scoring alignment true for param1, false for
 * param 2
 */
bool betterAlignment(alignList *list1, alignList *list2);

/**
 * mergeAligned: Merge 2 alignment list choices into one list, keeping a
 * maximum of n entries, and return it.
 * @param list1,list2: the lists to compare
 * @param n: the max number of items to store in returned list
 * @return Returns a list containing the n best translations.
 * @pre list1 and list2 must be sorted from best to worse
 */
alignList* mergeAligned(alignList *list1, alignList *list2, Uint n);

/**
 * mergeAligned: Merge 2 alignment list choices into one list, with a maximum
 * of n entries, and return it.  This version also returns through parameters
 * the number of items in the list and the lowest score observed (for
 * optimisation purposes)
 * @param list1,list2: the lists to compare
 * @param n: the max number of items to store in returned list
 * @param numCounts: the number of items in the list
 * @param minScore: the lowest score observed in the list
 * @return Returns a list containing the n best translations as well as
 * returning the number of translations and the lowest score, for optimization
 * reasons.
 * @pre list1 and list2 must be sorted from best to worse
 */
alignList* mergeAligned(alignList *list1, alignList *list2, Uint n,
                        Uint &numCounts, double &minScore);

/**
 * printAlignedList: print a list of maximum n alignments of list1 to out
 * @param out: output stream to print to (cout, stringstream, etc...)
 * @param list: list to print out
 * @param model: model in order to print names of target words
 * @param n: max number of alignments to print (padded with blank lines)
 * @param incScore: whether to include the score when printing or not (true)
 * @param onlyScore: print only score and no sentence (false)
 */
void printAlignedList(ostream &out, alignList *list, PhraseDecoderModel *model,
                      Uint n, bool incScore = true, bool onlyScore = false);

/**
 * makeAlignments: Given a DecoderState, produces the (max)n best alignment
 * options with that decoder state and returns that list
 * @param state: state to create alignments for (non-null)
 * @param model: model used to create the state
 * @param tlen: length of the given target sentence
 * @param n: number of alignments to keep (greater than 0)
 * @return Returns to the vector of alignments all of them in no order
 */
alignList* makeAlignments(DecoderState *state, PhraseDecoderModel *model,
                          Uint tlen, Uint n);

} // ends namespace Portage
