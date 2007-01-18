/**
 * @author George Foster
 * @file ibm_entropy.cc  Program that calculates per-src-word entropy for IBM models.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <cmath>
#include <printCopyright.h>
#include "ibm.h"
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
ibm_entropy [-v] ibm-model\n\
\n\
Calculate the bitwise entropy for each source word in the given IBM model file.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string ibmfile;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "ibm_entropy");
   getArgs(argc, argv);

   IBM1 ibm1(ibmfile);

   vector<string> src_voc;
   ibm1.getTTable().getSourceVoc(src_voc);

   double b = log(2.0);

   for (Uint i = 0; i < ibm1.getTTable().numSourceWords(); ++i) {

      double h = 0.0;
      TTable::SrcDistn sd = ibm1.getTTable().getSourceDistn(src_voc[i]);
      
      for (Uint j = 0; j < sd.size(); ++j)
	 if (sd[j].second)
	    h += sd[j].second * log(sd[j].second);
      
      cout << src_voc[i] << " " << -h / b << endl;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 1, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   
   arg_reader.testAndSet(0, "ibm-file", ibmfile);
}   
