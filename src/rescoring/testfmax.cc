/**
 * @author Aaron Tikuisis
 * @file testfmax.cc  Program test unit for fmax.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <fmax.h>
#include <portage_defs.h>
#include <iostream>

using namespace std;
using namespace Portage;

int main(int argc, char* argv[])
{
	 if (argc > 1) {
	    cerr << "Program test unit for fmax." << endl;
		 exit(1);
	 }

   double *f[3];
   f[0] = new double[3];
   f[1] = new double[3];
   f[2] = new double[3];
   f[0][0] = 1; f[0][1] = 4; f[0][2] = 3.1;
   f[1][0] = 2; f[1][1] = 6.1; f[1][2] = 1;
   f[2][0] = 3.1; f[2][1] = 1; f[2][2] = 2;
   int func[3];
   max_1to1_func(f, (Uint)3, (Uint)3, func);
   for (Uint i = 0; i < 3; i++)
   {
      cerr << "f(" << i << ") = " << func[i] << endl;
   } // for
} // main
