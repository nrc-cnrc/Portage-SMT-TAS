/**
 * @author George Foster
 * @file tm/try.cc  Program template.
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
#include <iomanip>
#include <arg_reader.h>
#include <tm_io.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
prog [-v][-n nl][-m do_markup] [infile [outfile]]\n\
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
static Uint do_markup = 0;
static string src_given_tgt;
static string tgt_given_src;
static ifstream ifs;
static ofstream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);
   istream& is = *isp;
   //ostream& os = *osp;

   cout << setprecision(20);

   Uint i;
   float x = 0;
   for (i = 0; i < 2000000000; ++i)
      x += 1;

   cout << i << endl;
   cout << x << endl;

   exit (0);
      
   Uint lineno = 0;
   vector<string> toks;
   string line;

   while (getline(is, line) && (lineno++ < num_lines || num_lines == 0)) {
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "n:", "m:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n", num_lines);
   arg_reader.testAndSet("m", do_markup);
   
   arg_reader.testAndSet(0, "infile", &isp, ifs);
   arg_reader.testAndSet(1, "outfile", &osp, ofs);
}   
