/**
 * @author George Foster
 * @file testconfig.cc Test the CanoeConfig class
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <arg_reader.h>
#include "config_io.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
testconfig [-what ws][-f configfile][CANOE-OPTIONS]\n\
\n\
Test the CanoeConfig class by reading a config file, processing canoe arguments,\n\
and writing resulting parameters in config-file format. \n\
\n\
Options:\n\
\n\
-what  What to write out: 0 = only parameters set in config file, 1 = params set in config\n\
       or on cmd line, 2 = all parameters. [0]\n\
";

// globals

//static bool verbose = false;
static string configfile = "";
static Uint what = 0;
static CanoeConfig cc;
static vector<string> canoe_args;

static void getArgs(int argc, char* argv[]);



/**
 * Program testconfig's entry point.
 * @param argc  number of command line arguments.
 * @param argv  vector containing the command line arguments.
 * @return Returns 0 if successful.
 */
int main(int argc, char* argv[])
{
   canoe_args = cc.getParamList();
   getArgs(argc, argv);

   cc.check();
   cc.write(cout, what);
}

// arg processing

void getArgs(int argc, char* argv[])
{
   canoe_args.push_back("what:");
   
   const char* switches[canoe_args.size()];
   for (Uint i = 0; i < canoe_args.size(); ++i)
      switches[i] = canoe_args[i].c_str();

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 0, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("what", what);
   arg_reader.testAndSet("f", configfile);

   if (configfile != "") cc.read(configfile.c_str());
   cc.setFromArgReader(arg_reader);
}   
