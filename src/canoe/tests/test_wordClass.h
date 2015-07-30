/**
 * @author Samuel Larkin
 * @file wordClass.h  Test suite for IWordClass.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "wordClass.h"

using namespace Portage;

namespace Portage {

class TestWordClass : public CxxTest::TestSuite 
{
public:

   void testTightlyPacked() {
      IWordClass* wc = loadClasses("tests/data/test_wordClass.mmMap");
      TS_ASSERT(wc != NULL);
      TS_ASSERT(!wc->empty());

      IWordClass::findType A = wc->find("A");
      TS_ASSERT(A.first == true);
      TS_ASSERT_SAME_DATA(A.second, "1", 1);

      IWordClass::findType B = wc->find("BB");
      TS_ASSERT(B.first == true);
      TS_ASSERT_SAME_DATA(B.second, "1", 1);

      IWordClass::findType C = wc->find("C");
      TS_ASSERT(C.first == true);
      TS_ASSERT_SAME_DATA(C.second, "cc", 2);

      IWordClass::findType D = wc->find("DDD");
      TS_ASSERT(D.first == true);
      TS_ASSERT_SAME_DATA(D.second, "d", 1);

      IWordClass::findType E = wc->find("E");
      TS_ASSERT(E.first == true);
      TS_ASSERT_SAME_DATA(E.second, "1", 1);

      IWordClass::findType Z = wc->find("D");
      TS_ASSERT(Z.first == false);
   }

   void testText() {
      IWordClass* wc = loadClasses("tests/test_wordClass.txt");
      TS_ASSERT(wc != NULL);
      TS_ASSERT(!wc->empty());

      IWordClass::findType A = wc->find("A");
      TS_ASSERT(A.first == true);
      TS_ASSERT_SAME_DATA(A.second, "1", 1);

      IWordClass::findType B = wc->find("BB");
      TS_ASSERT(B.first == true);
      TS_ASSERT_SAME_DATA(B.second, "1", 1);

      IWordClass::findType C = wc->find("C");
      TS_ASSERT(C.first == true);
      TS_ASSERT_SAME_DATA(C.second, "cc", 2);

      IWordClass::findType D = wc->find("DDD");
      TS_ASSERT(D.first == true);
      TS_ASSERT_SAME_DATA(D.second, "d", 1);

      IWordClass::findType E = wc->find("E");
      TS_ASSERT(E.first == true);
      TS_ASSERT_SAME_DATA(E.second, "1", 1);

      IWordClass::findType Z = wc->find("D");
      TS_ASSERT(Z.first == false);
   }

}; // TestWordClass

} // Portage
