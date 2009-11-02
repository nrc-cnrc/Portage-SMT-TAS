/**
 * @author Eric Joanis
 * @file test_binio_regression.h Make sure our two competing binary io
 *                               structures produce the same results.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "binio.h"
#include <sstream>

using namespace Portage;

namespace Portage {

class TestBinIORegression : public CxxTest::TestSuite 
{
   struct NonSafeMovableT {
      Uint v;
      NonSafeMovableT() : v(0) {}
      NonSafeMovableT(Uint v) : v(v) {}
      static Uint reverse_bytes(Uint v) {
         Uint result;
         char* in_p = reinterpret_cast<char*>(&v);
         char* out_p = reinterpret_cast<char*>(&result);
         out_p[0] = in_p[3];
         out_p[1] = in_p[2];
         out_p[2] = in_p[1];
         out_p[3] = in_p[0];
         return result;
      }
      void writebin(std::ostream& out) const { BinIO::writebin(out, reverse_bytes(v)); }
      void readbin(std::istream& in) { BinIO::readbin(in, v); v = reverse_bytes(v); }
      bool operator==(NonSafeMovableT x) const { return v == x.v; }
      void operator=(Uint v) { this->v = v; }
   };

public:
   void testPrimitive() {
      ostringstream os1, os2;
      Uint x = 0x1234543;
      const char ref_c[] = "\x43\x45\x23\x01";
      const string ref(ref_c, sizeof(ref_c)-1);

      BinIO::writebin(os1, x);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, x);
      TS_ASSERT_EQUALS(os2.str(), ref);

      Uint x1(0), x2(0);
      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, x1);
      TS_ASSERT_EQUALS(x,x1);
      BinIO::readbin(is1, x2);
      TS_ASSERT_EQUALS(x,x2);
   }

   void testNonMovablePrimitive() {
      ostringstream os1, os2;
      NonSafeMovableT x(0x1234543);
      const char ref_c[] = "\x01\x23\x45\x43";
      const string ref(ref_c, sizeof(ref_c)-1);

      BinIO::writebin(os1, x);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, x);
      TS_ASSERT_EQUALS(os2.str(), ref);

      NonSafeMovableT x1(0), x2(0);
      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, x1);
      TS_ASSERT_EQUALS(x,x1);
      BinIO::readbin(is1, x2);
      TS_ASSERT_EQUALS(x,x2);
   }

   void testVector() {
      vector<Uint> v, v1, v2;
      v.push_back(0x1234);
      v.push_back(0x42);
      v.push_back(0x39273783);
      v.push_back(0x123124);
      const char ref_c[] = "\x04\x00\x00\x00\x34\x12\x00\x00\x42\x00\x00\x00\x83\x37\x27\x39\x24\x31\x12\x00";
      const string ref(ref_c, sizeof(ref_c)-1);

      ostringstream os1, os2;
      BinIO::writebin(os1,v);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, v);
      TS_ASSERT_EQUALS(os2.str(), ref);

      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, v1);
      TS_ASSERT_EQUALS(v,v1);
      BinIO::readbin(is1, v2);
      TS_ASSERT_EQUALS(v,v2);
   }

   void testNonMovableVector() {
      vector<NonSafeMovableT> v, v1, v2;
      v.push_back(NonSafeMovableT(0x1234));
      v.push_back(NonSafeMovableT(0x42));
      v.push_back(NonSafeMovableT(0x39273783));
      v.push_back(NonSafeMovableT(0x123124));
      const char ref_c[] = "\x04\x00\x00\x00\x00\x00\x12\x34\x00\x00\x00\x42\x39\x27\x37\x83\x00\x12\x31\x24";
      const string ref(ref_c, sizeof(ref_c)-1);

      ostringstream os1, os2;
      BinIO::writebin(os1, v);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, v);
      TS_ASSERT_EQUALS(os2.str(), ref);
      
      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, v1);
      TS_ASSERT_EQUALS(v,v1);
      BinIO::readbin(is1, v2);
      TS_ASSERT_EQUALS(v,v2);
   }

   void testPair() {
      pair<Uint, Uint> v(make_pair(0x12345678,0x87654321)), v1, v2;
      const char ref_c[] = "\x78\x56\x34\x12\x21\x43\x65\x87";
      const string ref(ref_c, sizeof(ref_c)-1);

      ostringstream os1, os2;
      BinIO::writebin(os1, v);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, v);
      TS_ASSERT_EQUALS(os2.str(), ref);
      
      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, v1);
      TS_ASSERT_EQUALS(v,v1);
      BinIO::readbin(is1, v2);
      TS_ASSERT_EQUALS(v,v2);
   }

   void testNonMovablePair() {
      pair<Uint, NonSafeMovableT> v(make_pair(0x12345678,0x87654321)), v1, v2;
      const char ref_c[] = "\x78\x56\x34\x12\x87\x65\x43\x21";
      const string ref(ref_c, sizeof(ref_c)-1);

      ostringstream os1, os2;
      BinIO::writebin(os1, v);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, v);
      TS_ASSERT_EQUALS(os2.str(), ref);
      
      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, v1);
      TS_ASSERT_EQUALS(v,v1);
      BinIO::readbin(is1, v2);
      TS_ASSERT_EQUALS(v,v2);
   }

   void testArray() {
      Uint v[4], v1[4], v2[4];
      v[0] = 0x1234;
      v[1] = 0x42;
      v[2] = 0x39273783;
      v[3] = 0x123124;
      const char ref_c[] = "\x34\x12\x00\x00\x42\x00\x00\x00\x83\x37\x27\x39\x24\x31\x12\x00";
      const string ref(ref_c, sizeof(ref_c)-1);

      ostringstream os1, os2;
      BinIO::writebin(os1, v, 4);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, v, 4);
      TS_ASSERT_EQUALS(os2.str(), ref);

      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, v1, 4);
      TS_ASSERT_SAME_DATA(v,v1,sizeof(v));
      BinIO::readbin(is1, v2, 4);
      TS_ASSERT_SAME_DATA(v,v2,sizeof(v));
   }

   void testNonMovableArray() {
      NonSafeMovableT v[4], v1[4], v2[4];
      v[0] = 0x1234;
      v[1] = 0x42;
      v[2] = 0x39273783;
      v[3] = 0x123124;
      const char ref_c[] = "\x00\x00\x12\x34\x00\x00\x00\x42\x39\x27\x37\x83\x00\x12\x31\x24";
      const string ref(ref_c, sizeof(ref_c)-1);

      ostringstream os1, os2;
      BinIO::writebin(os1, v, 4);
      TS_ASSERT_EQUALS(os1.str(), ref);
      BinIO::writebin(os2, v, 4);
      TS_ASSERT_EQUALS(os2.str(), ref);
      
      istringstream is1(os1.str()), is2(os2.str());
      BinIO::readbin(is2, v1, 4);
      TS_ASSERT_SAME_DATA(v,v1,sizeof(v));
      BinIO::readbin(is1, v2, 4);
      TS_ASSERT_SAME_DATA(v,v2,sizeof(v));
   }

}; // TestBinIORegression

} // Portage
