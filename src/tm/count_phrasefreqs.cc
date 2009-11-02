/**
 * @author George Foster
 * @file count_phrasefreqs.cc
 * @brief For each phrase in \<phrasefile\>, sum the relative frequencies in
 * joint phrasetable \<jpt\> of all pairs in which that phrase appears on the
 * source side.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <voc.h>
#include "phrase_table_reader.h"
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
count_phrasefreqs [options] phrasefile [jpt]\n\
\n\
For each phrase in <phrasefile>, sum the relative frequencies in joint\n\
phrasetable <jpt> of all pairs in which that phrase appears on the source\n\
side. Write each phrase to stdout, along with its frequency sum.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string phrasefile;
static string jptfile("-");
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   CountingVoc voc;
   iSafeMagicStream phrases(phrasefile);
   string line;
   while (getline(phrases, line)) 
      voc.add(line.c_str(), 0);

   iSafeMagicStream jpt(jptfile);
   PhraseTableReader reader(jpt);

   while (!reader.eof()) {
      PhraseTableEntry e = reader.readNext();
      Uint index = voc.index(e.phrase1.c_str());
      if (index != voc.size())
	 voc.freq(index) += (Uint)e.prob();
   }

   for (Uint i = 0; i < voc.size(); ++i)
      cout << voc.freq(i) << " " << voc.word(i) << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "phrasefile", phrasefile);
   arg_reader.testAndSet(1, "jptfile", jptfile);
}
