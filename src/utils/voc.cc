/**
 * @author George Foster
 * @file voc.cc  Vocabulary associates an integer to every word.
 *
 *
 * COMMENTS:
 *
 * Map strings <-> unique indexes
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "voc.h"
#include <errors.h>
#include <iostream>
#include <iterator>

using namespace Portage;

void Voc::deleteWords()
{
   for (Uint i = 0; i < words.size(); ++i)
      delete[] words[i];
}

Uint Voc::add(const char* word) {
   // Optimization: don't strdup unless we actually need to add word.  The
   // extra call to find means adding a new word is somewhat slower, though not
   // significantly.  But re-adding an existing word is much faster this way.
   // We often re-add the same words many times, so this implementation choice
   // makes the Voc class fastest overall.
   MapIter p = map.find(word);
   if ( p == map.end() ) {
      const char* w = strdup_new(word);
      pair<MapIter,bool> res = map.insert(make_pair(w, size()));
      assert(res.second);
      words.push_back(w);
      return res.first->second;
   } else {
      return p->second;
   }
}

void Voc::index(const vector<string>& src, vector<Uint>& dest) const
{
   // Operates on the basis of append mode
   dest.reserve(dest.size() + src.size());

   //transform(src.begin(), src.end(), back_insert_iterator< vector<Uint> >(dest), indexConverter(*this));
   //return;
   typedef vector<string>::const_iterator SRC_IT;
   for (SRC_IT it(src.begin()); it!=src.end(); ++it)
      dest.push_back(index(it->c_str()));
}

bool Voc::test() {

   Voc v;
   bool ok = true;

   const char* testwords[] = {
      "this", "is", "a", "somewhat", "redundant", "test", "of", "one", "small",
      "class", "not", "entirely", "superfluous", "itself", "however"
   };

   for (Uint iter = 0; ok && iter < 10; ++iter) {

      for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i) {
         if (v.add(testwords[i]) != v.size()-1) {
            error(ETWarn, "init add test failed");
            ok = false;
            break;
         }
      }
      if (v.size() != ARRAY_SIZE(testwords)) {
         error(ETWarn, "init size check failed");
         ok = false;
      }
      for (Uint i = 0; i < v.size(); ++i) {
         if (strcmp(v.word(i),testwords[i]) != 0) {
            error(ETWarn, "index contents check failed");
            ok = false;
         }
      }
      for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i) {
         if (v.index(testwords[i]) != i) {
            error(ETWarn, "find test failed");
            ok = false;
            break;
         }
      }
      for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i) {
         if (v.add(testwords[i]) != i) {
            error(ETWarn, "redundancy test failed");
            ok = false;
            break;
         }
      }
      v.write("-", ", "); cout << endl;
      v.clear();
   }

   return ok;
}

bool Portage::testCountingVoc()
{
   CountingVoc v;
   bool ok = true;

   const char* testwords[] = {
      "what", "is", "the", "price", "of", "experience", "?",
      "do", "men", "buy", "it", "for", "a", "song", ",", "or", "wisdom",
      "for", "a", "dance", "in", "the", "street", "?",
      "no", ",", "it", "is", "bought", "with", "the", "cost", "of", "all",
      "that", "man", "has", ",", "his", "house", ",", "his", "wife", ",",
      "his", "children"
   };

   for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i)
      v.add(testwords[i]);

   Uint total = 0;
   for (Uint i = 0; i < v.size(); ++i) {
      total += v.freq(i);
      // cout << v.word(i) << " " << v.freq(i) << endl;
   }

   ok = (total == ARRAY_SIZE(testwords));

   return ok;
}

