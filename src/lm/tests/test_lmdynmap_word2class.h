/**
 * @author Samuel Larkin
 * @file test_lmdynmap_word2class.h  Test suite for IWordClassesMapping.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2016, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2016, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "lmdynmap.h"

using namespace Portage;

namespace Portage {

class TestLMDynMapWord2ClassesMapping : public CxxTest::TestSuite, public LMDynMap
{
   const string wordClassFilename;

public:
   TestLMDynMapWord2ClassesMapping()
      : wordClassFilename("tests/test_lmdynmap_word2class")
   {
      ostringstream classMap;
      classMap << "DDD" << '\t' << 1 << endl;
      classMap << "A" << '\t' << 2 << endl;
      classMap << "E" << '\t' << 3 << endl;
      classMap << "C" << '\t' << 0 << endl;
      classMap << "BB" << '\t' << 1 << endl;  // NOTE: BB & DDD are both in cluster 1

      oMagicStream text(wordClassFilename + ".txt");
      text << classMap.str();

      oMagicStream mmmap(wordClassFilename + ".MMmap");
      istringstream is(classMap.str());
      mkMemoryMappedMap(is, mmmap);
   }

   void testWord2ClassesTextMapper() {
      string test;

      Voc vocab;
      //vocab.add("A");  // Let's omit "A" from the vocabulary but it is in the word2classes map.
      vocab.add("BB");
      vocab.add("C");
      vocab.add("DDD");
      vocab.add("E");
      vocab.add("NotInWord2Classes");
      vocab.add("AlsoNotInWord2Classes");
      TS_ASSERT_EQUALS(vocab.index("A"), vocab.size());

      IWordClassesMapping* mapper = getWord2ClassesMapper(wordClassFilename + ".txt", &vocab);

      TS_ASSERT(mapper != NULL);

      test = PLM::UNK_Symbol;
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);

      test = PLM::SentStart;
      TS_ASSERT_EQUALS((*mapper)(test), PLM::SentStart);

      test = PLM::SentEnd;
      TS_ASSERT_EQUALS((*mapper)(test), PLM::SentEnd);


      test = "DDD";
      TS_ASSERT_EQUALS((*mapper)(test), "1");

      // "A" is not part of the vocabulary.
      test = "A";
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);

      test = "E";
      TS_ASSERT_EQUALS((*mapper)(test), "3");

      test = "C";
      TS_ASSERT_EQUALS((*mapper)(test), "0");

      test = "BB";
      TS_ASSERT_EQUALS((*mapper)(test), "1");

      // Let's test a word that is not part of the map.
      test = "NotInWord2Classes";
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);
      test = "AlsoNotInWord2Classes";
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);
   }

   void testWord2ClassesMemoryMappedMapper() {
      string test;

      Voc vocab;
      //vocab.add("A");  // Let's omit "A" from the vocabulary but it is in the word2classes map.
      vocab.add("BB");
      vocab.add("C");
      vocab.add("DDD");
      vocab.add("E");
      vocab.add("NotInWord2Classes");
      vocab.add("AlsoNotInWord2Classes");
      TS_ASSERT_EQUALS(vocab.index("A"), vocab.size());

      IWordClassesMapping* mapper = getWord2ClassesMapper(wordClassFilename + ".MMmap", &vocab);

      TS_ASSERT(mapper != NULL);

      test = PLM::UNK_Symbol;
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);

      test = PLM::SentStart;
      TS_ASSERT_EQUALS((*mapper)(test), PLM::SentStart);

      test = PLM::SentEnd;
      TS_ASSERT_EQUALS((*mapper)(test), PLM::SentEnd);


      test = "DDD";
      TS_ASSERT_EQUALS((*mapper)(test), "1");

      // "A" is not part of the vocabulary.
      test = "A";
      TS_ASSERT_EQUALS((*mapper)(test), "2");

      test = "E";
      TS_ASSERT_EQUALS((*mapper)(test), "3");

      test = "C";
      TS_ASSERT_EQUALS((*mapper)(test), "0");

      test = "BB";
      TS_ASSERT_EQUALS((*mapper)(test), "1");

      // Let's test a word that is not part of the map.
      test = "NotInWord2Classes";
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);
      test = "AlsoNotInWord2Classes";
      TS_ASSERT_EQUALS((*mapper)(test), PLM::UNK_Symbol);
   }

}; // TestLMDynMapWord2ClassesMapping

} // Portage
