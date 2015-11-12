/**
 * @author Eric Joanis
 * @file test_str_utils.h  Test suite for the utilities in str_utils.{h,cc}
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "str_utils.h"

using namespace Portage;

namespace Portage {

class TestStrUtils : public CxxTest::TestSuite
{
public:
   void testTrimChar() {
      char test_str[] = " \t asd qwe\t asdf \tasdf \t ";
      char test_str_answer[] = "asd qwe\t asdf \tasdf";
      TS_ASSERT(strcmp(trim(test_str), test_str_answer) == 0);
      char test_str2[] = "";
      TS_ASSERT(strcmp(trim(test_str2), "") == 0);
      char test_str3[] = "asdf";
      TS_ASSERT(strcmp(trim(test_str3), "asdf") == 0);
   }

   void testJoinAppend() {
      vector<string> v;
      v.push_back("a");
      v.push_back("b");
      v.push_back("c");
      string buffer;
      TS_ASSERT_EQUALS(join_append(v, buffer), "a b c");
      TS_ASSERT_EQUALS(buffer, "a b c");
      buffer.clear();
      TS_ASSERT_EQUALS(join_append(v.begin(), v.begin(), buffer), "");
      TS_ASSERT_EQUALS(join_append(v.begin(), v.begin()+1, buffer), "a");
      buffer.clear();
      TS_ASSERT_EQUALS(join_append(v, buffer, "__"), "a__b__c");
   }

   void testJoiner() {
      vector<string> v;
      v.push_back("a");
      v.push_back("b");
      v.push_back("c");
      TS_ASSERT_EQUALS(join(v), "a b c");
      TS_ASSERT_EQUALS(join(v.begin(), v.end()), "a b c");
      TS_ASSERT_EQUALS(join(v.begin(), v.begin()), "");
      TS_ASSERT_EQUALS(join(v.begin(), v.begin()+1), "a");
      TS_ASSERT_EQUALS(join(v.begin(), v.end(), "+"), "a+b+c");

      vector<float> f;
      f.push_back(1.0/7);
      f.push_back(2.0/7);
      f.push_back(3.0/7);
      f.push_back(4.0/7);
      TS_ASSERT_EQUALS(join(f), "0.14285715 0.2857143 0.42857143 0.5714286");
      TS_ASSERT_EQUALS(join(f, " ", 2), "0.14 0.29 0.43 0.57");
      TS_ASSERT_EQUALS(join(f, "_", 4), "0.1429_0.2857_0.4286_0.5714");

      ostringstream oss;
      oss << join(f, "_-_", 5);
      TS_ASSERT_EQUALS(oss.str(), "0.14286_-_0.28571_-_0.42857_-_0.57143");
   }


   void testJoinIterator() {
      Uint a[] = {1, 2, 3};
      TS_ASSERT_EQUALS(join(a, a+3), "1 2 3");
      TS_ASSERT_EQUALS(join(a, a), "");
      TS_ASSERT_EQUALS(join(a, a+1), "1");
   }

   void testJoinPlus() {
      Uint a[] = {1, 2, 3};
      TS_ASSERT_EQUALS(join(a, a+3) + " foo", "1 2 3 foo");
      TS_ASSERT_EQUALS("foo " + join(a, a+3), "foo 1 2 3");
      TS_ASSERT_EQUALS(join(a, a+3), string("1 2 3"));
      TS_ASSERT_EQUALS(string("1 2 3"), join(a, a+3));
   }

   void testSplitIntoArray() {
      const char* s = "1 -0.4 5.3e-21";
      float toks[3];

      const Uint numTok = split(s, toks, convT<float>);
      TS_ASSERT_EQUALS(numTok, 3);
      TS_ASSERT_DELTA(toks[0], 1, 1e-35);
      TS_ASSERT_DELTA(toks[1], -0.4, 1e-5);
      TS_ASSERT_DELTA(toks[2], 5.3e-21, 1e-25);
   }

   void testSplitString() {
      string s("a ||| b ||| c");
      vector<string> toks;
      string sep(" ||| ");

      splitString(s, toks, sep);
      TS_ASSERT_EQUALS(toks.size(), 3);
      TS_ASSERT_EQUALS(toks[0], "a");
      TS_ASSERT_EQUALS(toks[1], "b");
      TS_ASSERT_EQUALS(toks[2], "c");

      splitStringZ(s, toks, sep);
      TS_ASSERT_EQUALS(toks.size(), 3);
      TS_ASSERT_EQUALS(toks[0], "a");
      TS_ASSERT_EQUALS(toks[1], "b");
      TS_ASSERT_EQUALS(toks[2], "c");
   }

   void testEmptySplit() {
      string empty("");
      vector<string> toks;

      split(empty, toks, ";");
      TS_ASSERT_EQUALS(toks.size(), 0);
   }

   void testSplitMaxToks() {
      string input = "asdf qwer tyui ghkl";
      vector<string> tokens;
      TS_ASSERT_EQUALS(splitZ(input, tokens, " ", 5), 4u);
      TS_ASSERT_EQUALS(tokens[3], "ghkl");
      TS_ASSERT_EQUALS(splitZ(input, tokens, " ", 4), 4u);
      TS_ASSERT_EQUALS(tokens[3], "ghkl");
      TS_ASSERT_EQUALS(splitZ(input, tokens, " ", 3), 3u);
      TS_ASSERT_EQUALS(tokens[2], "tyui ghkl");

      input = "asdf qwer tyui   ghkl   ";
      TS_ASSERT_EQUALS(splitZ(input, tokens, " ", 5), 4u);
      TS_ASSERT_EQUALS(tokens[3], "ghkl");
      TS_ASSERT_EQUALS(splitZ(input, tokens, " ", 4), 4u);
      TS_ASSERT_EQUALS(tokens[3], "ghkl   ");
      TS_ASSERT_EQUALS(splitZ(input, tokens, " ", 3), 3u);
      TS_ASSERT_EQUALS(tokens[2], "tyui   ghkl   ");
   }

   void testDestructiveSplit() {
      const char input[] = "asdf qwer tyui ghkl   ";
      char buffer[30] = "";
      char* tokens[10];
      strcpy(buffer, input);
      TS_ASSERT_EQUALS(destructive_split(buffer, tokens, 5), 4u);
      TS_ASSERT_EQUALS(string(tokens[3]), "ghkl");
      strcpy(buffer, input);
      TS_ASSERT_EQUALS(destructive_split(buffer, tokens, 4), 4u);
      TS_ASSERT_EQUALS(string(tokens[3]), "ghkl   ");
      strcpy(buffer, input);
      TS_ASSERT_EQUALS(destructive_split(buffer, tokens, 3), 3u);
      TS_ASSERT_EQUALS(string(tokens[2]), "tyui ghkl   ");

      const char input2[] = "asdf qwer  tyui  \t \tghkl";
      strcpy(buffer, input2);
      TS_ASSERT_EQUALS(destructive_split(buffer, tokens, 4, " \t"), 4u);
      TS_ASSERT_EQUALS(string(tokens[3]), "ghkl");
      strcpy(buffer, input2);
      TS_ASSERT_EQUALS(destructive_split(buffer, tokens, 3, " \t"), 3u);
      TS_ASSERT_EQUALS(string(tokens[2]), "tyui  \t \tghkl");
   }

   void testConvUint() {
      Uint x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000u);
      TS_ASSERT(conv("4000000000", x));
      TS_ASSERT_EQUALS(x, 4000000000u);
      TS_ASSERT(!conv("5000000000", x));
      TS_ASSERT(!conv("-1", x));
      TS_ASSERT(!conv("-2000000000", x));
      TS_ASSERT(!conv("-3000000000", x));
      TS_ASSERT(!conv("-5000000000", x));
   }

   void testConvInt() {
      int x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000);
      TS_ASSERT(!conv("4000000000", x));
      TS_ASSERT(!conv("5000000000", x));
      TS_ASSERT(conv("-1", x));
      TS_ASSERT_EQUALS(x, -1);
      TS_ASSERT(conv("-2000000000", x));
      TS_ASSERT_EQUALS(x, -2000000000);
      TS_ASSERT(!conv("-3000000000", x));
   }

   void testConvUint64() {
      Uint64 x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000u);
      TS_ASSERT(conv("4000000000", x));
      TS_ASSERT_EQUALS(x, 4000000000u);
      TS_ASSERT(conv("5000000000", x));
      TS_ASSERT_EQUALS(x, 5000000000ull);
      TS_ASSERT(conv("9223372036854775807", x)); // LLONG_MAX
      TS_ASSERT_EQUALS(x, 9223372036854775807ull);
      TS_ASSERT(conv("18446744073709551615", x)); // ULLONG_MAX
      TS_ASSERT_EQUALS(x, 18446744073709551615ull);
      TS_ASSERT(!conv("20000000000000000000", x));
      TS_ASSERT(!conv("-1", x));
      TS_ASSERT(!conv("-5000000000", x));
      TS_ASSERT(!conv("-9000000000000000000", x));
      TS_ASSERT(!conv("-15000000000000000000", x));
      TS_ASSERT(!conv("-20000000000000000000", x));
   }

   void testConvInt64() {
      Int64 x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000);
      TS_ASSERT(conv("4000000000", x));
      TS_ASSERT_EQUALS(x, 4000000000ll);
      TS_ASSERT(conv("5000000000", x));
      TS_ASSERT_EQUALS(x, 5000000000ll);
      TS_ASSERT(conv("9223372036854775807", x)); // LLONG_MAX
      TS_ASSERT_EQUALS(x, 9223372036854775807ll);
      TS_ASSERT(!conv("18446744073709551615", x)); // ULLONG_MAX
      TS_ASSERT(!conv("20000000000000000000", x));
      TS_ASSERT(conv("-1", x));
      TS_ASSERT_EQUALS(x, -1);
      TS_ASSERT(conv("-5000000000", x));
      TS_ASSERT_EQUALS(x, -5000000000ll);
      TS_ASSERT(conv("-9000000000000000000", x));
      TS_ASSERT_EQUALS(x, -9000000000000000000ll);
      TS_ASSERT(!conv("-15000000000000000000", x));
      TS_ASSERT(!conv("-20000000000000000000", x));
   }

   void testReplaceAll() {
      string a = "AbAbAA";
      replaceAll(a, "A", "Z");
      TS_ASSERT_EQUALS(a, "ZbZbZZ");

      replaceAll(a, "Zb", "zB");
      TS_ASSERT_EQUALS(a, "zBzBZZ");
   }


   struct testConverter {
      bool operator()(const char* const src, Uint& dest) const {
         convT<Uint>(src, dest);
         dest += 10;
         return true;
      }
   };

   void testSplitWithConverter() {
      Uint dest[6];
      bzero(dest, 6*sizeof(Uint));
      testConverter converter;
      TS_ASSERT_EQUALS(split("0 1 2 43", dest, converter), 4);
      TS_ASSERT_EQUALS(dest[0], 10);
      TS_ASSERT_EQUALS(dest[1], 11);
      TS_ASSERT_EQUALS(dest[2], 12);
      TS_ASSERT_EQUALS(dest[3], 53);
      TS_ASSERT_EQUALS(dest[4], 0);
      TS_ASSERT_EQUALS(dest[5], 0);
   }

}; // TestStrUtils

} // Portage
