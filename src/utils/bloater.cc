/**
 * @author George Foster
 * @file bloater.cc Program template.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
bloater blocksize\n\
\n\
Copy <infile> to <outfile> (default stdin to stdout).\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-n  Copy only first <nl> lines [0 = all]\n\
";

// globals

static bool verbose = false;
static Uint num_lines = 0;
static Uint blocksize = 0;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   Uint tot_size = 0;
   //Uint output_interval = 1000000;
   char* bloat_vect;
   Uint num_blocks = 0;

   while (1) {

      bloat_vect = new char[blocksize];
      bloat_vect[0] = 1;
      for (size_t i = 0; i * 256 < blocksize; ++i)
         bloat_vect[i*256] = i;

      tot_size += blocksize;
      // if (tot_size / output_interval != (tot_size+blocksize) / output_interval) {
         if (verbose)
            cerr << ++num_blocks << " total size = " << tot_size << endl;
         sleep(1);
      // }

   }

   cout << bloat_vect[0] << endl;
   
   
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n", num_lines);
  
   arg_reader.testAndSet(0, "blocksize", blocksize);
}   
