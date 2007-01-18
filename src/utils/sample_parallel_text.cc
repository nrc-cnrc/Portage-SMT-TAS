/**
 * @author George Foster
 * @file sample_parallel_text.cc  Application.
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
#include <assert.h>
#include <iostream>
#include <numeric>
#include <set>
#include "arg_reader.h"
#include "file_utils.h"
#include "gfstats.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
sample_parallel_text [-r][-n samplesize][-s suffix][-l min-max] \n\
                     src tgt1 [tgt2 tgt3 ...]\n\
\n\
Produce a sub-sample of a parallel text, with one or more target texts. Output\n\
files are written to <src>.<suffix> and <tgt_i>.<suffix>.\n\
\n\
Options:\n\
\n\
-r  Sample randomly, without replacement [sample by choosing lines at fixed intervals]\n\
-n  Produce a sample of size samplesize [min(100,input-size)]\n\
-s  Use given suffix for output files [.sample]\n\
-l  Choose segments having between min and max tokens (inclusive). Max may be omitted\n\
    to specify no limit. This is incompatible with -r, and -n, and always extracts all\n\
    matching segments.\n\
";

// globals

static bool randomsel = false;
static Uint samplesize = 100;
static string length_spec = "";
static Uint minlength, maxlength;
static string suffix = ".sample";
static string srcfilename;
static vector<string> tgtfilenames;

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   set<Uint> line_numbers;

   // count lines & tokens

   Uint num_lines = 0;
   { // extra scope so sfile is closed when it falls out of scope
      IMagicStream sfile(srcfilename);
      string line;
      vector<string> toks;
      while (getline(sfile, line)) {
         if (length_spec != "") {
            toks.clear();
            Uint ntoks = split(line, toks);
            if (ntoks >= minlength && ntoks <= maxlength)
               line_numbers.insert(num_lines);
         }
         ++num_lines;
      }
   }

   // generate random set of lines to output

   samplesize = min(samplesize, num_lines);
   if (length_spec == "") {
      if (randomsel)
         while (line_numbers.size() < samplesize)
            line_numbers.insert(rand(num_lines));
      else
         for (Uint i = 0; i < samplesize; ++i)
            line_numbers.insert(Uint(((double)num_lines / samplesize) * i));
      //	 line_numbers.insert(i * num_lines / samplesize);
      assert(line_numbers.size() == samplesize);
   }


   // read input files and write sample files
   
   string srcsamplefilename = srcfilename + suffix;
   IMagicStream srcfile(srcfilename);
   OMagicStream srcsamplefile(srcsamplefilename);

   vector<string> tgtsamplefilenames(tgtfilenames.size());
   vector<istream*> tgtfiles(tgtfilenames.size());
   vector<ostream*> tgtsamplefiles(tgtfilenames.size());
   for (Uint i = 0; i < tgtfilenames.size(); ++i) {
      tgtsamplefilenames[i] = tgtfilenames[i] + suffix;
      tgtfiles[i] = new IMagicStream(tgtfilenames[i]);
      tgtsamplefiles[i] = new OMagicStream(tgtsamplefilenames[i]);
   }

   num_lines = 0;
   string src_line;
   vector<string> tgt_lines(tgtfilenames.size());
   for (set<Uint>::iterator p = line_numbers.begin(); p != line_numbers.end(); ++p) {
      for (; num_lines < *p+1; ++num_lines) {
	 if (!getline(srcfile, src_line))
	    error(ETFatal, "source file " + srcfilename + " too short!");
         for (Uint i = 0; i < tgtfilenames.size(); ++i)
            if (!getline(*tgtfiles[i], tgt_lines[i]))
               error(ETFatal, "target file " + tgtfilenames[i] + " too short!");
      }
      srcsamplefile << src_line << endl;
      for (Uint i = 0; i < tgtfilenames.size(); ++i)
         (*tgtsamplefiles[i]) << tgt_lines[i] << endl;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   char* switches[] = {"r", "n:", "s:", "l:"};

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("r", randomsel);
   arg_reader.testAndSet("n", samplesize);
   arg_reader.testAndSet("s", suffix);
   arg_reader.testAndSet("l", length_spec);
   
   arg_reader.testAndSet(0, "src", srcfilename);
   arg_reader.getVars(1, tgtfilenames);

   if (length_spec != "") {
      if (arg_reader.getSwitch("r") || arg_reader.getSwitch("n"))
         error(ETFatal, "Switches -r and -n are not compatible with -l");
      vector<string> toks;
      if (split(length_spec, toks, "- ,") != 2)
         error(ETFatal, "-l argument must be in the form 'min-max'");
      minlength = conv<Uint>(toks[0]);
      maxlength = conv<Uint>(toks[1]);
   }
}   
