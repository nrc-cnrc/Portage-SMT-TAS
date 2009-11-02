/**
 * @author Eric Joanis
 * @file test_bivector.h Test suite for BiVector.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "bivector.h"
#include "file_utils.h"

using namespace Portage;

namespace Portage {

class TestBiVector : public CxxTest::TestSuite 
{
public:
   void testSimple() {
      BiVector<int> v;
      TS_ASSERT_EQUALS(v[4], 0);
      TS_ASSERT_EQUALS(v.size(), 0u);
   }
   void testInsertPos() {
      BiVector<int> v;
      TS_ASSERT_EQUALS(v[4], 0);
      v.setAt(4) = 3;
      TS_ASSERT_EQUALS(v[4], 3);
      TS_ASSERT_EQUALS(v[3], 0);
      TS_ASSERT_EQUALS(v[5], 0);
      TS_ASSERT_EQUALS(v.size(), 1u);
      TS_ASSERT_EQUALS(v.first(), 4);
      TS_ASSERT_EQUALS(v.last(), 4);
   }
   void testInsertNeg() {
      BiVector<int> v;
      TS_ASSERT_EQUALS(v[-4], 0);
      v.setAt(-4) = 3;
      TS_ASSERT_EQUALS(v[-4], 3);
      TS_ASSERT_EQUALS(v[-3], 0);
      TS_ASSERT_EQUALS(v[-5], 0);
      TS_ASSERT_EQUALS(v.size(), 1u);
      TS_ASSERT_EQUALS(v.first(), -4);
      TS_ASSERT_EQUALS(v.last(), -4);
   }
   void testInsertBoth() {
      BiVector<int> v;
      v.setAt(-2) = -2;
      v.setAt(-4) = -4;
      TS_ASSERT_EQUALS(v[-1], 0);
      TS_ASSERT_EQUALS(v[-2], -2);
      TS_ASSERT_EQUALS(v[-3], 0);
      TS_ASSERT_EQUALS(v[-4], -4);
      TS_ASSERT_EQUALS(v[-5], 0);
      TS_ASSERT_EQUALS(v.size(), 3u);
      TS_ASSERT_EQUALS(v.first(), -4);
      TS_ASSERT_EQUALS(v.last(), -2);
   }
   void testInitPos() {
      BiVector<int> v(5,9,42);
      TS_ASSERT_EQUALS(v.size(), 5u)
      TS_ASSERT_EQUALS(v.first(), 5);
      TS_ASSERT_EQUALS(v[4], 0);
      TS_ASSERT_EQUALS(v[5], 42);
      TS_ASSERT_EQUALS(v.last(), 9);
      TS_ASSERT_EQUALS(v[9], 42);
      TS_ASSERT_EQUALS(v[10], 0);
   }
   void testInitPosInsert() {
      BiVector<int> v(5,9,42);
      v.setAt(3) = 3;
      TS_ASSERT_EQUALS(v.size(), 7u)
      TS_ASSERT_EQUALS(v.first(), 3);
      TS_ASSERT_EQUALS(v[2], 0);
      TS_ASSERT_EQUALS(v[3], 3);
      TS_ASSERT_EQUALS(v[4], 0);
      TS_ASSERT_EQUALS(v[5], 42);
      TS_ASSERT_EQUALS(v.last(), 9);
      TS_ASSERT_EQUALS(v[9], 42);
      TS_ASSERT_EQUALS(v[10], 0);
   }
   void testInitPosInsertAfter() {
      BiVector<int> v(5,9,42);
      v.setAt(12) = 3;
      TS_ASSERT_EQUALS(v.size(), 8u)
      TS_ASSERT_EQUALS(v.first(), 5);
      TS_ASSERT_EQUALS(v[4], 0);
      TS_ASSERT_EQUALS(v[5], 42);
      TS_ASSERT_EQUALS(v.last(), 12);
      TS_ASSERT_EQUALS(v[9], 42);
      TS_ASSERT_EQUALS(v[10], 0);
      TS_ASSERT_EQUALS(v[11], 0);
      TS_ASSERT_EQUALS(v[12], 3);
      TS_ASSERT_EQUALS(v[13], 0);
   }
   void testInitNeg() {
      BiVector<int> v(-5,-3,42);
      TS_ASSERT_EQUALS(v.size(), 3u)
      TS_ASSERT_EQUALS(v.first(), -5);
      TS_ASSERT_EQUALS(v.last(), -3);
      TS_ASSERT_EQUALS(v[-6], 0);
      TS_ASSERT_EQUALS(v[-5], 42);
      TS_ASSERT_EQUALS(v[-4], 42);
      TS_ASSERT_EQUALS(v[-3], 42);
      TS_ASSERT_EQUALS(v[-2], 0);
   }
   void testInitNegInsert() {
      BiVector<int> v(-5,-3,42);
      v.setAt(-7) = 3;
      v.setAt(-1) = 6;
      v.setAt(-4) += 3;
      TS_ASSERT_EQUALS(v.size(), 7u)
      TS_ASSERT_EQUALS(v.first(), -7);
      TS_ASSERT_EQUALS(v.last(), -1);
      TS_ASSERT_EQUALS(v[-8], 0);
      TS_ASSERT_EQUALS(v[-7], 3);
      TS_ASSERT_EQUALS(v[-6], 0);
      TS_ASSERT_EQUALS(v[-5], 42);
      TS_ASSERT_EQUALS(v[-4], 45);
      TS_ASSERT_EQUALS(v[-3], 42);
      TS_ASSERT_EQUALS(v[-2], 0);
      TS_ASSERT_EQUALS(v[-1], 6);
      TS_ASSERT_EQUALS(v[-0], 0);
   }
   void testSetRange() {
      BiVector<int> v;
      v.setRange(-5,-3,42);
      TS_ASSERT_EQUALS(v.size(), 3u)
      TS_ASSERT_EQUALS(v.first(), -5);
      TS_ASSERT_EQUALS(v.last(), -3);
      TS_ASSERT_EQUALS(v[-6], 0);
      TS_ASSERT_EQUALS(v[-5], 42);
      TS_ASSERT_EQUALS(v[-4], 42);
      TS_ASSERT_EQUALS(v[-3], 42);
      TS_ASSERT_EQUALS(v[-2], 0);
      v.setRange(-4,-2,63);
      TS_ASSERT_EQUALS(v.size(), 4u)
      TS_ASSERT_EQUALS(v.first(), -5);
      TS_ASSERT_EQUALS(v.last(), -2);
      TS_ASSERT_EQUALS(v[-6], 0);
      TS_ASSERT_EQUALS(v[-5], 42);
      TS_ASSERT_EQUALS(v[-4], 63);
      TS_ASSERT_EQUALS(v[-3], 63);
      TS_ASSERT_EQUALS(v[-2], 63);
      TS_ASSERT_EQUALS(v[-1], 0);
      v.setRange(-7,-4,21);
      TS_ASSERT_EQUALS(v.size(), 6u)
      TS_ASSERT_EQUALS(v.first(), -7);
      TS_ASSERT_EQUALS(v.last(), -2);
      TS_ASSERT_EQUALS(v[-8], 0);
      TS_ASSERT_EQUALS(v[-7], 21);
      TS_ASSERT_EQUALS(v[-6], 21);
      TS_ASSERT_EQUALS(v[-5], 21);
      TS_ASSERT_EQUALS(v[-4], 21);
      TS_ASSERT_EQUALS(v[-3], 63);
      TS_ASSERT_EQUALS(v[-2], 63);
      TS_ASSERT_EQUALS(v[-1], 0);
      v.setRange(-5,-5,11);
      TS_ASSERT_EQUALS(v[-6], 21);
      TS_ASSERT_EQUALS(v[-5], 11);
      TS_ASSERT_EQUALS(v[-4], 21);
   }
   void testEqual() {
      BiVector<int> a(4,6,3);
      a.setAt(10) = 0; // increases storage but does not modify values.
      BiVector<int> b;
      b.setAt(0) = 0;
      b.setAt(4) = 3;
      b.setAt(5) = 3;
      b.setAt(6) = 3;
      TS_ASSERT(a == b);
   }
   void testNotEqual() {
      BiVector<int> a(4,6,3);
      BiVector<int> b(4,6,3);
      b.setAt(10) = 1;
      TS_ASSERT(a != b);
   }
   void testReadWrite() {
      BiVector<int> a(4,6,3);
      a.setAt(-3) = 0;
      BiVector<int> b(-3,4,2);
      b.setAt(10) = 0;
      FILE* f = tmpfile();
      oMagicStream os(f);
      a.write(os);
      b.write(os);
      os << "Extra line" << endl;
      os.close();
      rewind(f);
      iMagicStream is(f);
      BiVector<int> a_copy;
      BiVector<int> b_copy;
      a_copy.read(is);
      TS_ASSERT(a == a_copy);
      TS_ASSERT_EQUALS(a_copy.first(), 4);
      b_copy.read(is);
      TS_ASSERT(b == b_copy);
      TS_ASSERT_EQUALS(b_copy.last(), 4);
      string line;
      TS_ASSERT(getline(is, line));
      TS_ASSERT_EQUALS(line, "Extra line");
      is.close();
      fclose(f);
   }
   void testReadWriteBin() {
      BiVector<int> a(4,6,3);
      a.setAt(-3) = 0;
      BiVector<int> b(-3,4,2);
      b.setAt(10) = 0;
      FILE* f = tmpfile();
      oMagicStream os(f);
      a.writebin(os);
      b.writebin(os);
      os << "Extra line" << endl;
      os.close();
      rewind(f);
      iMagicStream is(f);
      BiVector<int> a_copy;
      BiVector<int> b_copy;
      a_copy.readbin(is);
      TS_ASSERT(a == a_copy);
      TS_ASSERT_EQUALS(a_copy.first(), a.first());
      b_copy.readbin(is);
      TS_ASSERT(b == b_copy);
      TS_ASSERT_EQUALS(b_copy.last(), b.last());
      string line;
      TS_ASSERT(getline(is, line));
      TS_ASSERT_EQUALS(line, "Extra line");
      is.close();
      fclose(f);
   }
   void testAdd() {
      BiVector<int> a(4,6,3);
      BiVector<int> b(2,4,2);
      a += b;
      TS_ASSERT_EQUALS(a[1], 0);
      TS_ASSERT_EQUALS(a[2], 2);
      TS_ASSERT_EQUALS(a[3], 2);
      TS_ASSERT_EQUALS(a[4], 5);
      TS_ASSERT_EQUALS(a[5], 3);
      TS_ASSERT_EQUALS(a[6], 3);
      TS_ASSERT_EQUALS(a[7], 0);
      BiVector<int> c;
      a += c;
      TS_ASSERT_EQUALS(a.first(), 2);
   }
   void testNonConstBracketOp() {
      BiVector<int> a(4,6,3);
      int i = a[10];
      TS_ASSERT_EQUALS(i, 0);
      TS_ASSERT_EQUALS(a[20], 0);
      TS_ASSERT_EQUALS(a.size(), 3u);
      a[9] = 4;
      TS_ASSERT_EQUALS(a[3], 0);
      TS_ASSERT_EQUALS(a[4], 3);
      TS_ASSERT_EQUALS(a[5], 3);
      TS_ASSERT_EQUALS(a[6], 3);
      TS_ASSERT_EQUALS(a[7], 0);
      TS_ASSERT_EQUALS(a[8], 0);
      TS_ASSERT_EQUALS(a[9], 4);
      TS_ASSERT_EQUALS(a[10], 0);
      TS_ASSERT_EQUALS(a.size(), 6u);
   }
}; // TestBiVector

} // Portage
