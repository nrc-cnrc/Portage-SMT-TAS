/**
 * @author George Foster
 * @file paddle_server.cc  Program paddle_server that tests Portage API
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 a*/
#include <iostream>
#include <fstream>
#include <arg_reader.h>
#include "portage_api.h"
#include "net_srv.h"
#include <string>
#include "logging.h"
#include <printCopyright.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
\n\
Usage: paddle_server [-vr][-c config][-pPortNum] srclang tgtlang\n\
       [infile [outfile]]\n\
\n\
Translate <infile> to <outfile> (default stdin to stdout). Text in <infile>\n\
should be free of markup, except for blank lines which may be used to\n\
delimit paragraphs.\n\
\n\
Options:\n\
-v  Write progress reports to cerr.\n\
-c  Use <config> as base name for canoe configuration file (complete filename\n\
    is <config>.<srclang>2<tgtlang>). [std-config]\n\
-r  Treat any model names mentioned in config file as valid pathnames relative\n\
    to the current directory. [Treat them as names of files in demo directory.]\n\
-p  PortNum specify which TCP port number to start listening on for translation\n\
    requests\n\
";

// globals

static bool verbose = false;
static bool rel_models = false;
static string config = "std-config";
static string srclang;
static string tgtlang;
static unsigned int port;
static ifstream ifs;
static ofstream ofs;
static istream* isp = &cin;
static ostream* osp = &cout;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "paddle_server");
   Logging::init();

   getArgs(argc, argv);
   istream& is = *isp;
   ostream& os = *osp;

   PortageAPI portage(srclang, tgtlang, config, !rel_models, verbose);
   
   string tgt;
   string src;
   
   if(port !=0 )
   {
      //We are called with a port value we start the server
      Net_srv network_server(&portage, port);	
      network_server.run();
   }
   else
   {
      //We are called interactively
      string line;
      while (getline(is, line) || src != "") {
         if (line != "") {       // accumulate until blank line
            src += line + "\n";
         } else {                // translate
            portage.translate(src, tgt);
            os << tgt;
            src.clear();
         }
         line.clear();
      }
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"v", "r", "c:", "p:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", rel_models);
   arg_reader.testAndSet("c", config);
   arg_reader.testAndSet("p", port);
   arg_reader.testAndSet(0, "srclang", srclang);
   arg_reader.testAndSet(1, "tgtlang", tgtlang);
   arg_reader.testAndSet(2, "infile", &isp, ifs);
   arg_reader.testAndSet(3, "outfile", &osp, ofs);
   
}   
