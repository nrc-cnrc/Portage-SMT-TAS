/**
 * @author Eric Joanis
 * @file test_errorRateStats.h
 *
 * Make sure the new ErrorRateStats::convert{To,From}Pnorm work correctly.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "ErrorRateStats.h"

using namespace Portage;

namespace Portage {

class TestErrorRateStats : public CxxTest::TestSuite 
{
public:
   void testRoundTripPnormConversion() {
      for (double WER = 0; WER < 3; WER += 0.1) {
         double score = ErrorRateStats::convertFromDisplay(WER);
         TS_ASSERT_EQUALS(score, -WER);
         double pnorm = ErrorRateStats::convertToPnorm(score);
         double round_trip_score = ErrorRateStats::convertFromPnorm(pnorm);
         TS_ASSERT_DELTA(score, round_trip_score, 0.00001);
         //cerr << "WER = " << WER << "  score = " << score << "  pnorm = " << pnorm << endl;
      }
   }
};

} // namespace Portage



