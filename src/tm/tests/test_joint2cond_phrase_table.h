/**
 * @author Darlene Stewart
 * @file test_joint2cond_phrase_table.h  Tests for PhraseTableGen.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include "phrase_table.h"
#include "phrase_smoother.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"

using namespace Portage;

namespace Portage {

class TestJoint2CondPhraseTable : public CxxTest::TestSuite
{

   struct jpt_data {
      string s;
      string t;
      int cnt;
   };

   static const string sep;
   static const string psep;
   static const struct jpt_data data[];
   static const Uint data_length;
   static const double rf_pl1gl2[];
   static const double rf_pl2gl1[];
   static const Uint prune;
   static const struct jpt_data pruned_data[];
   static const Uint pruned_data_length;
   static const double pruned_rf_pl1gl2[];
   static const double pruned_rf_pl2gl1[];

   string makeDataStream(const struct jpt_data data[], Uint length)
   {
      ostringstream text;
      for (Uint i=0; i < length; ++i) {
         text << data[i].s << psep << data[i].t << psep << data[i].cnt << endl;
      }
      return text.str();
   };

   istringstream *jpt_in;

   void setUp()
   {
      jpt_in = new istringstream(makeDataStream(data, data_length), istringstream::in);
   };

   void tearDown()
   {
      delete jpt_in;
   }

   void initPhraseTableGen(PhraseTableGen<Uint>& ptg, bool reduce_memory,
                           Uint prune1, Uint prune1w, const char *name)
   {
      //cout << "-------- " << name << " reduce_memory: " << reduce_memory
      //     << " prune1: " << prune1 << " prune1w: " << prune1w << endl;
      ptg.readJointTable(*jpt_in, reduce_memory);
      if (prune1 || prune1w)
         ptg.pruneLang2GivenLang1(prune1, prune1w);
   }

   void createSmoothersAndTally(vector< PhraseSmoother<Uint>* >& smoothers,
                                PhraseTableGen<Uint>& ptg)
   {
      vector<string> smoothing_methods(1);
      smoothing_methods[0] = "RFSmoother";
      PhraseSmootherFactory<Uint> smoother_factory(&ptg, NULL, NULL, 0);
      smoother_factory.createSmoothersAndTally(smoothers, smoothing_methods, false);
   }

   void dumpIt(PhraseTableGen<Uint>::iterator &it, Uint i)
   {
      Uint index1 = it.getPhraseIndex(1);
      Uint index2 = it.getPhraseIndex(2);
      string phrase1, phrase2;
      it.getPhrase(1, phrase1);
      it.getPhrase(2, phrase2);
      Uint count = it.getJointFreq();
      cout << "i:" << i << " " << index1 << " " << phrase1 << " "
           << index2 << " " << phrase2 << " " << count << endl;
   }

   void checkIt(PhraseTableGen<Uint>::iterator &it, const struct jpt_data *expected, Uint i)
   {
//      dumpIt(it, i);
      Uint index1 = it.getPhraseIndex(1);
      Uint index2 = it.getPhraseIndex(2);
      string phrase1, phrase2;
      it.getPhrase(1, phrase1);
      it.getPhrase(2, phrase2);
      Uint count = it.getJointFreq();
      TS_ASSERT_EQUALS(index1, expected[i].s[1] - '0' - 1);
      TS_ASSERT_EQUALS(index2, expected[i].t[1] - '0' - 1);
      TS_ASSERT_EQUALS(phrase1, expected[i].s);
      TS_ASSERT_EQUALS(phrase2, expected[i].t);
      TS_ASSERT_EQUALS(count, expected[i].cnt);
   }

public:
   void testReadJointTable(bool reduce_memory, Uint prune1=0, Uint prune1w=0)
   {
      const bool pruning = prune1 || prune1w;
      PhraseTableGen<Uint> ptg;
      initPhraseTableGen(ptg, reduce_memory, prune1, prune1w, "testReadJointTable");

      const struct jpt_data *expected = pruning ? pruned_data : data;
      const Uint expected_len = pruning ? pruned_data_length : data_length;
      PhraseTableGen<Uint>::iterator it = ptg.begin();
      Uint i = 0;
      for ( ; it != ptg.end() && i < expected_len; ++it, ++i)
         checkIt(it, expected, i);
      TS_ASSERT(it == ptg.end());
      for ( ; it != ptg.end(); ++it, ++i);
      TS_ASSERT_EQUALS(i, expected_len);
   }

   void testReadJointTableNoReduceMemoryNoPruning()
   {
      testReadJointTable(false);
   }

   void testReadJointTableReduceMemoryNoPruning()
   {
      testReadJointTable(true);
   }

   void testReadJointTableNoReduceMemoryPruning()
   {
      testReadJointTable(false, prune);
   }

   void testReadJointTableReduceMemoryPruning()
   {
      testReadJointTable(true, prune);
   }

   void testReadJointTableNoReduceMemoryPerWordPruning()
   {
      testReadJointTable(false, 0, prune);
   }

   void testReadJointTableReduceMemoryPerWordPruning()
   {
      testReadJointTable(true, 0, prune);
   }

   void testIteratorAssignmentAndCopyConstruction(bool reduce_memory, Uint prune1=0, Uint prune1w=0)
   {
      const bool pruning = prune1 || prune1w;
      PhraseTableGen<Uint> ptg;
      initPhraseTableGen(ptg, reduce_memory, prune1, prune1w, "testIteratorAssignmentAndCopyConstruction");

      const struct jpt_data *expected = pruning ? pruned_data : data;
      const Uint expected_len = pruning ? pruned_data_length : data_length;
      PhraseTableGen<Uint>::iterator it = ptg.begin();
      Uint i = 0;
      for (; it != ptg.end() && i < expected_len; ++it, ++i) {
         // Using a copy constructor, take stream ownership from 'it'.
         PhraseTableGen<Uint>::iterator it_copy = it;
         checkIt(it_copy, expected, i);
         // Using assignment operator, return ownership to 'it'.
         it = it_copy;
      }
      TS_ASSERT(it == ptg.end());
      for ( ; it != ptg.end(); ++it, ++i);
      TS_ASSERT_EQUALS(i, expected_len);
   }

   void testIteratorAssignmentAndCopyConstructionNoReduceMemoryNoPruning()
   {
      testIteratorAssignmentAndCopyConstruction(false);
   }

   void testIteratorAssignmentAndCopyConstructionReduceMemoryNoPruning()
   {
      testIteratorAssignmentAndCopyConstruction(true);
   }

   void testIteratorAssignmentAndCopyConstructionNoReduceMemoryPruning()
   {
      testIteratorAssignmentAndCopyConstruction(false, prune);
   }

   void testIteratorAssignmentAndCopyConstructionReduceMemoryPruning()
   {
      testIteratorAssignmentAndCopyConstruction(true, prune);
   }

   void testIteratorAssignmentAndCopyConstructionNoReduceMemoryPerWordPruning()
   {
      testIteratorAssignmentAndCopyConstruction(false, 0, prune);
   }

   void testIteratorAssignmentAndCopyConstructionReduceMemoryPerWordPruning()
   {
      testIteratorAssignmentAndCopyConstruction(true, 0, prune);
   }

   void testCreateSmootherAndTally(bool reduce_memory)
   {
      PhraseTableGen<Uint> ptg;
      initPhraseTableGen(ptg, reduce_memory, 0, 0, "testCreateSmootherAndTally");

      PhraseSmootherFactory<Uint> sf(&ptg, NULL, NULL, 0);
      PhraseSmoother<Uint>* smoother = sf.createSmootherAndTally("RFSmoother", false);
      TS_ASSERT(smoother != NULL);

      Uint i = 0;
      for (PhraseTableGen<Uint>::iterator it = ptg.begin();
           it != ptg.end(); ++it, ++i) {
         TS_ASSERT_DELTA(smoother->probLang1GivenLang2(it), rf_pl1gl2[i], 0.000001);
         TS_ASSERT_DELTA(smoother->probLang2GivenLang1(it), rf_pl2gl1[i], 0.000001);
      }
      TS_ASSERT_EQUALS(i, data_length);
   }

   void testCreateSmootherAndTallyNoReduceMemory()
   {
      testCreateSmootherAndTally(false);
   }

   void testCreateSmootherAndTallyReduceMemory()
   {
      testCreateSmootherAndTally(true);
   }

   void testCreateSmoothersAndTally(bool reduce_memory, Uint prune1=0, Uint prune1w=0)
   {
      const bool pruning = prune1 || prune1w;
      PhraseTableGen<Uint> ptg;
      initPhraseTableGen(ptg, reduce_memory, prune1, prune1w, "testCreateSmoothersAndTally");

      vector< PhraseSmoother<Uint>* > smoothers;
      createSmoothersAndTally(smoothers, ptg);
      TS_ASSERT_EQUALS(smoothers.size(), 1);
      TS_ASSERT(smoothers[0] != NULL);

      const double *expected_pl1gl2 = pruning ? pruned_rf_pl1gl2 : rf_pl1gl2;
      const double *expected_pl2gl1 = pruning ? pruned_rf_pl2gl1 : rf_pl2gl1;
      Uint i = 0;
      for (PhraseTableGen<Uint>::iterator it = ptg.begin();
           it != ptg.end(); ++it, ++i) {
         TS_ASSERT_DELTA(smoothers[0]->probLang1GivenLang2(it), expected_pl1gl2[i], 0.000001);
         TS_ASSERT_DELTA(smoothers[0]->probLang2GivenLang1(it), expected_pl2gl1[i], 0.000001);
      }
      TS_ASSERT_EQUALS(i, pruning ? pruned_data_length : data_length);
   }

   void testCreateSmoothersAndTallyNoReduceMemoryNoPruning()
   {
      testCreateSmoothersAndTally(false);
   }

   void testCreateSmoothersAndTallyReduceMemoryNoPruning()
   {
      testCreateSmoothersAndTally(true);
   }

   void testCreateSmoothersAndTallyNoReduceMemoryPruning()
   {
      testCreateSmoothersAndTally(false, prune);
   }

   void testCreateSmoothersAndTallyReduceMemoryPruning()
   {
      testCreateSmoothersAndTally(true, prune);
   }

   void testCreateSmoothersAndTallyNoReduceMemoryPerWordPruning()
   {
      testCreateSmoothersAndTally(false, 0, prune);
   }

   void testCreateSmoothersAndTallyReduceMemoryPerWordPruning()
   {
      testCreateSmoothersAndTally(true, 0, prune);
   }

   void testDumpMultiProb(bool reduce_memory, Uint prune1=0, Uint prune1w=0)
   {
      const bool pruning = prune1 || prune1w;
      PhraseTableGen<Uint> ptg;
      initPhraseTableGen(ptg, reduce_memory, prune1, prune1w, "testDumpMultiProb");

      vector< PhraseSmoother<Uint>* > smoothers;
      createSmoothersAndTally(smoothers, ptg);

      ostringstream out;
      dumpMultiProb(out, 1, ptg, smoothers);

      ostringstream expected_out;
      expected_out.precision(9);
      Uint expected_length = pruning ? pruned_data_length : data_length;
      const struct jpt_data *expected_data = pruning ? pruned_data : data;
      const double *expected_pl1gl2 = pruning ? pruned_rf_pl1gl2 : rf_pl1gl2;
      const double *expected_pl2gl1 = pruning ? pruned_rf_pl2gl1 : rf_pl2gl1;
      for (Uint i=0; i < expected_length; ++i) {
         expected_out << expected_data[i].s << psep << expected_data[i].t << psep
                      << expected_pl1gl2[i] << sep << expected_pl2gl1[i] << endl;
      }
      TS_ASSERT_EQUALS(out.str(), expected_out.str());

      out.str("");
      dumpMultiProb(out, 2, ptg, smoothers);

      expected_out.str("");
      for (Uint i=0; i < expected_length; ++i) {
         expected_out << expected_data[i].t << psep << expected_data[i].s << psep
                      << expected_pl2gl1[i] << sep << expected_pl1gl2[i] << endl;
      }
      TS_ASSERT_EQUALS(out.str(), expected_out.str());
   }

   void testDumpMultiProbNoReduceMemoryNoPruning()
   {
      testDumpMultiProb(false);
   }

   void testDumpMultiProbReduceMemoryNoPruning()
   {
      testDumpMultiProb(true);
   }

   void testDumpMultiProbNoReduceMemoryPruning()
   {
      testDumpMultiProb(false, prune);
   }

   void testDumpMultiProbReduceMemoryPruning()
   {
      testDumpMultiProb(true, prune);
   }

   void testDumpMultiProbNoReduceMemoryPerWordPruning()
   {
      testDumpMultiProb(false, 0, prune);
   }

   void testDumpMultiProbReduceMemoryPerWordPruning()
   {
      testDumpMultiProb(true, 0, prune);
   }

};

const string TestJoint2CondPhraseTable::sep(" ");
const string TestJoint2CondPhraseTable::psep(" ||| ");

const struct TestJoint2CondPhraseTable::jpt_data TestJoint2CondPhraseTable::data[] =
{
   {"s1", "t1", 5},
   {"s1", "t2", 1},
   {"s2", "t1", 3},
   {"s2", "t3", 1},
   {"s2", "t4", 1},
   {"s3", "t2", 6},
   {"s3", "t1", 5},
   {"s4", "t3", 5},
   {"s4", "t4", 2},
   {"s4", "t5", 10},
};

const Uint TestJoint2CondPhraseTable::data_length( ARRAY_SIZE(data) );

const double TestJoint2CondPhraseTable::rf_pl1gl2[] =
{
   5./13, 1./7, 3./13, 1./6, 1./3, 6./7, 5./13, 5./6, 2./3, 10./10
};

const double TestJoint2CondPhraseTable::rf_pl2gl1[] =
{
   5./6, 1./6, 3./5, 1./5, 1./5, 6./11, 5./11, 5./17, 2./17, 10./17
};

const Uint TestJoint2CondPhraseTable::prune = 2;

const struct TestJoint2CondPhraseTable::jpt_data TestJoint2CondPhraseTable::pruned_data[] =
{
   {"s1", "t1", 5},
   {"s1", "t2", 1},
   {"s2", "t1", 3},
   {"s2", "t4", 1},
   {"s3", "t2", 6},
   {"s3", "t1", 5},
   {"s4", "t3", 5},
   {"s4", "t5", 10},
};

const Uint TestJoint2CondPhraseTable::pruned_data_length( ARRAY_SIZE(pruned_data) );

const double TestJoint2CondPhraseTable::pruned_rf_pl1gl2[] =
{
   5./13, 1./7, 3./13, 1./1, 6./7, 5./13, 5./5, 10./10
};

const double TestJoint2CondPhraseTable::pruned_rf_pl2gl1[] =
{
   5./6, 1./6, 3./4, 1./4, 6./11, 5./11, 5./15, 10./15
};

} // Portage
