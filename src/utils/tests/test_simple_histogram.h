/**
 * @author Samuel Larkin
 * @file test_simple_histogram.h Tests the simple histogram's functionalities
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __TEST_SIMPLE_HISTOGRAM_H__
#define __TEST_SIMPLE_HISTOGRAM_H__

#include <cxxtest/TestSuite.h>
#include "simple_histogram.h"

using namespace std;
using namespace Portage;

namespace Portage {

/// Tests the implementation of SimpleHistogram
class TestSimpleHistogram : public CxxTest::TestSuite 
{
   protected:
      /// Simplified definition of the basic histogram for testing.
      typedef SimpleHistogram<unsigned int> HIST;
      /// Histogram for testing basic functionalities.
      HIST* hist_fix_bin_1;

   public:
      /// Default Constructor.
      TestSimpleHistogram()
      : hist_fix_bin_1(NULL)
      {}


      /// Setup a clean experiment before each test.
      void setUp() {
         if (hist_fix_bin_1 == NULL) {
            hist_fix_bin_1 = new HIST(new fixBinner(1));
            hist_fix_bin_1->add(0);
            hist_fix_bin_1->add(1);
            hist_fix_bin_1->add(2);
            hist_fix_bin_1->add(3);
            hist_fix_bin_1->add(4);
            hist_fix_bin_1->add(5);
         }
      }


      /// Clean up after each test.
      void tearDown() {
         if (hist_fix_bin_1) {
            hist_fix_bin_1->clear();
            delete hist_fix_bin_1;
            hist_fix_bin_1 = NULL;
         }
      }


      /// Checks that the histogram return the given binner.
      void testGetBinner() {
         fixBinner* binner = new fixBinner(3);
         HIST hist(binner);
         TS_ASSERT(hist.getBinner() == binner);
         // the binner gets deleted by the histogram.
      }


      /// Checks the sum of the data points.
      void testSum() {
         TS_ASSERT_EQUALS(hist_fix_bin_1->getSum(), 15u);
      }


      /// Checks the sum of the square value of the data points.
      void testSum2() {
         TS_ASSERT_EQUALS(hist_fix_bin_1->getSum2(), 55u);
      }


      /// Check the overall number of data points.
      void testNum() {
         TS_ASSERT_EQUALS(hist_fix_bin_1->getNum(), 6u);
      }


      /// Tests the logBinner in base 3 with offset of 5 bins.
      void testLogBinner_3_5() {
         logBinner binner(3, 5);

         // Expected bins:
         // 0: [0, 0)
         // 1: [1, 1)
         // 2: [2, 2)
         // 3: [3, 3)
         // 4: [4, 4)
         // 5: [5, 5)  => (5-1) + 3^0
         // 6: [7, 12)  => (5-1) + 3^1
         // 7: [13, 30)  => (5-1) + 3^2
         // 8: [31, 84)  => (5-1) + 3^3
         // 9: [85, 246)  => (5-1) + 3^4
         // 10: [247, 733)  => (5-1) + 3^5

         TS_ASSERT_EQUALS(binner.whichBin(0), 0u);
         TS_ASSERT_EQUALS(binner.whichBin(1), 1u);
         TS_ASSERT_EQUALS(binner.whichBin(2), 2u);
         TS_ASSERT_EQUALS(binner.whichBin(3), 3u);
         TS_ASSERT_EQUALS(binner.whichBin(4), 4u);
         TS_ASSERT_EQUALS(binner.whichBin(5), 5u);
         TS_ASSERT_EQUALS(binner.whichBin(6), 5u);
         TS_ASSERT_EQUALS(binner.whichBin(7), 6u);
         TS_ASSERT_EQUALS(binner.whichBin(12), 6u);
         TS_ASSERT_EQUALS(binner.whichBin(13), 7u);
         TS_ASSERT_EQUALS(binner.whichBin(30), 7u);
         TS_ASSERT_EQUALS(binner.whichBin(200), 9u);

         TS_ASSERT_EQUALS(binner.toIndex(0), 0u);
         TS_ASSERT_EQUALS(binner.toIndex(1), 1u);
         TS_ASSERT_EQUALS(binner.toIndex(2), 2u);
         TS_ASSERT_EQUALS(binner.toIndex(3), 3u);
         TS_ASSERT_EQUALS(binner.toIndex(4), 4u);
         TS_ASSERT_EQUALS(binner.toIndex(5), 5u);
         TS_ASSERT_EQUALS(binner.toIndex(6), 7u);
         TS_ASSERT_EQUALS(binner.toIndex(7), 13u);
         TS_ASSERT_EQUALS(binner.toIndex(8), 31u);
         TS_ASSERT_EQUALS(binner.toIndex(9), 85u);
         TS_ASSERT_EQUALS(binner.toIndex(10), 247u);

         // Expected value is the lower bound of the bin
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(0)), 0u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(1)), 1u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(2)), 2u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(3)), 3u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(4)), 4u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(5)), 5u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(6)), 5u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(7)), 7u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(12)), 7u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(13)), 13u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(30)), 13u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(200)), 85u);
      }


      /// Tests the fix binner with bins of size 9.
      void testFixBinner_9() {
         fixBinner binner(9);

         // Expected bins:
         // 0: [0, 8)
         // 1: [9, 17)
         // 2: [18, 26)

         TS_ASSERT_EQUALS(binner.whichBin(0), 0u);
         TS_ASSERT_EQUALS(binner.whichBin(8), 0u);
         TS_ASSERT_EQUALS(binner.whichBin(9), 1u);
         TS_ASSERT_EQUALS(binner.whichBin(17), 1u);
         TS_ASSERT_EQUALS(binner.whichBin(18), 2u);
         TS_ASSERT_EQUALS(binner.whichBin(26), 2u);

         TS_ASSERT_EQUALS(binner.toIndex(0), 0u);
         TS_ASSERT_EQUALS(binner.toIndex(1), 9u);
         TS_ASSERT_EQUALS(binner.toIndex(2), 18u);
         TS_ASSERT_EQUALS(binner.toIndex(3), 27u);
         TS_ASSERT_EQUALS(binner.toIndex(4), 36u);
         TS_ASSERT_EQUALS(binner.toIndex(5), 45u);

         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(0)), 0u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(8)), 0u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(9)), 9u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(17)), 9u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(18)), 18u);
         TS_ASSERT_EQUALS(binner.toIndex(binner.whichBin(26)), 18u);
      }


      /// Tests a log histogram with base 3 and offset of 0 bins.
      void testLogHistogram_3_0() {
         HIST hist(new logBinner(3, 0));

         hist.add(0);   // 0 = (0 - 1) + 3^0
         hist.add(1);   // 0
         hist.add(2);   // 1 = (0 - 1) + 3^1
         hist.add(7);   // 1
         hist.add(8);   // 2 = (0 - 1) + 3^2
         hist.add(25);   // 2
         hist.add(26);   // 3 = (0 - 1) + 3^3
         hist.add(79);   // 3
         hist.add(80);   // 4 = (0 - 1) + 3^4
         hist.add(241);   // 4

         //hist.display(std::cerr);

         // Check the overall number of data points
         TS_ASSERT_EQUALS(hist.getNum(), 10u);

         TS_ASSERT_EQUALS(hist.getBinner()->whichBin(26), 3u);

         TS_ASSERT_EQUALS(hist.getNumBin(), 5u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(0), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(1), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(2), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(3), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(4), 2u);
      }


      /// Tests a log histogram with base 3 and offset of 5 bins.
      void testLogHistogram_3_5() {
         HIST hist(new logBinner(3, 5));

         hist.add(0);   // 0
         hist.add(1);   // 1
         hist.add(2);   // 2
         hist.add(3);   // 3
         hist.add(4);   // 4
         hist.add(5);   // 5 = (5 - 1) + 3^0
         hist.add(6);   // 5
         hist.add(7);   // 6 = (5 - 1) + 3^1
         hist.add(12);  // 6
         hist.add(13);  // 7 = (5 - 1) + 3^2
         hist.add(30);  // 7
         hist.add(200); // 9

         //hist.display(std::cerr);

         // Check the overall number of data points
         TS_ASSERT_EQUALS(hist.getNum(), 12u);

         TS_ASSERT_EQUALS(hist.getNumBin(), 9u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(0), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(1), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(2), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(3), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(4), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(5), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(6), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(7), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(9), 1u);
      }


      /// Tests a log histogram with base 2 and offset of 10 bins.
      void testLogHistogram_2_10() {
         HIST hist(new logBinner(2, 10));

         hist.add(0);   // 0
         hist.add(2);   // 2
         hist.add(9);   // 9
         hist.add(10);   // 10 = (10-1) + 2^0
         hist.add(11);   // 11 = (10-1) + 2^1
         hist.add(12);   // 11
         hist.add(13);   // 12 = (10-1) + 2^2
         hist.add(16);   // 12
         hist.add(17);   // 13 = (10-1) + 2^3
         hist.add(24);   // 13
         hist.add(25);   // 14 = (10-1) + 2^4
         hist.add(40);   // 14
         hist.add(41);   // 15 = (10-1) + 2^5
         hist.add(72);   // 15
         hist.add(73);   // 16 = (10-1) + 2^6
         hist.add(136);  // 16

         TS_ASSERT_EQUALS(hist.getNum(), 16u);

         TS_ASSERT_EQUALS(hist.getNumBin(), 10u);

         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(0), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(2), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(9), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(10), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(11), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(12), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(13), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(14), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(15), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(16), 2u);
         //hist.display(std::cerr);
      }


      /// Tests a fix histogram with bins of size 5.
      void testFixBinHistogram_5() {
         HIST hist(new fixBinner(5));

         hist.add(0);  // 0
         hist.add(2);  // 0
         hist.add(10);  // 2
         hist.add(12);  // 2
         hist.add(20);  // 4
         hist.add(200);  // 40

         TS_ASSERT_EQUALS(hist.getNum(), 6u);

         TS_ASSERT_EQUALS(hist.getNumBin(), 4u);

         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(0), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(2), 2u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(4), 1u);
         TS_ASSERT_EQUALS(hist.getNumDataPointInBin(40), 1u);

         //hist.display(std::cerr);
      }
}; // TestSimpleHistogram

} // Portage

#endif // __TEST_SIMPLE_HISTOGRAM_H__
