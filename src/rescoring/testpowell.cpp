/**
 * @author Aaron Tikuisis
 * @file testpowell.cpp  Program to test the Powell's algorithm.
 * $Id$
 *
 * K-Best Rescoring Module
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#include <powell.h>
#include <rescoring_general.h>
#include <bleu.h>
#include <boostDef.h>
#include <vector>


using namespace Portage;
using namespace std;


namespace Portage {

/// Program test powell's namespace.
/// Prevents pollution of the global namespace
namespace TestPowell {

/// Quick test class for Powell's algorithm
class TestScore
{
   public:
      int  val;
      /// Default constructor.
      TestScore(const int _val = 0) { val = _val; }
      /// Get score value
      /// @return Returns the score value.
      double score() const { return val; }
      /// Prints score's value for debug mode
      void debugoutput() const { RSC_DEBUG(val); }
      /// Prints score value to out
      /// @param out  output stream to print score's value
      void output(ostream &out) { out << val << endl; }
};

/**
 * Substraction operator for TestScore
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns s1 - s2
 */
TestScore operator-(const TestScore &s1, const TestScore &s2)
{
   TestScore result;
   result.val = s1.val - s2.val;
   return result;
}

/**
 * Addition operator for TestScore
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns s1 + s2
 */
TestScore operator+(const TestScore &s1, const TestScore &s2)
{
   TestScore result;
   result.val = s1.val + s2.val;
   return result;
}
} // ends namespace TestPowell
} // ends namespace Portage
using namespace Portage::TestPowell;



int main()
{
   typedef unsigned int Uint;
   //const Uint S(2);
   const Uint K(3);
   const Uint M(3);

   /*
   H[0] = [ 1  -1   0 ]
          [ 3   1   0 ]
          [ 3   0   1 ]
   H[1] = [ 1  -1   0 ]
          [ 5   1   0 ]
          [ 0   0  -2 ]
   */

   vector<uMatrix> vH;
   vH.push_back(uMatrix(K, M));
   vH.push_back(uMatrix(K, M));
   vH[0](0,0) = 1;
   vH[0](0,1) = -1;
   vH[0](0,2) = 0;
   vH[0](1,0) = 3;
   vH[0](1,1) = 1;
   vH[0](1,2) = 0;
   vH[0](2,0) = 3;
   vH[0](2,1) = 0;
   vH[0](2,2) = 1;
   vH[1](0,0) = 1;
   vH[1](0,1) = -1;
   vH[1](0,2) = 0;
   vH[1](1,0) = 5;
   vH[1](1,1) = 1;
   vH[1](1,2) = 0;
   vH[1](2,0) = 0;
   vH[1](2,1) = 0;
   vH[1](2,2) = -2;

   /*
   p = (1, 1, 1)^T
   dirs = identity
   */
   uVector p(M);
   p(0) = 1;
   p(1) = 1;
   p(2) = 1;
   uMatrix dirs(M, M);
   dirs = uIdentityMatrix(M);

   vector< vector<TestScore> > bleu;
   bleu.push_back(vector<TestScore>());
   bleu.back().push_back(1);
   bleu.back().push_back(2);
   bleu.back().push_back(3);
   bleu.push_back(vector<TestScore>());
   bleu.back().push_back(1);
   bleu.back().push_back(3);
   bleu.back().push_back(2);

   int iter(0);
   double score(0.0f);
   powell(p, dirs, 0.01, iter, score, vH, bleu);

   cout << p << endl;
}
