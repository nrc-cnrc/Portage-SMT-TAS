/**
 * @author Aaron Tikuisis
 * @file testphrasefinder.cc  This file contains code to test the
 * RangePhraseFinder class.  Currently, the testing is not fully automated;
 * rather, it produces output which must be checked by a user.
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

#include "phrasefinder.h"
#include "phrasedecoder_model.h"
#include "canoe_general.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace Portage;

/**
 * Prints the phrases in a given vector by printing the first word in each.
 * @param[in] v  The vector containing pointers to phrases found.
 */
void print(const vector<PhraseInfo *> &v)
{
   cout << "Phrases found: " << flush;
   vector<PhraseInfo *>::const_iterator it = v.begin();
   while (it != v.end())
   {
      PhraseInfo *cur = *it;
      cout << cur->phrase.front();
      it++;
      if (it != v.end())
      {
         cout << ", ";
      }
      cout << flush;
   }
   cout << endl;
} // print




/**
 * Program testphrasefinder's entry point.
 * Sets up and runs the test.
 * @return Returns 0 if successful.
 */
int main(int argc, char* argv[])
{
   if (argc > 1) {
      cerr << "This is used to test the RangePhraseFinder class." << endl;
      exit(1);
   }

   const Uint NUMPHRASES = 10;
   const Uint SENTLENGTH = 5;
   const Uint NUMFINDSETS = 3;
   const Uint DISTLIMIT[] = {NO_MAX_DISTORTION, 1, 2};
   const Uint PHRASEEND = 2;
/*
Phrase range table:
    0  1  2  3  4  5
  0 [     )
  1    [  )
  2    [     )
  3    [        )
  4       [  )
  5       [        )
  6       [        )
  7          [  )
  8          [     )
  9             [  )

Range-sets to find phrases for:
[0, 5) - should get all phrases with no dist. limit; nothing with distlimit=1;
         and 0, 1 with distlimit=2.
[1, 4) - should get 1, 2, 3, 4, 7 without dist. limit; 1, 2, 3 with distlimit=1
[1, 2), [3, 5) - should get 1, 7, 8, 9 with no dist limit.; 1 with distlimit=1
         or 2.
*/

   // Create the phrases in an array
   PhraseInfo phrases[NUMPHRASES];
   phrases[0].src_words.start = 0;
   phrases[0].src_words.end = 2;

   phrases[1].src_words.start = 1;
   phrases[1].src_words.end = 2;

   phrases[2].src_words.start = 1;
   phrases[2].src_words.end = 3;

   phrases[3].src_words.start = 1;
   phrases[3].src_words.end = 4;

   phrases[4].src_words.start = 2;
   phrases[4].src_words.end = 3;

   phrases[5].src_words.start = 2;
   phrases[5].src_words.end = 5;

   phrases[6].src_words.start = 2;
   phrases[6].src_words.end = 5;

   phrases[7].src_words.start = 3;
   phrases[7].src_words.end = 4;

   phrases[8].src_words.start = 3;
   phrases[8].src_words.end = 5;

   phrases[9].src_words.start = 4;
   phrases[9].src_words.end = 5;

   // Set each phrase's first word to its index and put pointers to the
   // phrases into a vector
   vector<PhraseInfo *> **phTArray =
      TriangArray::Create<vector<PhraseInfo *> >()(SENTLENGTH);
   for (Uint i = 0; i < NUMPHRASES; i++)
   {
      phrases[i].phrase.push_back(i);
      phTArray[phrases[i].src_words.start][phrases[i].src_words.end -
         phrases[i].src_words.start - 1].push_back(phrases + i);
   }

   // Create the Uint sets to be used to find phrases for
   UintSet sets[NUMFINDSETS];

   {
      Range cur;

      cur.start = 0;
      cur.end = 5;
      sets[0].push_back(cur);

      cur.start = 1;
      cur.end = 4;
      sets[1].push_back(cur);

      cur.start = 1;
      cur.end = 2;
      sets[2].push_back(cur);
      cur.start = 3;
      cur.end = 5;
      sets[2].push_back(cur);
   }
   PhraseInfo ph;
   ph.src_words.end = PHRASEEND;

   for (Uint i = 0; i < ARRAY_SIZE(DISTLIMIT); ++i)
   {
      // Create the range phrase finder and use it to find phrases
      RangePhraseFinder finder(phTArray, SENTLENGTH, DISTLIMIT[i],
                               false, false);

      for (Uint i = 0; i < NUMFINDSETS; i++)
      {
         PartialTranslation cur;
         cur.sourceWordsNotCovered = sets[i];
         cur.lastPhrase = &ph;

         // Print the phrases found
         vector<PhraseInfo *> v;
         finder.findPhrases(v, cur);
         print(v);
      }
   }
   TriangArray::Delete<vector<PhraseInfo *> >()(phTArray, SENTLENGTH);
} // main
