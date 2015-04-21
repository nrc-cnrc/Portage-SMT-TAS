/**
 * @author Samuel Larkin
 * @file test_translationReader.h
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "translationReader.h"
#include "tmp_val.h"

using namespace Portage;
using namespace Portage::Error_ns;

namespace Portage {

class testTranslationReader : public CxxTest::TestSuite 
{
private:
   // Temporarily mute the the function error used by arg_reader for all tests.
   tmp_val<Error_ns::ErrorCallback> tmp;

   const string fixNbest, fixPal, dynamicNbest, dynamicPal;

public:
   /**
    * Default constructor.
    * Important to note we are replacing error's callback since we want to mute
    * it and track some info.
    */
   testTranslationReader()
   : tmp(Current::errorCallback, countErrorCallBack)
   , fixNbest("tests/data/fixNbest")
   , fixPal("tests/data/fixPal")
   , dynamicNbest("tests/data/dynamicNbest")
   , dynamicPal("tests/data/dynamicPal")
   { }

   void setUp() {
      ErrorCounts::Total = 0;
      ErrorCounts::last_msg.clear();
   }


   ////////////////////////////////////////
   // FIX
   void testFix() {
      //TS_TRACE("testFix");
      NbestReader nbestReader(createT(fixNbest, fixPal, 5));
      Uint s(0);
      for (; nbestReader->pollable(); ++s) {
         Nbest nbest;
         nbestReader->poll(nbest);
         TS_ASSERT_EQUALS(5u, nbest.size());
         for (Uint t(0); t<nbest.size(); ++t) {
            TS_ASSERT_EQUALS(1u, nbest[t].phraseAlignment.size());
            const AlignedPair& al = nbest[t].phraseAlignment.front();
            TS_ASSERT_EQUALS(10u*s, al.source.first);
            TS_ASSERT_EQUALS(10u*s+t, al.source.last);
            TS_ASSERT_EQUALS(1u, al.target.first);
            TS_ASSERT_EQUALS(1u, al.target.last);
         }
      }
      TS_ASSERT_EQUALS(3u, s);
      TS_ASSERT(!nbestReader->pollable());
      nbestReader.reset();
      TS_ASSERT_EQUALS(0u, ErrorCounts::Total);
   }

   void testFixNotEnoughPhraseAlignment() {
      //TS_TRACE("testFixNotEnoughPhraseAlignment");
      NbestReader nbestReader(createT(fixNbest, fixPal+".short", 5));
      Uint s(0);
      for (; nbestReader->pollable(); ++s) {
         Nbest nbest;
         nbestReader->poll(nbest);
         TS_ASSERT_EQUALS(5u, nbest.size());
         for (Uint t(0); t<nbest.size(); ++t) {
            if (s==2 && t==4) {
               // NOTE: technically, nbest[t] should be empty if we had done
               // the error FATAL but for the purpose of this unittest we are
               // only grabbing the error string to validate it late thus the
               // code keeps on going and actually reads the translation.
               // That's why it is not empty.
               //TS_ASSERT(nbest[t].empty());
               TS_ASSERT(nbest[t].phraseAlignment.empty());
            }
            else {
               TS_ASSERT_EQUALS(1u, nbest[t].phraseAlignment.size());
               const AlignedPair& al = nbest[t].phraseAlignment.front();
               TS_ASSERT_EQUALS(10u*s, al.source.first);
               TS_ASSERT_EQUALS(10u*s+t, al.source.last);
               TS_ASSERT_EQUALS(1u, al.target.first);
               TS_ASSERT_EQUALS(1u, al.target.last);
            }
         }
      }
      TS_ASSERT_EQUALS(1u, ErrorCounts::Total);
      TS_ASSERT_EQUALS(szNotEnoughAlignments, ErrorCounts::last_msg);
      TS_ASSERT_EQUALS(3u, s);
      TS_ASSERT(!nbestReader->pollable());
      // NOTE read previous note to understand why we are not expecting any
      // error here.
      nbestReader.reset();
   }

   void testFixTooManyPhraseAlignment() {
      //TS_TRACE("testFixTooManyPhraseAlignment");
      NbestReader nbestReader(createT(fixNbest, fixPal+".long", 5));
      Uint s(0);
      for (; nbestReader->pollable(); ++s) {
         Nbest nbest;
         nbestReader->poll(nbest);
         TS_ASSERT_EQUALS(5u, nbest.size());
         for (Uint t(0); t<nbest.size(); ++t) {
            TS_ASSERT_EQUALS(1u, nbest[t].phraseAlignment.size());
            const AlignedPair& al = nbest[t].phraseAlignment.front();
            TS_ASSERT_EQUALS(10u*s, al.source.first);
            TS_ASSERT_EQUALS(10u*s+t, al.source.last);
            TS_ASSERT_EQUALS(1u, al.target.first);
            TS_ASSERT_EQUALS(1u, al.target.last);
         }
      }
      TS_ASSERT_EQUALS(3u, s);
      TS_ASSERT(!nbestReader->pollable());
      nbestReader.reset();
      TS_ASSERT_EQUALS(1u, ErrorCounts::Total);
      TS_ASSERT_EQUALS("Not all alignments were read.", ErrorCounts::last_msg);
   }


   ////////////////////////////////////////
   // DYNAMIC
   void testDynamic() {
      //TS_TRACE("testDynamic");
      static const Uint expected_Nbest_size[] = { 5, 2, 4 };
      NbestReader nbestReader(createT(dynamicNbest, dynamicPal, 0));
      Uint s(0);

      for (; nbestReader->pollable(); ++s) {
         Nbest nbest;
         nbestReader->poll(nbest);
         TS_ASSERT_EQUALS(expected_Nbest_size[s], nbest.size());
         for (Uint t(0); t<nbest.size(); ++t) {
            TS_ASSERT_EQUALS(1u, nbest[t].phraseAlignment.size());
            const AlignedPair& al = nbest[t].phraseAlignment.front();
            TS_ASSERT_EQUALS(10u*s, al.source.first);
            TS_ASSERT_EQUALS(10u*s+t, al.source.last);
            TS_ASSERT_EQUALS(1u, al.target.first);
            TS_ASSERT_EQUALS(1u, al.target.last);
         }
      }

      TS_ASSERT_EQUALS(3u, s);
      TS_ASSERT(!nbestReader->pollable());
      nbestReader.reset();
      TS_ASSERT_EQUALS(0u, ErrorCounts::Total);
   }

   void testDynamicMissingAlignment() {
      //TS_TRACE("testDynamicMissingAlignment");
      NbestReader nbestReader(createT(dynamicNbest, dynamicPal + ".missing", 0));
      Uint s(0);

      Nbest nbest;
      nbestReader->poll(nbest);
      TS_ASSERT_EQUALS(5u, nbest.size());
      // Last alignment of first nbest is missing.
      for (Uint t(0); t<nbest.size()-1; ++t) {
         TS_ASSERT_EQUALS(1u, nbest[t].phraseAlignment.size());
         const AlignedPair& al = nbest[t].phraseAlignment.front();
         TS_ASSERT_EQUALS(10u*s, al.source.first);
         TS_ASSERT_EQUALS(10u*s+t, al.source.last);
         TS_ASSERT_EQUALS(1u, al.target.first);
         TS_ASSERT_EQUALS(1u, al.target.last);
      }
      TS_ASSERT_EQUALS(1u, ErrorCounts::Total);
      TS_ASSERT_EQUALS(szMisaligned, ErrorCounts::last_msg);
      // Reseting should add two more errors and the last error message should be about not having read all translations.
      nbestReader.reset();
      TS_ASSERT_EQUALS(3u, ErrorCounts::Total);
      TS_ASSERT_EQUALS(szIncompleteTranslationRead, ErrorCounts::last_msg);
   }

   void testDynamicTooManyAlignment() {
      //TS_TRACE("testDynamic");
      static const Uint expected_Nbest_size[] = { 5, 2, 4 };
      NbestReader nbestReader(createT(dynamicNbest, dynamicPal+".long", 0));
      Uint s(0);

      for (; nbestReader->pollable(); ++s) {
         Nbest nbest;
         nbestReader->poll(nbest);
         TS_ASSERT_EQUALS(expected_Nbest_size[s], nbest.size());
         for (Uint t(0); t<nbest.size(); ++t) {
            TS_ASSERT_EQUALS(1u, nbest[t].phraseAlignment.size());
            const AlignedPair& al = nbest[t].phraseAlignment.front();
            TS_ASSERT_EQUALS(10u*s, al.source.first);
            TS_ASSERT_EQUALS(10u*s+t, al.source.last);
            TS_ASSERT_EQUALS(1u, al.target.first);
            TS_ASSERT_EQUALS(1u, al.target.last);
            //nbest[t].write(cerr); cerr << endl;  // SAM DEBUGGING
         }
      }

      TS_ASSERT_EQUALS(3u, s);
      TS_ASSERT(!nbestReader->pollable());
      TS_ASSERT_EQUALS(0u, ErrorCounts::Total);
      nbestReader.reset();
      TS_ASSERT_EQUALS(1u, ErrorCounts::Total);
      TS_ASSERT_EQUALS(szIncompleteAlignmentRead, ErrorCounts::last_msg);
   }

}; // testTranslationReader

} // Portage
