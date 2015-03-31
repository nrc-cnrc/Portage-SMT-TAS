/**
 * @author George Foster
 * @file gen_jpt_filter_tool.cc 
 * @brief Specialized filter program for gen-jpt-parallel.sh 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <voc.h>
#include <arg_reader.h>
#include <printCopyright.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
gen_jpt_filter_tool [-l lang] vocfile phrasefile\n\
\n\
Special-purpose filtering program used by gen-jpt-parallel.sh.\n\
Filter the single-word phrase pairs in <phrasefile> by matching their initial\n\
column (indicating the number of corpora that generated them) with the initial\n\
column of the corresponding entry in <vocfile>. Only if these columns match,\n\
indicating that a phrase pair was generated from all of the corpora in which\n\
its source word was found (or target word if -l is 2) is the pair written to\n\
stdout.\n\
\n\
Options:\n\
\n\
-l The column in <phrasefile> that <vocfile> pertains to: the 1st if <lang> is\n\
   1, the 2nd if it is 2. [1]\n\
";

// globals

static bool verbose = false;
static Uint lang = 1;
static string vocfilename;
static string phrasefilename;
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2009, "gen_jpt_filter_tool");
   getArgs(argc, argv);

   if (lang != 1 && lang != 2)
      error(ETFatal, "bad value for -l switch: %d", lang);

   iSafeMagicStream vocfile(vocfilename);
   CountingVoc voc;

   string line;
   vector<string> toks;
   while (getline(vocfile, line)) {
      if (splitZ(line, toks) != 2)
         error(ETFatal, "expecting lines in format 'count word' in vocfile");
      Uint count = conv<Uint>(toks[0]);
      voc.add(toks[1].c_str(), count);
   }

   iSafeMagicStream phrasefile(phrasefilename);

   while (getline(phrasefile, line)) {
      Uint tok_count = splitZ(line, toks);
      if (tok_count != 6 && (tok_count != 7 && toks[6] != "a=0"))
         error(ETFatal, "expecting lines in format 'src ||| tgt ||| 1' in phrasefile, with optional 'a=0' at the end");
      const string& word = lang == 1 ? toks[1] : toks[3];
      if (voc.freq(word.c_str()) == conv<Uint>(toks[0])) {
         cout << toks[1] << " ||| " << toks[3] << " ||| " << 1;
         if (tok_count == 7) cout << " " << toks[6];
         cout << "\n";
      }

      if (verbose)
         cerr << "matching on " << word << ": " 
              << "voc freq = " << voc.freq(word.c_str())
              << ", phrase freq = " << conv<Uint>(toks[0]) << endl;

   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "l:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("l", lang);

   arg_reader.testAndSet(0, "vocfile", vocfilename);
   arg_reader.testAndSet(1, "phrasefile", phrasefilename);
}
