/**
 * @author Aaron Tikuisis
 * @file testlinemax.cc  Program to test the linemax algorithm.
 * $Id$
 *
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <linemax.h>
#include <rescoring_general.h>
#include <linearmetric.h>
#include <boostDef.h>
#include <vector>

using namespace Portage;
using namespace std;

int main(int argc, char* argv[])
{
	 if (argc > 1) {
	    cerr << "Program to test the linemax algorithm." << endl;
		 exit(1);
	 }

   typedef unsigned int Uint;

   //const Uint S(2);
   const Uint K(2);
   const Uint M(2);

   /*
   H[0] = [ 1  0 ]
          [ 0  1 ]
   H[1] = [ 1  0 ]
          [ 3 -1 ]
   */
   vector<uMatrix> vH;
   vH.push_back(uMatrix(K, M));
   vH.back()(0,0) = 1;
   vH.back()(0,1) = 0;
   vH.back()(1,0) = 0;
   vH.back()(1,1) = 1;
   vH.push_back(uMatrix(K, M));
   vH.back()(0,0) = 1;
   vH.back()(0,1) = 0;
   vH.back()(1,0) = 3;
   vH.back()(1,1) = -1;

   /*
   p = (1, 0)^T
   dir = (0, 1)^T
   */
   uVector p(M);
   p(0) = 1;
   p(1) = 0;
   uVector dir(M);
   dir(0) = 0;
   dir(1) = 1;

   vector< vector<LinearMetric> > translationScore;
   translationScore.push_back(vector<LinearMetric>());
   translationScore.back().push_back(1);
   translationScore.back().push_back(2);
   translationScore.push_back(vector<LinearMetric>());
   translationScore.back().push_back(1);
   translationScore.back().push_back(2);

   LineMax<LinearMetric> linemax(vH, translationScore);
   linemax(p, dir);

   cout << dir << endl;
}
