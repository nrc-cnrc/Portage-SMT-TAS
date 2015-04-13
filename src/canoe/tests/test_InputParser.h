/**
 * @author Samuel Larkin
 * @file test_InputParser.h  Test suite for InputParser.
 *
 * This class is to test functionalities of the InputParser class.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "inputparser.h"
#include "tmp_val.h"

using namespace Portage;

namespace Portage {

class TestInputParser : public CxxTest::TestSuite 
{
friend class InputParser;
public:
   void testNullCharacterForSkipSpaces() {
      // Initialize to something else than '\0' or else you get one more
      // NULL_CHARACTER from parer.skipSpaces.
      char c = ' ';
      const string source("\0\0\0\0\0allo", 9);
      // Let make sure the source is what we think it is.
      TS_ASSERT_EQUALS(source.size(), 9);

      // Temporarily mute the the function error used in the parse's skipSpaces function.
      using namespace Error_ns;
      tmp_val<ErrorCallback> tmp(Current::errorCallback, countErrorCallBack);

      // Get ready to parse the string with null characters.
      istringstream iss(source);
      InputParser parser(iss);
      parser.skipSpaces(c);

      // Validate what we came here to do, make sure we've ate all null
      // characters and that we are standing on the a of allo.
      TS_ASSERT_EQUALS(c, 'a');
      TS_ASSERT_EQUALS(parser.warning_counters[InputParser::NULL_CHARACTER], 5);
   }

   bool parse(const string& source, newSrcSentInfo& nss) {
      // Setup error() for counting
      using namespace Error_ns;
      tmp_val<ErrorCallback> tmp(Current::errorCallback, countErrorCallBack);
      ErrorCounts::clear();

      istringstream iss(source);
      InputParser parser(iss);

      return parser.readMarkedSent(nss);
   }

   void testParseWall() {
      newSrcSentInfo nss;
      TS_ASSERT(parse("a b <wall/> c d", nss));
      TS_ASSERT_EQUALS(nss.walls.size(), 1u);
      TS_ASSERT_EQUALS(nss.walls[0].pos, 2u);
      TS_ASSERT_EQUALS(nss.src_sent.size(), 4u);
      TS_ASSERT_EQUALS(nss.walls[0].name, "");

      TS_ASSERT(!parse("a b <wall/ > c d", nss));
      TS_ASSERT_EQUALS(Error_ns::ErrorCounts::Warn, 1u);

      TS_ASSERT(!parse("a b <wall> c d", nss));
      TS_ASSERT(Error_ns::ErrorCounts::Warn >= 1u);

      TS_ASSERT(parse("a b <wall name=\"id\"/> c d", nss));
      TS_ASSERT_EQUALS(Error_ns::ErrorCounts::Warn, 0u);

      TS_ASSERT(parse("a <wall name=\"foo\"/> b", nss));
      TS_ASSERT_EQUALS(nss.walls.size(), 1u);
      TS_ASSERT_EQUALS(nss.walls[0].pos, 1u);
      TS_ASSERT_EQUALS(nss.walls[0].name, "foo");

      TS_ASSERT(parse("a <wall name=\"\"/> b", nss));
      TS_ASSERT(!parse("a <wall name\"foo\"/> b", nss));
      TS_ASSERT(!parse("a <wall name=\"foo\"> b", nss));
      TS_ASSERT(!parse("a <wall nam=\"foo\"/> b", nss));
      TS_ASSERT(!parse("a <wall name=foo/> b", nss));
      TS_ASSERT(!parse("a <wall name=\"foo/> b", nss));
   }

   void testParseZone() {
      newSrcSentInfo nss;
      TS_ASSERT(parse("a b <zone> c </zone> d", nss));
      TS_ASSERT_EQUALS(nss.zones.size(), 1u);
      TS_ASSERT_EQUALS(nss.zones[0].range, Range(2,3));
      TS_ASSERT_EQUALS(nss.src_sent.size(), 4u);

      TS_ASSERT(parse("a b <zone> c </zone> <zone> d </zone>", nss));
      TS_ASSERT_EQUALS(nss.zones.size(), 2u);
      TS_ASSERT_EQUALS(nss.zones[0].range, Range(2,3));
      TS_ASSERT_EQUALS(nss.zones[1].range, Range(3,4));
      TS_ASSERT_EQUALS(nss.zones[0].name, "");
      TS_ASSERT_EQUALS(nss.zones[1].name, "");
      TS_ASSERT_EQUALS(nss.src_sent.size(), 4u);

      TS_ASSERT(!parse("a b <zone> c d", nss));
      TS_ASSERT_EQUALS(Error_ns::ErrorCounts::Warn, 1u);

      TS_ASSERT(!parse("a b </zone> c d", nss));
      TS_ASSERT_EQUALS(Error_ns::ErrorCounts::Warn, 1u);

      TS_ASSERT(parse("a b <zone name=\"id\"> c </zone> d", nss));
      TS_ASSERT_EQUALS(nss.zones.size(), 1u);
      TS_ASSERT_EQUALS(nss.zones[0].range, Range(2,3));
      TS_ASSERT_EQUALS(nss.zones[0].name, "id");

      TS_ASSERT(parse("a b <zone name=\"\"> c </zone> d", nss));
      TS_ASSERT(!parse("a b <zone name\"id\"> c </zone> d", nss));
      TS_ASSERT(!parse("a b <zone name=\"id\"/> c </zone> d", nss));
      TS_ASSERT(!parse("a b <zone name=id\"> c </zone> d", nss));
      TS_ASSERT(!parse("a b <zone nam=\"id\"> c </zone> d", nss));
      TS_ASSERT(!parse("a b <zone name=\"id\"> c </zone > d", nss));
      TS_ASSERT(!parse("a b <zone name=\"id\"> c </ zone> d", nss));
      TS_ASSERT(!parse("a b <zone name=\"id\"> c < /zone> d", nss));
   }

   void testNestedZones() {
      newSrcSentInfo nss;
      TS_ASSERT(parse("a b <zone> c <zone> d </zone> e </zone>", nss));
      TS_ASSERT_EQUALS(nss.zones.size(), 2u);

      TS_ASSERT(parse("a b <zone name=\"outer\"> c <zone name=\"inner\"> d e </zone> f </zone> g", nss));
      TS_ASSERT_EQUALS(nss.zones.size(), 2u);
      TS_ASSERT_EQUALS(nss.zones[0].range, Range(2,6));
      TS_ASSERT_EQUALS(nss.zones[1].range, Range(3,5));
      TS_ASSERT_EQUALS(nss.zones[0].name, "outer");
      TS_ASSERT_EQUALS(nss.zones[1].name, "inner");
   }

   void testLocalWalls() {
      newSrcSentInfo nss;
      TS_ASSERT(parse("a b <zone name=\"outer\"> c <localwall name=\"foo\"/> <zone name=\"inner\"> d <localwall/> e </zone> <localwall/> f </zone> g", nss));
      TS_ASSERT_EQUALS(nss.zones.size(), 2u);
      TS_ASSERT_EQUALS(nss.zones[0].range, Range(2,6));
      TS_ASSERT_EQUALS(nss.zones[1].range, Range(3,5));
      TS_ASSERT_EQUALS(nss.zones[0].name, "outer");
      TS_ASSERT_EQUALS(nss.zones[1].name, "inner");

      TS_ASSERT_EQUALS(nss.local_walls.size(), 3u);
      TS_ASSERT_EQUALS(nss.local_walls[0].pos, 3u);
      TS_ASSERT_EQUALS(nss.local_walls[0].zone, Range(2,6));
      TS_ASSERT_EQUALS(nss.local_walls[0].name, "foo");
      TS_ASSERT_EQUALS(nss.local_walls[1].pos, 4u);
      TS_ASSERT_EQUALS(nss.local_walls[1].zone, Range(3,5));
      TS_ASSERT_EQUALS(nss.local_walls[1].name, "");
      TS_ASSERT_EQUALS(nss.local_walls[2].pos, 5u);
      TS_ASSERT_EQUALS(nss.local_walls[2].zone, Range(2,6));
      TS_ASSERT_EQUALS(nss.local_walls[2].name, "");
   }
}; // TestInputParser

} // Portage
