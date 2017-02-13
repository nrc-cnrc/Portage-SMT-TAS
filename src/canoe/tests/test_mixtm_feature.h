/**
 * @author Eric Joanis
 * @file test_mixtm_feature.h
 *
 * Unit test for mixtm_feature.h
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "mixtm_feature.h"
#include "config_io.h"

using namespace Portage;

namespace Portage {

class TestMixTMFeature : public CxxTest::TestSuite {
   string filename;
   string mixtppt;
   vector<string> tppts;
public:
   TestMixTMFeature() {
      const char* pPortageEnv = getenv("PORTAGE");
      string PortageEnv(pPortageEnv ? pPortageEnv : "../..");
      tppts.push_back(PortageEnv + "/test-suite/systems/toy-regress-ch2en/models/tm/cpt.merged.hmm3-kn3-zn.tm-train.ch2en.tppt");
      tppts.push_back(PortageEnv + "/test-suite/systems/toy-regress-en2fr/models/tm/cpt.merged.hmm3-kn3-zn.tm-train.en2fr.tppt");
      tppts.push_back(PortageEnv + "/test-suite/systems/toy-regress-fr2en/models/tm/cpt.merged.hmm3-kn3-zn.tm-train.fr2en.tppt");
   }

   void setUp() {
      filename = "test_mixtm_feature.mixtm";
      oSafeMagicStream out(filename);
      out << MixTMFeature::magicNumber << endl;
      out << "cpt1\t0.1 0.3 0.2" << endl;
      out << "cpt2\t0.5 0.2 0.7" << endl;
      out << "cpt3\t0.4 0.5 0.1" << endl;

      mixtppt = "test_mixtm_tppts.mixtm"; 
      oSafeMagicStream out2(mixtppt);
      out2 << MixTMFeature::magicNumber << endl;
      out2 << tppts[0] << "\t0.1 0.3 0.2 0.5" << endl;
      out2 << tppts[1] << "\t0.5 0.2 0.7 0.1" << endl;
      out2 << tppts[2] << "\t0.4 0.5 0.1 0.4" << endl;

      Error_ns::Current::errorCallback = Error_ns::nullErrorCallBack; // silence errors
   }
   void tearDown() {
      unlink(filename.c_str());
      unlink(mixtppt.c_str());
   }

   void testIsA() {
      TS_ASSERT(MixTMFeature::isA("test.mixtm"));
      TS_ASSERT(!MixTMFeature::isA("test.blah"));
      TS_ASSERT(MixTMFeature::isA(filename));
   }

   void testCreator() {
      MixTMFeature::Creator creator(filename);
      TS_ASSERT(!creator.checkFileExists(NULL));
   }

   void testCheckFileExists() {
      TS_ASSERT(!PhraseTableFeature::checkFileExists("tests/data/mix12.mixtm"));
      TS_ASSERT(!PhraseTableFeature::checkFileExists("tests/data/mix34.mixtm"));
      TS_ASSERT(PhraseTableFeature::checkFileExists("tests/data/mixtppt12.mixtm"));
      TS_ASSERT(PhraseTableFeature::checkFileExists("tests/data/mixtppt34.mixtm"));
      TS_ASSERT(!PhraseTableFeature::checkFileExists("tests/data/mixtppt12-bad.mixtm"));
      TS_ASSERT(!PhraseTableFeature::checkFileExists("tests/data/mixtppt13.mixtm"));
      TS_ASSERT(!PhraseTableFeature::checkFileExists("tests/data/mix0.mixtm"));
      TS_ASSERT(PhraseTableFeature::checkFileExists(mixtppt));
   }

   void testGetNumScores() {
      Uint numScores(-1), numAdir(-1), numCounts(-1);
      bool hasAlignments(true);
      PhraseTableFeature::getNumScores(filename, numScores, numAdir, numCounts, hasAlignments);
      TS_ASSERT_EQUALS(numScores, 0);
      TS_ASSERT_EQUALS(numAdir, 0);
      TS_ASSERT_EQUALS(numCounts, 0);
      TS_ASSERT_EQUALS(hasAlignments, false);

      numScores = numAdir = numCounts = Uint(-1);
      hasAlignments = true;
      PhraseTableFeature::getNumScores("tests/data/mixtppt12.mixtm", numScores, numAdir, numCounts, hasAlignments);
      TS_ASSERT_EQUALS(numScores, 2);
      TS_ASSERT_EQUALS(numAdir, 0);
      TS_ASSERT_EQUALS(numCounts, 0);
      TS_ASSERT_EQUALS(hasAlignments, false);

      numScores = numAdir = numCounts = Uint(-1);
      hasAlignments = false;
      PhraseTableFeature::getNumScores("tests/data/mixtppt34.mixtm", numScores, numAdir, numCounts, hasAlignments);
      TS_ASSERT_EQUALS(numScores, 1);
      TS_ASSERT_EQUALS(numAdir, 1);
      TS_ASSERT_EQUALS(numCounts, 1);
      TS_ASSERT_EQUALS(hasAlignments, true);

      numScores = numAdir = numCounts = Uint(-1);
      hasAlignments = true;
      PhraseTableFeature::getNumScores("tests/data/mixtppt12-bad.mixtm", numScores, numAdir, numCounts, hasAlignments);
      TS_ASSERT_EQUALS(numScores, 0);
      TS_ASSERT_EQUALS(numAdir, 0);
      TS_ASSERT_EQUALS(numCounts, 0);
      TS_ASSERT_EQUALS(hasAlignments, false);

      numScores = numAdir = numCounts = Uint(-1);
      hasAlignments = true;
      PhraseTableFeature::getNumScores("tests/data/mixtppt13.mixtm", numScores, numAdir, numCounts, hasAlignments);
      TS_ASSERT_EQUALS(numScores, 0);
      TS_ASSERT_EQUALS(numAdir, 0);
      TS_ASSERT_EQUALS(numCounts, 0);
      TS_ASSERT_EQUALS(hasAlignments, false);
   }

   void testTotalMemmapSize() {
      TS_ASSERT_EQUALS(0, PhraseTableFeature::totalMemmapSize("tests/data/mix12.mixtm"));

      Uint64 memmap1 = PhraseTableFeature::totalMemmapSize(tppts[0]);
      Uint64 memmap2 = PhraseTableFeature::totalMemmapSize(tppts[1]);
      Uint64 memmap3 = PhraseTableFeature::totalMemmapSize(tppts[2]);
      TS_ASSERT(memmap1 > 0);
      TS_ASSERT(memmap2 > 0);
      TS_ASSERT(memmap3 > 0);
      TS_ASSERT_EQUALS(PhraseTableFeature::totalMemmapSize(mixtppt), memmap1 + memmap2 + memmap3);
   }

   void testCreateBad() {
      CanoeConfig c;
      Voc v;
      TS_ASSERT(PhraseTableFeature::create(filename, c, v) == NULL);
      TS_ASSERT(PhraseTableFeature::create("tests/data/mixtppt13.mixtm", c, v) == NULL);
      TS_ASSERT(PhraseTableFeature::create("tests/data/mixtppt12-bad.mixtm", c, v) == NULL);
   }
      
   string displayAnnotation(TScore &tscore) {
      ostringstream oss;
      tscore.annotations.write(oss);
      return oss.str();
   }

   void testMix12() {
      CanoeConfig c;
      Voc v;
      PhraseTableFeature* pt = PhraseTableFeature::create("tests/data/mixtppt12.mixtm", c, v);
      MixTMFeature* mixpt = dynamic_cast<MixTMFeature*>(pt);
      TS_ASSERT(mixpt != NULL);
      TS_ASSERT_EQUALS(pt->getNumModels(), 2);
      TS_ASSERT_EQUALS(pt->getNumAdir(), 0);
      TS_ASSERT_EQUALS(pt->getNumCounts(), 0);
      TS_ASSERT_EQUALS(pt->hasAlignments(), false);

      vector<string> sentence;
      sentence.push_back(string("a"));
      sentence.push_back(string("b"));
      sentence.push_back(string("c"));
      sentence.push_back(string("d"));
      pt->newSrcSent(sentence);
      shared_ptr<TargetPhraseTable> tpt;

      // find("a") -> {"b", "c", "e"}
      tpt = pt->find(Range(0,1));
      TS_ASSERT_EQUALS(tpt->size(), 3);
      VectorPhrase phrase(1);
      phrase[0] = v.add("b");
      TScore tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.19, 0.00001);
      TS_ASSERT_DELTA(tscore.backward[1], 0.28, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.37, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[1], 0.46, 0.00001);
      TS_ASSERT(tscore.adir.empty());
      TS_ASSERT_EQUALS(displayAnnotation(tscore), "");

      phrase[0] = v.add("c");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.01, 0.00001);
      TS_ASSERT_DELTA(tscore.backward[1], 0.04, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.09, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[1], 0.16, 0.00001);
      TS_ASSERT(tscore.adir.empty());

      phrase[0] = v.add("e");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.18, 0.00001);
      TS_ASSERT_DELTA(tscore.backward[1], 0.24, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.28, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[1], 0.30, 0.00001);
      TS_ASSERT(tscore.adir.empty());

      // find("b") -> {"d"}
      tpt = pt->find(Range(1,2));
      TS_ASSERT_EQUALS(tpt->size(), 1);
      phrase[0] = v.add("d");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.01, 0.00001);
      TS_ASSERT_DELTA(tscore.backward[1], 0.04, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.09, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[1], 0.16, 0.00001);

      // find("c") -> {"a"}
      tpt = pt->find(Range(2,3));
      TS_ASSERT_EQUALS(tpt->size(), 1);
      phrase[0] = v.add("a");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.18, 0.00001);
      TS_ASSERT_DELTA(tscore.backward[1], 0.24, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.28, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[1], 0.30, 0.00001);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), "");

      // find("d") -> {}
      tpt = pt->find(Range(3,4));
      TS_ASSERT_EQUALS(tpt->size(), 0);
   }

   void testMix34() {
      CanoeConfig c;
      Voc v;
      PhraseTableFeature* pt = PhraseTableFeature::create("tests/data/mixtppt34.mixtm", c, v);
      MixTMFeature* mixpt = dynamic_cast<MixTMFeature*>(pt);
      TS_ASSERT(mixpt != NULL);
      TS_ASSERT_EQUALS(pt->getNumModels(), 1);
      TS_ASSERT_EQUALS(pt->getNumAdir(), 1);
      TS_ASSERT_EQUALS(pt->getNumCounts(), 1);
      TS_ASSERT_EQUALS(pt->hasAlignments(), true);

      vector<string> sentence;
      sentence.push_back(string("c"));
      sentence.push_back(string("d"));
      sentence.push_back(string("e"));
      sentence.push_back(string("f"));
      pt->newSrcSent(sentence);
      shared_ptr<TargetPhraseTable> tpt;

      // find("c") -> {"a", "b", "c", "d"}
      tpt = pt->find(Range(0,1));
      TS_ASSERT_EQUALS(tpt->size(), 4);
      VectorPhrase phrase(1);
      phrase[0] = v.add("a");
      TScore tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.19, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.28, 0.00001);
      TS_ASSERT_DELTA(tscore.adir[0], 0.37, 0.00001);
      TS_ASSERT(tscore.backward.size() == 1 && tscore.forward.size() == 1 && tscore.adir.size() == 1);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), " a=0 c=8");

      phrase[0] = v.add("b");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.01, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.04, 0.00001);
      TS_ASSERT_DELTA(tscore.adir[0], 0.09, 0.00001);
      TS_ASSERT(tscore.backward.size() == 1 && tscore.forward.size() == 1 && tscore.adir.size() == 1);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), " a=- c=10");

      phrase[0] = v.add("c");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.18, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.24, 0.00001);
      TS_ASSERT_DELTA(tscore.adir[0], 0.28, 0.00001);
      TS_ASSERT(tscore.backward.size() == 1 && tscore.forward.size() == 1 && tscore.adir.size() == 1);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), " a=0 c=1");

      phrase[0] = v.add("d");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.01, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.04, 0.00001);
      TS_ASSERT_DELTA(tscore.adir[0], 0.09, 0.00001);
      TS_ASSERT(tscore.backward.size() == 1 && tscore.forward.size() == 1 && tscore.adir.size() == 1);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), " a=0 c=1");

      // find("d") -> {"e f"}
      tpt = pt->find(Range(1,2));
      TS_ASSERT_EQUALS(tpt->size(), 1);
      phrase[0] = v.add("e");
      phrase.push_back(v.add("f"));
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.01, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.04, 0.00001);
      TS_ASSERT_DELTA(tscore.adir[0], 0.09, 0.00001);
      TS_ASSERT(tscore.backward.size() == 1 && tscore.forward.size() == 1 && tscore.adir.size() == 1);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), " a=0,1 c=1");
      phrase.pop_back();

      // find("e") -> {"f"}
      tpt = pt->find(Range(2,3));
      TS_ASSERT_EQUALS(tpt->size(), 1);
      phrase[0] = v.add("f");
      tscore = tpt->find(phrase)->second;
      TS_ASSERT_DELTA(tscore.backward[0], 0.9, 0.00001);
      TS_ASSERT_DELTA(tscore.forward[0], 0.8, 0.00001);
      TS_ASSERT_DELTA(tscore.adir[0], 0.7, 0.00001);
      TS_ASSERT(tscore.backward.size() == 1 && tscore.forward.size() == 1 && tscore.adir.size() == 1);
      TS_ASSERT_EQUALS(displayAnnotation(tscore), " c=0");

      // find("f") -> {}
      tpt = pt->find(Range(3,4));
      TS_ASSERT_EQUALS(tpt->size(), 0);
   }

   void testMixBigTPPT() {
      CanoeConfig c;
      Voc v;
      PhraseTableFeature* pt = PhraseTableFeature::create(mixtppt, c, v);
      MixTMFeature* mixpt = dynamic_cast<MixTMFeature*>(pt);
      TS_ASSERT(mixpt != NULL);
      TS_ASSERT_EQUALS(pt->getNumModels(), 2);
      TS_ASSERT_EQUALS(pt->getNumAdir(), 0);
      TS_ASSERT_EQUALS(pt->getNumCounts(), 0);
      TS_ASSERT_EQUALS(pt->hasAlignments(), true);
   }
      

}; // class TestMixTMFeature

} // namespace Portage
