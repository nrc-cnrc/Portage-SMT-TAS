/**
 * @author Aaron Tikuisis
 * @file testlinemax.cpp  Program to test the linemax algorithm.
 * $Id$
 *
 * K-Best Rescoring Module
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#include <linemax.h>
#include <rescoring_general.h>
#include <linearmetric.h>
#include <boostDef.h>
#include <vector>

using namespace Portage;
using namespace std;

int main()
{
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

   vector< vector<LinearMetric> > bleu;
   bleu.push_back(vector<LinearMetric>());
   bleu.back().push_back(1);
   bleu.back().push_back(2);
   bleu.push_back(vector<LinearMetric>());
   bleu.back().push_back(1);
   bleu.back().push_back(2);

   linemax(p, dir, vH, bleu);

   cout << dir << endl;
}
