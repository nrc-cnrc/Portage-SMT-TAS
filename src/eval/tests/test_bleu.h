/**
 * @author Samuel Larkin
 * @file test_belu.h  Test suite for BLEU.
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
#include "bleu.h"

using namespace Portage;

namespace Portage {

class TestBLEU : public CxxTest::TestSuite 
{
public:
   void testBLEUSmooth4_1() {
      Sentence translation("a z y x");
      Reference reference("a b c d");
      References references;
      references.push_back(reference);
      BLEUstats stat(translation, references, 4);

      TS_ASSERT_EQUALS(stat.match[0], 1);
      TS_ASSERT_EQUALS(stat.match[1], 0);
      TS_ASSERT_EQUALS(stat.match[2], 0);
      TS_ASSERT_EQUALS(stat.match[3], 0);
      TS_ASSERT_DELTA(stat.score(), -1.83423, 0.00001f);
      //stat.output(cerr);
   }

   void testBLEUSmooth4_2() {
      Sentence translation("a b z y");
      Reference reference("a b c d");
      References references;
      references.push_back(reference);
      BLEUstats stat(translation, references, 4);

      TS_ASSERT_EQUALS(stat.match[0], 2);
      TS_ASSERT_EQUALS(stat.match[1], 1);
      TS_ASSERT_EQUALS(stat.match[2], 0);
      TS_ASSERT_EQUALS(stat.match[3], 0);
      TS_ASSERT_DELTA(stat.score(), -1.141087, 0.00001f);
      //stat.output(cerr);
   }

   void testBLEUSmooth4_3() {
      Sentence translation("a b c z");
      Reference reference("a b c d");
      References references;
      references.push_back(reference);
      BLEUstats stat(translation, references, 4);

      TS_ASSERT_EQUALS(stat.match[0], 3);
      TS_ASSERT_EQUALS(stat.match[1], 2);
      TS_ASSERT_EQUALS(stat.match[2], 1);
      TS_ASSERT_EQUALS(stat.match[3], 0);
      TS_ASSERT_DELTA(stat.score(), -0.51986, 0.00001f);
      //stat.output(cerr);
   }

   void testBLEUSmooth4_4() {
      // In this test, the smoothing 1 / ( 2^k ) shouldn't kick in thus the
      // result should be the same BLEU score as if we used the normal BLEU for
      // scoring.
      Sentence translation("a b c d");
      Reference reference("a b c d");
      References references;
      references.push_back(reference);
      BLEUstats stat(translation, references, 4);
      BLEUstats stat_ref(translation, references, 0);

      TS_ASSERT_EQUALS(stat.match[0], 4);
      TS_ASSERT_EQUALS(stat.match[1], 3);
      TS_ASSERT_EQUALS(stat.match[2], 2);
      TS_ASSERT_EQUALS(stat.match[3], 1);
      TS_ASSERT_DELTA(stat.score(), -0.0, 0.00001f);
      TS_ASSERT_DELTA(stat.score(), stat_ref.score(), 0.00001f);
      //stat.output(cerr);
   }

}; // TestBLEU

} // Portage
