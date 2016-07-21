/**
 * @author George Foster
 * @file count_unal_words.cc
 * @brief Count unaligned words in a set of jtps with word-alignment info.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "voc.h"
#include "file_utils.h"
#include "arg_reader.h"
#include "phrase_table.h"
#include "word_align_io.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
count_unal_words [options] jpt1 [jpt2... ]\n\
\n\
Count unaligned words in a set of jpts with word-alignment info. Write a list\n\
of words, with frequencies, to stdout.\n\
\n\
Options:\n\
\n\
  -v    Write each extracted word to stderr, along with its phrase context.\n\
  -l    Specify language for unal words: 1 (left) or 2 (right) [1]\n\
";

// globals

static bool verbose = false;
static Uint lang = 1;
static vector<string> files;

static void getArgs(int argc, char* argv[]);

void writeContextPhrase(const string& word, const PhraseTableUint::iterator& p, 
                        const string& al)
{
   static string src, tgt;
   cerr << word << " - unaligned in: "
        << p.getPhrase(1, src) << " ||| " << p.getPhrase(2, src) << " ||| "
        << p.getJointFreq() << " " << al << endl;
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2011, "count_unal_words");
   getArgs(argc, argv);
   CountingVoc voc;

   for (Uint i = 0; i < files.size(); ++i) {
      PhraseTableUint pt;
      pt.readJointTable(files[i], true);
      
      string src, tgt, al;
      vector<string> stoks, ttoks;
      vector< vector<Uint> > sets;
      vector<bool> tpos;

      for (PhraseTableUint::iterator p = pt.begin(); p != pt.end(); ++p) {
         
         p.getPhrase(1, stoks);
         p.getPhrase(2, ttoks);
         p.getAlignmentString(al, false, true); // not reversed, top only

         GreenReader('_').operator()(al, sets);
         if (sets.size() < stoks.size())
            sets.resize(stoks.size()); // pad w/ empty sets if some missing

         if (lang == 1) {
            for (Uint i = 0; i < sets.size(); ++i)
               if (sets[i].empty() || sets[i][0] == ttoks.size()) {
                  voc.add(stoks[i].c_str(), p.getJointFreq());
                  if (verbose) writeContextPhrase(stoks[i], p, al);
               }
         } else {
            tpos.resize(ttoks.size(), false);
            // use of stoks.size() rather than sets.size() deliberate here: we
            // treat aligned-to-null as unaligned
            for (Uint i = 0; i < stoks.size(); ++i)
               for (Uint j = 0; j < sets[i].size(); ++j)
                  tpos[sets[i][j]] = true;
            for (Uint i = 0; i < ttoks.size(); ++i)
               if (!tpos[i]) {
                  voc.add(ttoks[i].c_str(), p.getJointFreq());
                  if (verbose) writeContextPhrase(ttoks[i], p, al);
               }
         }
      }
   }

   for (Uint i = 0; i < voc.size(); ++i)
      cout << voc.word(i) << ' ' << voc.freq(i) << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "l:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("l", lang);
   arg_reader.getVars(0, files);

   if (lang != 1 && lang != 2)
      error(ETFatal, "error: language must be either 1 or 2");
}
