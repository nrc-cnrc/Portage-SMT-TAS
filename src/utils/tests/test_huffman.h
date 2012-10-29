/**
 * @author Eric Joanis
 * @file test_huffman.h  Test Huffman coding and decoding class
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "huffman.h"
#include "tmp_val.h"
#include <sstream>

using namespace Portage;

namespace Portage {

class TestHuffman : public CxxTest::TestSuite 
{
   vector<pair<char,Uint> > values;
   HuffmanCoder<char> h;

public:
   void setUp() {
      // This test case follows the example in CLR (Algorithms) page 337: 17.3
      // Huffman codes.
      values.push_back(make_pair('a',45));
      values.push_back(make_pair('b',13));
      values.push_back(make_pair('c',12));
      values.push_back(make_pair('d',16));
      values.push_back(make_pair('e',9));
      values.push_back(make_pair('f',5));
      h.buildCoder(values.begin(), values.end());
   }

   void tearDown() {
      values.clear();
      h.clear();
   }

   void testTotalCost() {
      //h.dumpTree(cerr);
      TS_ASSERT_EQUALS(h.totalCost(), 1*45 + 3*(13+12+16) + 4*(9+5)); // 224
   }

   void testIndividualValues() {
      // This test case follows the example in CLR (Algorithms) page 337: 17.3
      // Huffman codes.
      //h.dumpTree(cout);
      char ref[7] = "\x00\xA0\x80\xE0\xD0\xC0";
      Uchar offsets[6] = {1,3,3,3,4,4};
      for (char c = 'a'; c < 'g'; ++c) {
         string encoded;
         Uchar offset = h.appendEncoded(encoded, 0, c);
         TS_ASSERT_EQUALS(encoded.size(),1u);
         TS_ASSERT_EQUALS(encoded[0], ref[c-'a']);
         TS_ASSERT_EQUALS(offset, offsets[c-'a']);
      }
   }

   string fillValueSequence(HuffmanCoder<char>& h) {
      string packed;
      Uchar offset = 0;
      for (char c = 'a'; c < 'g'; ++c)
         offset = h.appendEncoded(packed, offset, c);
      TS_ASSERT_EQUALS(offset, 2u);
      return packed;
   }

   void testValueSequence() {
      string packed = fillValueSequence(h);
      TS_ASSERT_EQUALS(packed.size(), 3u);
      string packed_ref("\x59\xF7");
      packed_ref.push_back(0); // a \x00 character
      TS_ASSERT_EQUALS(packed, packed_ref);

      pair<const char*, Uchar> z = make_pair(packed.c_str(), 0);
      for (Uint i = 0; i < 6; ++i) {
         char value;
         z = h.readEncoded(z.first, z.second, value);
         TS_ASSERT_EQUALS(value, values[i].first);
      }
      TS_ASSERT_EQUALS(z.first, packed.c_str()+2);
      TS_ASSERT_EQUALS(z.second, 2u);
   }

   void testReadWriteCoder() {
      ostringstream oss;
      h.writeCoder(oss);
      istringstream iss(oss.str());
      HuffmanCoder<char> h2(iss, "stream");
      string packed = fillValueSequence(h);
      string packed2 = fillValueSequence(h2);
      TS_ASSERT_EQUALS(packed,packed2);

      {
         using namespace Portage::Error_ns;
         tmp_val<ErrorCallback> tmp(Current::errorCallback, countErrorCallBack);
         Error_ns::ErrorCounts::Fatal = 0;
         istringstream iss2(oss.str());
         HuffmanCoder<Uint> h3(iss2, "bad stream");
         TS_ASSERT_EQUALS(ErrorCounts::Fatal, 1u);
      }
   }

   void testDecodeWithTPCoder() {
      ostringstream oss;
      h.writeTPCoder(oss);
      string s = oss.str();
      const char* pos = s.c_str();
      HuffmanCoder<char> tp_h;
      tp_h.openTPCoder(pos, "in-memory-stream");

      string packed = fillValueSequence(h);
      pair<const char*, Uchar> z = make_pair(packed.c_str(), 0);
      for (Uint i = 0; i < 6; ++i) {
         char value;
         z = tp_h.readEncoded(z.first, z.second, value);
         TS_ASSERT_EQUALS(value, values[i].first);
      }
      TS_ASSERT_EQUALS(z.first, packed.c_str()+2);
      TS_ASSERT_EQUALS(z.second, 2u);
   }

   void testEncodeWithTPCoder() {
      ostringstream oss;
      h.writeTPCoder(oss);
      string s = oss.str();
      const char* pos = s.c_str();
      HuffmanCoder<char> tp_h;
      tp_h.openTPCoder(pos, "in-memory-stream");

      string packed = fillValueSequence(h);
      string packed_tp = fillValueSequence(tp_h);
      TS_ASSERT_EQUALS(packed, packed_tp);
   }

}; // TestYourClass

} // Portage
