/**
 * @author Evan Stratford
 * @file test_binio_utils.h Tests the Portage::BinIO binary I/O library.
 *
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technolo
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef TEST_BINIO_UTILS_H
#define TEST_BINIO_UTILS_H

#include "binio.h"
#include "binio_maps.h"
#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include <sstream>

using namespace Portage;

namespace Portage {

class MockA {
   char c;
   Uint i;
public:
   MockA() : c(0), i(0) {}
   MockA(Uint i) : c('A'), i(i) {}
   void writebin(ostream& os) const {
      BinIO::writebin(os, c);
      BinIO::writebin(os, i);
   }
   void readbin(istream& is) {
      BinIO::readbin(is, c);
      BinIO::readbin(is, i);
   }
   bool operator==(const MockA& rhs) const {
      return c == rhs.c && i == rhs.i;
   }
}; // class MockA

class MockB {
   char c;
   double d;
   string s;
public:
   MockB() : c(0), d(0.0), s() {}
   MockB(double d, const string& s) : c('B'), d(d), s(s) {}
   void writebin(ostream& os) const {
      BinIO::writebin(os, c);
      BinIO::writebin(os, d);
      BinIO::writebin(os, s);
   }
   void readbin(istream& is) {
      BinIO::readbin(is, c);
      BinIO::readbin(is, d);
      BinIO::readbin(is, s);
   }
   bool operator==(const MockB& rhs) const {
      return c == rhs.c && d == rhs.d && s == rhs.s;
   }
}; // class MockA

class TestBinIOUtils : public CxxTest::TestSuite {
public:
   void testPrimitiveIO() {
      Uint i1 = 5;
      double d1 = 2.718;
      char c1 = 'c';
      ostringstream out;
      BinIO::writebin(out, i1);
      BinIO::writebin(out, d1);
      BinIO::writebin(out, c1);

      Uint i2;
      double d2;
      char c2;
      istringstream in(out.str());
      BinIO::readbin(in, i2);
      TS_ASSERT_EQUALS(i1, i2);
      BinIO::readbin(in, d2);
      TS_ASSERT_EQUALS(d1, d2);
      BinIO::readbin(in, c2);
      TS_ASSERT_EQUALS(c1, c2);
   }

   void testArrayOfPrimitiveIO() {
      const Uint SIZE = 3;
      Uint a1[SIZE] = { 4, 2, 1 };
      ostringstream out;
      BinIO::writebin(out, a1, SIZE);

      Uint* a2 = new Uint[SIZE];
      istringstream in(out.str());
      BinIO::readbin(in, a2, SIZE);
      for (Uint i = 0; i < SIZE; i++) {
         TS_ASSERT_EQUALS(a1[i], a2[i]);
      }
      delete[] a2;
   }

   void testObjectIO() {
      MockA a1(42);
      MockB b1(3.14159265358, "pi == yum");
      ostringstream out;
      BinIO::writebin(out, a1);
      BinIO::writebin(out, b1);
      
      MockA a2;
      MockB b2;
      istringstream in(out.str());
      BinIO::readbin(in, a2);
      TS_ASSERT_EQUALS(a1, a2);
      BinIO::readbin(in, b2);
      TS_ASSERT_EQUALS(b1, b2);
   }

   void testArrayOfObjectIO() {
      const Uint SIZE = 3;
      MockA a1[SIZE] = { MockA(4), MockA(2), MockA(1) };
      ostringstream out;
      BinIO::writebin(out, a1, SIZE);

      MockA* a2 = new MockA[SIZE];
      istringstream in(out.str());
      BinIO::readbin(in, a2, SIZE);
      for (Uint i = 0; i < SIZE; i++) {
         TS_ASSERT_EQUALS(a1[i], a2[i]);
      }
      delete[] a2;
   }

   void testStringIO() {
      const char c[] = "test\0string!\n\r\tyay";
      string s1(c, 18);
      ostringstream out;
      BinIO::writebin(out, s1);

      string s2;
      istringstream in(out.str());
      BinIO::readbin(in, s2);
      TS_ASSERT_EQUALS(s1, s2);
   }

   void testPairIO() {
      pair<Uint, string> p1 = make_pair(42, "the answer");
      ostringstream out;
      BinIO::writebin(out, p1);

      pair<Uint, string> p2;
      istringstream in(out.str());
      BinIO::readbin(in, p2);
      TS_ASSERT_EQUALS(p1, p2);
   }

   void testVectorIO() {
      Uint SIZE = 10;
      vector<Uint> v1(2, 1);
      for (Uint i = 2; i < SIZE; i++) {
         v1.push_back(v1[i-1]+v1[i-2]);
      }
      ostringstream out;
      BinIO::writebin(out, v1);

      vector<Uint> v2;
      istringstream in(out.str());
      BinIO::readbin(in, v2);
      TS_ASSERT_EQUALS(v1.size(), v2.size());
      for (Uint i = 0; i < SIZE; i++) {
         TS_ASSERT_EQUALS(v1[i], v2[i]);
      }
   }

   void testMapIO() {
      map<string, MockA> m1;
      MockA a1(1), a2(2), a3(3);
      m1["foo"] = a1;
      m1["bar"] = a2;
      m1["baz"] = a3;
      ostringstream out;
      BinIO::writebin(out, m1);

      map<string, MockA> m2;
      istringstream in(out.str());
      BinIO::readbin(in, m2);
      map<string, MockA>::const_iterator iter1, iter2;
      TS_ASSERT_EQUALS(m1.size(), m2.size());
      for (iter1 = m1.begin(); iter1 != m1.end(); ++iter1) {
         iter2 = m2.find(iter1->first);
         TS_ASSERT_DIFFERS(iter2, m2.end());
         TS_ASSERT_EQUALS(iter1->second, iter2->second);
      }
   }

   void testNestedIO() {
      map<Uint, map<Uint, string> > mm1;
      mm1[0][0] = "X";
      mm1[0][2] = "X";
      mm1[1][1] = "O";
      mm1[2][0] = "X";
      mm1[2][2] = "X";
      ostringstream out;
      BinIO::writebin(out, mm1);

      map<Uint, map<Uint, string> > mm2;
      istringstream in(out.str());
      BinIO::readbin(in, mm2);
      map<Uint, map<Uint, string> >::const_iterator itera1, itera2;
      map<Uint, string>::const_iterator iterb1, iterb2;
      TS_ASSERT_EQUALS(mm1.size(), mm2.size());
      for (itera1 = mm1.begin(); itera1 != mm1.end(); ++itera1) {
         itera2 = mm2.find(itera1->first);
         TS_ASSERT_DIFFERS(itera2, mm2.end());
         TS_ASSERT_EQUALS((itera1->second).size(), (itera2->second).size());
         for (iterb1 = (itera1->second).begin();
               iterb1 != (itera1->second).end(); ++iterb1) {
            iterb2 = (itera2->second).find(iterb1->first);
            TS_ASSERT_DIFFERS(iterb2, (itera2->second).end());
            TS_ASSERT_EQUALS(iterb1->second, iterb2->second);
         }
      }
   }

}; // class TestBinIOUtils

}

#endif
