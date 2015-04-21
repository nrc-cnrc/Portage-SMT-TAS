/**
 * @author Eric Joanis
 * @file test_inheritance.h  Check that a ref to a *ptr still gets to virtual
 *                           methods as expected.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>

namespace Portage {

class TestInheritance : public CxxTest::TestSuite 
{
   struct Base {
      virtual ~Base() {}
      virtual int foo() { return 0; }
   };
   struct Sub : public Base {
      virtual int foo() { return 1; }
   };
public:
   void testInheritanceRef() {
      Base* p = new Sub;
      Base& r(*p);
      TS_ASSERT_EQUALS(r.foo(), 1);
      if (p) delete p;
   }
}; // testInheritance


} // Portage

using namespace Portage;

