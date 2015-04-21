/**
 * @author Samuel Larkin
 * @file test_perSentenceStats.h
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
#include "perSentenceStats.h"
#include "bleu.h"

using namespace Portage;

namespace Portage {

typedef perSentenceStats<BLEUstats> PsBLEU;

class TestPerSentenceStats : public CxxTest::TestSuite 
{
public:
   void test1() {
      Translation translation("a a a a");
      References refs;
      refs.push_back(Reference("a a a a"));

      PsBLEU B(translation, refs);
      BLEUstats b(translation, refs, 1);

      /*
      B.output(cerr);
      b.output(cerr);
      */
      TS_ASSERT_EQUALS(1.0f, B.total);
      TS_ASSERT_EQUALS(1.0f, B.count);
      TS_ASSERT_EQUALS(1.0f, B.score());
      TS_ASSERT_EQUALS(1.0f, BLEUstats::convertToDisplay(b.score()));
      TS_ASSERT_EQUALS(B.score(), BLEUstats::convertToDisplay(b.score()));
   }

   void test2() {
      PsBLEU B1, B2;
      BLEUstats b1, b2;

      {
         Translation translation("a a a a");
         References refs;
         refs.push_back(Reference("a a a a"));

         B1.init(translation, refs);
         b1.init(translation, refs, 1);

         TS_ASSERT_EQUALS(1.0f, B1.total);
         TS_ASSERT_EQUALS(1u, B1.count);
      }
      
      {
         Translation translation("a a a a");
         References refs;
         refs.push_back(Reference("a a a b"));

         B2.init(translation, refs);
         b2.init(translation, refs, 1);

         TS_ASSERT_DELTA(0.0f, B2.total, 0.0001f);
         TS_ASSERT_EQUALS(1u, B2.count);
      }

      TS_ASSERT_DELTA(1.0f, (B1+B2).total, 0.0001f);
      TS_ASSERT_EQUALS(2u, (B1+B2).count);

      /*
      B1.output(cerr);
      B2.output(cerr);
      (B1+B2).output(cerr);
      cerr << endl;
      b1.output(cerr);
      b2.output(cerr);
      (b1+b2).output(cerr);
      */

      TS_ASSERT((B1+B2).score() != (b1+b2).score());
      const float average = (BLEUstats::convertToDisplay(b1.score()) + BLEUstats::convertToDisplay(b2.score())) / 2.0f;
      TS_ASSERT_DELTA((B1+B2).score(), average, 0.0001f);
   }

}; // TestPerSentenceStats

} // Portage
