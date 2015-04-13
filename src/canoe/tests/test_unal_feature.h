/**
 * @author Eric Joanis
 * @file test_unal_feature.h  Test suite for UnalFeature
 *
 * This class is to test the various ways to count unaligned words in UnalFeature.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "unal_feature.h"
#include "phrasetable.h"
#include "tmp_val.h"

using namespace Portage;

namespace Portage {

class TestUnalFeature : public CxxTest::TestSuite {
   vector<UnalFeature> f;
   PhraseInfo fbpi;
public:
   void setUp() {
      f.push_back(UnalFeature("any"));
      f.push_back(UnalFeature("edges"));
      f.push_back(UnalFeature("left"));
      f.push_back(UnalFeature("right"));
      f.push_back(UnalFeature("srcany"));
      f.push_back(UnalFeature("srcedges"));
      f.push_back(UnalFeature("srcleft"));
      f.push_back(UnalFeature("srcright"));
      f.push_back(UnalFeature("tgtany"));
      f.push_back(UnalFeature("tgtedges"));
      f.push_back(UnalFeature("tgtleft"));
      f.push_back(UnalFeature("tgtright"));
      f.push_back(UnalFeature("tgtright+srcright"));
      f.push_back(UnalFeature("any+edges+srcany+tgtany"));
      fbpi.src_words = Range(3,9); // source phrase of length 6
      fbpi.phrase.resize(5); // target phrase of length 5
      Error_ns::Current::errorCallback = Error_ns::nullErrorCallBack; // silence errors
   }
   void tearDown() {
      f.clear();
   }
   void test1() {
      fbpi.annotations.initAnnotation("a", "-_3,4_-_1_-_-");
      double answers[] = {6, 4, 2, 2, 4, 3, 1, 2, 2, 1, 1, 0, 2, 16};
      check_answers(answers);
   }
   void test2() {
      fbpi.annotations.initAnnotation("a", "3_3_-_1_-_1");
      double answers[] = {5, 2, 1, 1, 2, 0, 0, 0, 3, 2, 1, 1, 1, 12};
      check_answers(answers);
   }
   void test_insect_phrase_pair() {
      fbpi.src_words = Range(1,2);
      fbpi.phrase.resize(1);
      fbpi.annotations.initAnnotation("a", "-");
      double answers[] = {2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 6};
      check_answers(answers);
   }
   void test_trivial() {
      fbpi.src_words = Range(1,2);
      fbpi.phrase.resize(1);
      fbpi.annotations.initAnnotation("a", "0");
      double answers[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      check_answers(answers);
   }
   void test_simple() {
      fbpi.src_words = Range(1,4);
      fbpi.phrase.resize(4);
      fbpi.annotations.initAnnotation("a", "3_2_1");
      double answers[] = {1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 3};
      check_answers(answers);
   }
   void test_cache_hit() {
      fbpi.src_words = Range(1,4);
      fbpi.phrase.resize(4);
      fbpi.annotations.initAnnotation("a", "3_2_1");
      double answers[] = {1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 3};
      check_answers(answers, 'i');
      fbpi.phrase.resize(6);
      fbpi.annotations.initAnnotation("a", "3_2_1");
      double answers2[] = {3, 3, 1, 2, 0, 0, 0, 0, 3, 3, 1, 2, 2, 9};
      check_answers(answers2, 'j');
      fbpi.phrase.resize(4);
      check_answers(answers, 'k');
      fbpi.phrase.resize(6);
      check_answers(answers2, 'l');
   }
   void check_answers(double* answers, char prefix = 'i') {
      char message[] = "i=0";
      message[0] = prefix;
      for ( Uint i = 0; i < f.size(); ++i ) {
         message[2] = 'a' + i;
         TSM_ASSERT_EQUALS(message, f[i].precomputeFutureScore(fbpi), 0.0 - answers[i]);
      }
   }
}; // class TestUnalFeature


} // namespace Portage
