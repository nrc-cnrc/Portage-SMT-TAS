/**
 * @author Samuel Larkin
 * @file test_to_JSON.h
 *
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include <sstream>
#include "portage_defs.h"
#include "toJSON.h"

using namespace Portage;

namespace Portage {

struct Object {
   ostream& toJSON(ostream& out) const {
      return out << to_JSON(string("Object::toJSON()"));
   }
};


class TestToJSON : public CxxTest::TestSuite
{
public:
   void testKeyJSON() {
      TS_ASSERT_EQUALS(keyJSON("allo"), "\"allo\":");
      TS_ASSERT_EQUALS(keyJSON("al\"lo"), "\"al\\\"lo\":");
   }

   void testInt() {
      ostringstream oss;
      int a = 8;
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "8");
   }

   void testDouble() {
      ostringstream oss;
      double a = 8;
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "8");
   }

   void testBool() {
      ostringstream oss;
      bool a = true;
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "true");
   }

   void testString() {
      ostringstream oss;
      string a = "allo";
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "\"allo\"");
   }

   void testStringWithDoubleQuotes() {
      ostringstream oss;
      string a = "a\"llo";
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "\"a\\\"llo\"");
   }

   void testCharBuffer() {
      ostringstream oss;
      const char* a = "allo";
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "\"allo\"");
   }

   void testVectorInt() {
      ostringstream oss;
      vector<int> a;
      a.push_back(5);
      a.push_back(6);
      a.push_back(8);
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "[5,6,8]");
   }

   void testVectorString() {
      ostringstream oss;
      vector<string> a;
      a.push_back("Hello");
      a.push_back("World");
      a.push_back("!!!");
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "[\"Hello\",\"World\",\"!!!\"]");
   }

   void testVectorBool() {
      ostringstream oss;
      vector<bool> a;
      a.push_back(true);
      a.push_back(true);
      a.push_back(false);
      oss << to_JSON(a);
      TS_ASSERT_EQUALS(oss.str(), "[true,true,false]");
   }

   void testKeyValue() {
      ostringstream oss;
      string value = "value";
      oss << to_JSON("key", value);
      TS_ASSERT_EQUALS(oss.str(), "\"key\":\"value\"");
   }

   void testObject() {
      ostringstream oss;
      Object o;
      oss << to_JSON(o);
      TS_ASSERT_EQUALS(oss.str(), string("\"Object::toJSON()\""));
   }

   void testObject2() {
      ostringstream oss;
      Object o;
      oss << to_JSON("key", o);
      TS_ASSERT_EQUALS(oss.str(), string("\"key\":\"Object::toJSON()\""));
   }

   void testObjectPointer() {
      ostringstream oss;
      Object o;
      oss << to_JSON(&o);
      TS_ASSERT_EQUALS(oss.str(), string("\"Object::toJSON()\""));
   }

   void testVectorPointerWithNull() {
      Object obj;
      vector<Object*> v;
      v.push_back(NULL);
      v.push_back(&obj);

      ostringstream oss;
      oss << to_JSON(v);
      TS_ASSERT_EQUALS(oss.str(), string("[null,\"Object::toJSON()\"]"));
   }

}; // TestToJSON

} // Portage
