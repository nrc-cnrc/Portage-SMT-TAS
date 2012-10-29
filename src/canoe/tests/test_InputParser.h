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
      using namespace Portage::Error_ns;
      tmp_val<Error_ns::ErrorCallback> tmp(Current::errorCallback, nullErrorCallBack);

      // Get ready to parse the string with null characters.
      istringstream iss(source);
      InputParser parser(iss);
      parser.skipSpaces(c);

      // Validate what we came here to do, make sure we've ate all null
      // characters and that we are standing on the a of allo.
      TS_ASSERT_EQUALS(c, 'a');
      TS_ASSERT_EQUALS(parser.warning_counters[InputParser::NULL_CHARACTER], 5);
   }
}; // TestInputParser

} // Portage
