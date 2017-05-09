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
        << p.getPhrase(1, src) << " ||| " << p.getPhrase(2, tgt) << " ||| "
        << p.getJointFreq() << " " << al << endl;
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2011, "count_unal_words");
   getArgs(argc, argv);
   CountingVoc voc;

   for (Uint i = 0; i < files.size(); ++i) {
      string al;
      vector<string> stoks, ttoks;
      vector< vector<Uint> > sets;
      vector<bool> tpos;

      iSafeMagicStream in(files[i]);
      string line;

      Voc alignment_voc;
      Uint lineno = 0;
      vector<string> toks;
      while (getline(in, line)) {
         ++lineno;
         toks.clear();
         PhraseTableUint::ToksIter b1, e1, b2, e2, v, a, f;
         PhraseTableBase::extractTokens(line, toks, b1, e1, b2, e2, v, a, f);
         stoks.assign(b1,e1);
         ttoks.assign(b2,e2);
         Uint jointFreq = 0;
         if (!conv(*v, jointFreq))
            error(ETFatal, "Cannot convert count %s to an integer on line %d of phrase table %s",
                  v->c_str(), lineno, files[i].c_str());
         if (a != f) {
            assert((*a)[0] == 'a');
            string alignments = a->substr(2);
            AlignmentFreqs<Uint> alignment_freqs;
            parseAndTallyAlignments(alignment_freqs, alignment_voc, alignments);
            displayAlignments(al, alignment_freqs, alignment_voc, stoks.size(), ttoks.size(), false, true);
         } else {
            al.clear();
         }

      /*
      // EJJ APR2017 - This original code was more elegant, taking advantage of
      // the File iterator strategy implemented for PhraseTableUint, which
      // means not loading the whole phrase table while having the nice,
      // easy-to-use iterator interface. Unfortunately, the iterator stills
      // builds a vocabulary over phrases in each language, and that ends up
      // requiring lots of memory, for something that is not needed in this
      // program. A clean solution would be to modify FileIteratorStrategy so
      // that it looks up id1 and id2 only when needed, i.e., when
      // iterator::getPhraseIndex() gets called, instead of in operator++(),
      // but that would require a pretty significant rewrite of the
      // IteratorStrategy class hierarchy, and I'm not up for that task right
      // now.  So instead, I resort to using the lower-level extractTokens()
      // directly.
      //
      // Results:
      // On a small Hansard JPT with 23M phrase pairs, we go from 130s to 72s
      // runtime, and from 1.0GB to .01GB RAM.
      // On a WMT-Russian JPT with 419M phrase pairs, we go from 49 to 32 min
      // runtime, but, more importantly, from 9.2GB to 0.2GB RAM.
      // Much larger JPTs sometimes required many 10s of GBs, now their
      // footprint will only be dictated by the word-vocabulary size, so
      // typically less than 1GB, even for gigantic systems.

      //PhraseTableUint pt;
      //pt.readJointTable(files[i], true);

      for (PhraseTableUint::iterator p = pt.begin(); p != pt.end(); ++p) {
         
         p.getPhrase(1, stoks);
         p.getPhrase(2, ttoks);
         p.getAlignmentString(al, false, true); // not reversed, top only
         Uint jointFreq = p.getJointFreq();
      */

         GreenReader('_').operator()(al, sets);
         if (sets.size() < stoks.size())
            sets.resize(stoks.size()); // pad w/ empty sets if some missing

         if (lang == 1) {
            for (Uint i = 0; i < sets.size(); ++i)
               if (sets[i].empty() || sets[i][0] == ttoks.size()) {
                  voc.add(stoks[i].c_str(), jointFreq);
                  //if (verbose) writeContextPhrase(stoks[i], p, al);
                  if (verbose)
                     cerr << stoks[i] << " - unaligned in: " << join(stoks) << " ||| "
                          << join(ttoks) << " ||| " << jointFreq << " " << al << endl;
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
                  voc.add(ttoks[i].c_str(), jointFreq);
                  //if (verbose) writeContextPhrase(ttoks[i], p, al);
                  if (verbose)
                     cerr << ttoks[i] << " - unaligned in: " << join(stoks) << " ||| "
                          << join(ttoks) << " ||| " << jointFreq << " " << al << endl;
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
