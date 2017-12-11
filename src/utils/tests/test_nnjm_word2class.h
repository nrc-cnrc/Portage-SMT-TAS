/**
 * @author Samuel Larkin
 * @file nnjm.h  Test suite for IWordClassesMapping
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "file_utils.h"
#include "wordClassMapper.h"

using namespace Portage;

namespace Portage {

class TestNNJMIWord2ClassesMapper : public CxxTest::TestSuite
{
   const string unknown;
   const string wordClassFilename;

public:
   TestNNJMIWord2ClassesMapper()
      : unknown("NOT FOUND")
      , wordClassFilename("tests/test_nnjm_word2class")
   {
      ostringstream classMap;
      classMap << "DDD" << '\t' << 1 << endl;
      classMap << "A" << '\t' << 34 << endl;
      classMap << "E" << '\t' << 'e' << endl;
      classMap << "C" << ' ' << 0 << endl;
      classMap << "BB" << ' ' << "bbbb" << endl;

      oMagicStream text(wordClassFilename + ".txt");
      text << classMap.str();

      oMagicStream mmmap(wordClassFilename + ".MMmap");
      istringstream is(classMap.str());
      mkMemoryMappedMap(is, mmmap);
   }

   void testWord2ClassesMemoryMapMapper() {
      IWordClassesMapper* mapper = getWord2ClassesMapper(wordClassFilename + ".MMmap", unknown);

      TS_ASSERT_EQUALS((*mapper)(string("DDD")), "1");
      TS_ASSERT_EQUALS((*mapper)(string("A")), "34");
      TS_ASSERT_EQUALS((*mapper)(string("E")), "e");
      TS_ASSERT_EQUALS((*mapper)(string("C")), "0");
      TS_ASSERT_EQUALS((*mapper)(string("BB")), "bbbb");
      TS_ASSERT_EQUALS((*mapper)(string("not part of the map")), unknown);
      TS_ASSERT_EQUALS((*mapper)(string("1")), unknown);
   }

   void testWord2ClasseTextMapper() {
      IWordClassesMapper* mapper = getWord2ClassesMapper(wordClassFilename + ".txt", unknown);

      TS_ASSERT_EQUALS((*mapper)(string("DDD")), "1");
      TS_ASSERT_EQUALS((*mapper)(string("A")), "34");
      TS_ASSERT_EQUALS((*mapper)(string("E")), "e");
      TS_ASSERT_EQUALS((*mapper)(string("C")), "0");
      TS_ASSERT_EQUALS((*mapper)(string("BB")), "bbbb");
      TS_ASSERT_EQUALS((*mapper)(string("not part of the map")), unknown);
      TS_ASSERT_EQUALS((*mapper)(string("1")), unknown);
   }

}; // TestNNJMIWord2ClassesMapper

} // Portage
