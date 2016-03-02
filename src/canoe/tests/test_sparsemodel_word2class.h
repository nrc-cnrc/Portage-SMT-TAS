/**
 * @author Samuel Larkin
 * @file test_sparsemodel_word2class.h  Test suite for IWord2Classes
 *
 * This file, test_your_class.h, written by Eric Joanis, is intended as a
 * template for your own unit test suites.  Copy it to tests/test_<YOUR_CLASS>.h,
 * remove all the irrelevant content (such as this paragraph and all the sample
 * test cases below), substitute all instances of "your class" with appropriate
 * text describing yours, and you're ready to go: "make test" will now run
 * your test suite among the rest.
 * Full documentation of CxxTest is here:
 *      http://cxxtest.sourceforge.net/guide.html
 * and the list of assertions is here:
 *      http://cxxtest.sourceforge.net/guide.html#TOC8
 * And lots of examples exist in our own tests/test*.h files.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "sparsemodel.h"

using namespace Portage;

namespace Portage {

class TestSparseModelWord2ClassesMapping : public CxxTest::TestSuite, public SparseModel
{
   const Uint unknown;
   const string wordClassFilename;

public:
   TestSparseModelWord2ClassesMapping()
      : unknown(-1)
      , wordClassFilename("tests/test_sparsemodel_word2class")
   {
      ostringstream classMap;
      classMap << "DDD" << '\t' << 1 << endl;
      classMap << "A" << '\t' << 2 << endl;
      classMap << "E" << '\t' << 3 << endl;
      classMap << "C" << ' ' << 0 << endl;
      classMap << "BB" << ' ' << 1 << endl;  // NOTE: BB & DDD are both in cluster 1

      oMagicStream text(wordClassFilename + ".txt");
      text << classMap.str();

      oMagicStream mmmap(wordClassFilename + ".MMmap");
      istringstream is(classMap.str());
      mkMemoryMappedMap(is, mmmap);
   }

   void testWord2ClassesTextMapper() {
      Voc vocab;
      //vocab.add("A");  // Let's omit "A" from the vocabulary but it is in the word2classes map.
      vocab.add("BB");
      vocab.add("C");
      vocab.add("DDD");
      vocab.add("E");
      vocab.add("NotInWord2Classes");
      vocab.add("AlsoNotInWord2Classes");
      TS_ASSERT_EQUALS(vocab.index("A"), vocab.size());

      IWordClassesMapping* mapper = getWordClassesMapper(wordClassFilename + ".txt", vocab, unknown);

      TS_ASSERT(mapper != NULL);
      TS_ASSERT_EQUALS(mapper->numClustIds(), 4);
      TS_ASSERT(vocab.index("A") < vocab.size());

      TS_ASSERT_EQUALS((*mapper)(vocab.index("DDD")), 1);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("A")), 2);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("E")), 3);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("C")), 0);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("BB")), 1);
      // Let's test a word that is not part of the map.
      TS_ASSERT_EQUALS((*mapper)(vocab.index("NotInWord2Classes")), unknown);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("AlsoNotInWord2Classes")), unknown);
   }

   void testWord2ClassesMemoryMapMapper() {
      Voc vocab;
      //vocab.add("A");  // Let's omit "A" from the vocabulary but it is in the word2classes map.
      vocab.add("BB");
      vocab.add("C");
      vocab.add("DDD");
      vocab.add("E");
      vocab.add("NotInWord2Classes");
      vocab.add("AlsoNotInWord2Classes");
      TS_ASSERT_EQUALS(vocab.index("A"), vocab.size());

      IWordClassesMapping* mapper = getWordClassesMapper(wordClassFilename + ".MMmap", vocab, unknown);

      TS_ASSERT(mapper != NULL);
      TS_ASSERT_EQUALS(mapper->numClustIds(), 4);
      TS_ASSERT_EQUALS(vocab.index("A"), vocab.size());

      TS_ASSERT_EQUALS((*mapper)(vocab.index("DDD")), 1);
      //TS_ASSERT_EQUALS((*mapper)(vocab.index("A")), 2);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("E")), 3);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("C")), 0);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("BB")), 1);
      // Let's test a word that is not part of the map.
      TS_ASSERT_EQUALS((*mapper)(vocab.index("NotInWord2Classes")), unknown);
      TS_ASSERT_EQUALS((*mapper)(vocab.index("AlsoNotInWord2Classes")), unknown);
   }

}; // TestSparseModelIWord2Classes

} // Portage
