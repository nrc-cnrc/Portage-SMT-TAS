/**
 * @author Colin Cherry
 * @file test_shift_reducer.h  Test suite for ShiftReducer
 *
 * Tests that shift-reduce parsing and recombination is working
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include <vector>
#include "shift_reducer.h"

using namespace Portage;

namespace Portage {

   class TestShiftReducer : public CxxTest::TestSuite {
   public:
      void test2413Parsing()
      {
         Uint numNonITG;

         numNonITG = ShiftReducer::getNonITGCount();
         Range r1[4] = {Range(0,1),Range(1,2),Range(2,3),Range(3,4)};
         ShiftReducer* sr1 = parseRanges(r1, 4, Range(0,4));
         TS_ASSERT(ShiftReducer::getNonITGCount()==numNonITG);

         numNonITG = ShiftReducer::getNonITGCount();
         Range r2[2] = {Range(3,4),Range(0,3)};
         ShiftReducer* sr2 = parseRanges(r2, 2, Range(0,4));
         TS_ASSERT(ShiftReducer::getNonITGCount()==numNonITG);

         numNonITG = ShiftReducer::getNonITGCount();
         Range r3[4] = {Range(1,2),Range(3,4),Range(0,1),Range(2,3)};
         ShiftReducer* sr3 = parseRanges(r3, 4, Range(0,4));
         TS_ASSERT(ShiftReducer::getNonITGCount()==numNonITG+1);
         
         TS_ASSERT(ShiftReducer::isRecombinable(sr1,sr2));
         TS_ASSERT(ShiftReducer::isRecombinable(sr1,sr3));
         TS_ASSERT_EQUALS(sr1->computeRecombHash(),
                          sr2->computeRecombHash());
         TS_ASSERT_EQUALS(sr1->computeRecombHash(),
                          sr3->computeRecombHash());
      }

      void testRecomb()
      {
         Range r1[3] = {Range(3,4),Range(0,1),Range(1,2)};
         Range r2[3] = {Range(3,4),Range(1,2),Range(0,1)};
         Range r3[3] = {Range(4,5),Range(0,1),Range(1,2)};
   
         ShiftReducer* sr1 = parseRanges(r1,3,Range(0,2));
         ShiftReducer* sr2 = parseRanges(r2,3,Range(0,2));
         ShiftReducer* sr3 = parseRanges(r3,3,Range(0,2));
         
         TS_ASSERT(ShiftReducer::isRecombinable(sr1,sr2));
         TS_ASSERT_EQUALS(sr1->computeRecombHash(),
                          sr2->computeRecombHash());

         TS_ASSERT(!ShiftReducer::isRecombinable(sr1,sr3));
      }

   private:
      ShiftReducer* parseRanges(Range ranges[], Uint size, Range answer)
      {
         ShiftReducer* sr = new ShiftReducer(10);
         for(Uint i=0;i<size;i++)
         {
            sr = new ShiftReducer(ranges[i],sr);
         }
         TS_ASSERT_EQUALS(sr->start(),answer.start);
         TS_ASSERT_EQUALS(sr->end(),answer.end);
         return sr;
      }
   };
}
   
