/**
 * @author Darlene Stewart
 * @file test_tpt_bitcoder.h
 * @brief Test suite for ugdiss::BitCoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "tpt_bitcoder.h"

using namespace Portage;

namespace Portage {

using namespace ugdiss;

class TestTptBitcoder : public CxxTest::TestSuite
{
private:
   BitCoder<uint32_t> bc;

public:
   void setUp() {
      // block[0] = 3, threshold[0] =    7, modMask[0] =  8 (00001000)
      // block[1] = 5, threshold[1] =  255, modMask[1] = 32 (00100000)
      // block[2] = 4, threshold[2] = 4095, modMask[2] = 16 (00010000)
      uint32_t blk_sizes[] = {3,5,4};
      vector<uint32_t> blocks(blk_sizes,blk_sizes+3);
      bc.setBlocks(blocks);
   }
   void tearDown() {}

   void test_writeNumber() {
      string dest;
      size_t offset;

      // 0: 0, needs 1 bit
      // so use 3 bits, plus stop bit (1),
      // therefore write 4 bits (1+000)
      // result: 10000000 (\x80), offset=4
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)0);
      TS_ASSERT_EQUALS(dest.size(), 1);
      TS_ASSERT_EQUALS(offset, 4);
      TS_ASSERT_EQUALS(dest[0], '\x80');

      // 5: 101, needs 3 bits
      // so use 3 bits, plus stop bit (1),
      // therefore write 4 bits (1+101)
      // result: 11010000 (\xD0), offset=4
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)5);
      TS_ASSERT_EQUALS(dest.size(), 1);
      TS_ASSERT_EQUALS(offset, 4);
      TS_ASSERT_EQUALS(dest[0], '\xD0');

      // 7: 111, needs 3 bits
      // so use 3 bits, plus stop bit (1),
      // therefore write 4 bits (1+111)
      // result: 11110000 (\xF0), offset=4
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)7);
      TS_ASSERT_EQUALS(dest.size(), 1);
      TS_ASSERT_EQUALS(offset, 4);
      TS_ASSERT_EQUALS(dest[0], '\xF0');

      // 8: 1000, needs 4 bits
      // so use 3+5 bits (00001000), plus stop bits for each (0+1)
      // therefore write 4 bits (0+000), 6 bits (1+01000)
      // result: 00001010 00000000 (\x0A \x00), offset=2
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)8);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 2);
      TS_ASSERT_EQUALS(dest[0], '\x0A');
      TS_ASSERT_EQUALS(dest[1], '\x00');

      // 19: 10011, needs 5 bits
      // so use 3+5 bits (00010011), plus stop bits for each (0+1)
      // therefore write 4 bits (0+000), 6 bits (1+10011)
      // result: 00001100 11000000 (\x0C \xC0), offset=2
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)19);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 2);
      TS_ASSERT_EQUALS(dest[0], '\x0C');
      TS_ASSERT_EQUALS(dest[1], '\xC0');

      // 107: 1101011, needs 7 bits
      // so use 3+5 bits (01101011), plus stop bits for each (0+1)
      // therefore write 4 bits (0+011), 6 bits (1+01011)
      // result: 00111010 11000000 (\x3A \xC0), offset=2
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)107);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 2);
      TS_ASSERT_EQUALS(dest[0], '\x3A');
      TS_ASSERT_EQUALS(dest[1], '\xC0');

      // 255: 11111111, needs 8 bits
      // so use 3+5 bits (11111111), plus stop bits for each (0+1)
      // therefore write 4 bits (0+111), 6 bits (1+11111)
      // result: 01111111 11000000 (\x7F \xC0), offset=2
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)255);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 2);
      TS_ASSERT_EQUALS(dest[0], '\x7F');
      TS_ASSERT_EQUALS(dest[1], '\xC0');

      // 256: 100000000, needs 9 bits
      // so use 3+5+4 bits (00010000000), plus stop bits for first 2 (0+0)
      // therefore write 4 bits (0+000), 6 bits (0+10000), 4 bits (0000)
      // result: 00000100 00000000 (\x04 \x00), offset=6
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)256);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 6);
      TS_ASSERT_EQUALS(dest[0], '\x04');
      TS_ASSERT_EQUALS(dest[1], '\x00');

      // 726: 1011010110, needs 10 bits
      // so use 3+5+4 bits (001011010110), plus stop bits for first 2 (0+0)
      // therefore write 4 bits (0+001), 6 bits (0+01101), 4 bits (0110)
      // result: 00010011 01011000 (\x13 \x58), offset=6
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)726);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 6);
      TS_ASSERT_EQUALS(dest[0], '\x13');
      TS_ASSERT_EQUALS(dest[1], '\x58');

      // 4095: 111111111111, needs 12 bits
      // so use 3+5+4 bits (111111111111), plus stop bits for first 2 (0+0)
      // therefore write 4 bits (0+111), 6 bits (0+11111), 4 bits (1111)
      // result: 01110111 11111100 (\x77 \xFC), offset=6
      dest.clear();
      offset = bc.writeNumber(dest, 0, (uint32_t)4095);
      TS_ASSERT_EQUALS(dest.size(), 2);
      TS_ASSERT_EQUALS(offset, 6);
      TS_ASSERT_EQUALS(dest[0], '\x77');
      TS_ASSERT_EQUALS(dest[1], '\xFC');
   }

   void test_readNumber() {
      char *src;
      uint32_t value;
      pair<char const*, unsigned char> res;

      // src: 10000000 (\x80)
      // uses 4 bits: 1+000 (4 bits of first byte)
      // value: 0: 0 (1 bit)
      // result: &src[0], 4
      src = (char *)"\x80\x00\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 0);
      TS_ASSERT_EQUALS(res.first, src);
      TS_ASSERT_EQUALS(res.second, 4);

      // src: 11010000 (\xD0)
      // uses 4 bits: 1+101 (4 bits of first byte)
      // value: 5: 101 (3 bits)
      // result: &src[0], 4
      src = (char *)"\xD0\x00\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 5);
      TS_ASSERT_EQUALS(res.first, src);
      TS_ASSERT_EQUALS(res.second, 4);

      // src: 11110000 (\xF0)
      // uses 4 bits: 1+111 (4 bits of first byte)
      // value: 7: 111 (3 bits)
      // result: &src[0], 4
      src = (char *)"\xF0\x00\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 7);
      TS_ASSERT_EQUALS(res.first, src);
      TS_ASSERT_EQUALS(res.second, 4);

      // pattern produced by broken writeNumber:
      // src: 00001001 11000000 (\x09 \xC0)
      // uses 4 bits: 0+000 plus 6 bits: 1+00111 (1 full byte + 2 bits of 2nd byte)
      // value: 7: 111 (3 bits)
      // result: &src[1], 2
      src = (char *)"\x09\xC0\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 7);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 2);

      // src: 00001010 00000000 (\x0A \x00)
      // uses 4 bits: 0+000 plus 6 bits: 1+01000 (1 full byte + 2 bits of 2nd byte)
      // value: 8: 1000 (4 bits)
      // result: &src[1], 2
      src = (char *)"\x0A\x00\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 8);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 2);

      // src: 00001100 11000000 (\x0C \xC0)
      // uses 4 bits: 0+000 plus 6 bits: 1+10011 (1 full byte + 2 bits of 2nd byte)
      // value: 8: 10011 (5 bits)
      // result: &src[1], 2
      src = (char *)"\x0C\xC0\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 19);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 2);

      // src: 00111010 11000000 (\x3A \xC0)
      // uses 4 bits: 0+011 plus 6 bits: 1+01011 (1 full byte + 2 bits of 2nd byte)
      // value: 107: 1101011 (7 bits)
      // result: &src[1], 2
      src = (char *)"\x3A\xC0\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 107);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 2);

      // src: 01111111 11000000 (\x7F \xC0)
      // uses 4 bits: 0+111 plus 6 bits: 1+11111 (1 full byte + 2 bits of 2nd byte)
      // value: 255: 11111111 (8 bits)
      // result: &src[1], 2
      src = (char *)"\x7F\xC0\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 255);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 2);

      // pattern produced by broken writeNumber:
      // src: 00000011 11111100 (\x03 \xFC)
      // uses 4 bits: 0+000 plus 6 bits: 0+01111 plus 4 bits: 1111  (1 full byte + 6 bits of 2nd byte)
      // value: 255: 11111111 (8 bits)
      // result: &src[1], 6
      src = (char *)"\x03\xFC\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 255);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 6);

      // src: 00000100 00000000 (\x04 \x00)
      // uses 4 bits: 0+000 plus 6 bits: 0+10000 plus 4 bits: 0000  (1 full byte + 6 bits of 2nd byte)
      // value: 256: 100000000 (9 bits)
      // result: &src[1], 6
      src = (char *)"\x04\x00\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 256);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 6);

      // src: 00010011 01011000 (\x13 \x58)
      // uses 4 bits: 0+001 plus 6 bits: 0+01101 plus 4 bits: 0110  (1 full byte + 6 bits of 2nd byte)
      // value: 726: 1011010110 (10 bits)
      // result: &src[1], 6
      src = (char *)"\x13\x58\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 726);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 6);

      // src: 01110111 11111100 (\x77 \xFC)
      // uses 4 bits: 0+111 plus 6 bits: 0+11111 plus 4 bits: 1111  (1 full byte + 6 bits of 2nd byte)
      // value: 4095: 111111111111 (12 bits)
      // result: &src[1], 6
      src = (char *)"\x77\xFC\x00\x00";
      res = bc.readNumber(src, 0, value);
      TS_ASSERT_EQUALS(value, 4095);
      TS_ASSERT_EQUALS(res.first, src+1);
      TS_ASSERT_EQUALS(res.second, 6);
   }
}; // TestTptBitcoder

} // Portage
