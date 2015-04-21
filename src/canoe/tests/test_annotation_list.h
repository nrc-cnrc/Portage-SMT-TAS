/**
 * @author Eric Joanis
 * @file test_annotation_list.h
 *
 * Unit test for annotation_list.h
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "annotation_list.h"
#include "vector_map.h"

using namespace Portage;

namespace Portage {

/// TestPrivateAnnotation class is used for unit testing - annotation initialized by client class, not from string
/// Private annotation types don't need to be visible to annotation_list.cc
class TestPrivateAnnotation : public PhrasePairAnnotation {
public:
   static const string name;
   static TestPrivateAnnotation* get(const AnnotationList& b) {
      return PhrasePairAnnotation::get<TestPrivateAnnotation>(name, b);
   }
   static TestPrivateAnnotation* getOrCreate(const AnnotationList& b, bool* is_new = NULL) {
      return PhrasePairAnnotation::getOrCreate<TestPrivateAnnotation>(name, b, is_new);
   }
   virtual void updateValue(const char* _value) {}
   virtual PhrasePairAnnotation* clone() const { return new TestPrivateAnnotation; }
};
const string TestPrivateAnnotation::name = "privateannotation";

class TestAnnotationList : public CxxTest::TestSuite {
public:
   void testStringAnnotation() {
      AnnotationList b;
      b.initAnnotation(TestStringAnnotation::name.c_str(), "bar");
      TestStringAnnotation* annotation = TestStringAnnotation::get(b);
      TS_ASSERT(annotation != NULL);
      if (annotation)
         TS_ASSERT_EQUALS(annotation->getValue(), "bar");
      b.initAnnotation(TestStringAnnotation::name.c_str(), "baz");
      TS_ASSERT_EQUALS(annotation->getValue(), "bar;baz");
      b.clear();
      TS_ASSERT(!TestStringAnnotation::get(b));
   }

   void testPrivateAnnotation() {
      AnnotationList b;
      TestPrivateAnnotation* annotation = TestPrivateAnnotation::get(b);
      TS_ASSERT(annotation == NULL);
      bool is_new;
      annotation = TestPrivateAnnotation::getOrCreate(b, &is_new);
      TS_ASSERT(annotation != NULL);
      TestPrivateAnnotation* annotation2 = TestPrivateAnnotation::get(b);
      TS_ASSERT_EQUALS(annotation, annotation2);
      TestPrivateAnnotation* annotation3 = TestPrivateAnnotation::getOrCreate(b, &is_new);
      TS_ASSERT_EQUALS(annotation, annotation3);
      TS_ASSERT(!is_new);
   }

   void testAnnotationMixing() {
      AnnotationList b;
      TestPrivateAnnotation* pc = TestPrivateAnnotation::getOrCreate(b);
      b.initAnnotation(TestStringAnnotation::name.c_str(), "another string annotation");

      TS_ASSERT_EQUALS(TestStringAnnotation::get(b)->getValue(), "another string annotation");
      TS_ASSERT_EQUALS(TestPrivateAnnotation::get(b), pc);
   }

   void testUnknownAnnotation() {
      AnnotationList b;
      b.initAnnotation("newannotationtype", "value");
      TS_ASSERT_EQUALS(UnknownAnnotation::get("newannotationtype", b)->getValue(), "value");

      TS_ASSERT(UnknownAnnotation::get("otherannotationtype", b) == NULL);
      b.initAnnotation("otherannotationtype", "othervalue");
      TS_ASSERT_EQUALS(UnknownAnnotation::get("otherannotationtype", b)->getValue(), "othervalue");
   }

   void testVectorMapAssumption() {
      // PhraseTable::AnnotatorRegistry relies on the fact that vector_map
      // keeps things in the order of first insertion, and yields them back
      // that way through iteration.  Make sure that's actually true!
      vector_map<string, Uint> map;
      map["foo"] = 0;
      map["bar"] = 1; // the order is not alphabetical
      map["foo"] = 0; // re-insertion does not change the position
      map["baz"] = 2;
      map["bar"] = 1;
      map["zing"] = 3;
      map["zoo"] = 4;
      map["baz"] = 2;
      map["foo"] = 0;
      Uint i = 0;
      for (vector_map<string, Uint>::iterator it(map.begin()), end(map.end());
           it != end; ++it, ++i)
         TS_ASSERT_EQUALS(it->second, i);
   }


}; // class TestAnnotationList

} // namespace Portage
