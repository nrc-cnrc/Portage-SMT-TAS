/**
 * @author George Foster
 * @file find_sentence_phrases.cc  Find matching phrase pairs for input sentences.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <file_utils.h>
#include <cmath>
#include <arg_reader.h>
#include <basicmodel.h>
#include <tm_io.h>
#include "inputparser.h"
#include <printCopyright.h>

using namespace Portage;
using namespace std;

/// Program find_sentence_phrase's usage.
static char help_message[] = "\n\
find_sentence_phrases [-vlswg][-n n][-forward file] src_given_tgt tgt_given_src\n\
                      [infile [outfile]]\n\
\n\
Find the set of phrase pairs from a phrase table whose source halves match a\n\
given set of tokenized sentences. Any markup in the sentences is stripped \n\
before matching, ie <date 01-Jan-1999>Jan 1st 99</date> is replaced with\n\
'Jan 1st 99'.\n\
The <src_given_tgt> and <tgt_given_src> parameters are phrase tables in both\n\
directions, and <infile> contains source sentences. Output phrase pairs are\n\
written one per line, in phrase table format:\n\
   src-phrase ||| tgt-phrase ||| p(src-phrase|target-phrase).\n\
\n\
Note: Unless -g is used, output is a sequence of per-sentence phrase options.\n\
To turn it into valid phrase table(s), use sort -u.\n\
\n\
Options:\n\
\n\
-w  Warn about untranslated source words.\n\
-l  Write log probabilities [write plain probabilities]\n\
-s  Write source sentence before, and blank line after, the set of phrase pairs\n\
    for each source sentence [don't]\n\
-g  Write global phrase table(s), rather than per-sentence ones\n\
    (disables -l and -s, forces -w)\n\
-n  Write only best n translations for each source phrase (0 for all) [30]\n\
-forward Write a forward phrase table (tgt, src, p(tgt|src)) to <file>.\n\
";
// EJJ: remove -v since it doesn't actually do anything.
// -v  Echo source sentences prior to the list of phrases they generate.

// globals

static bool warn_untrans = false;
static bool logprobs = false;
static bool writeseps = false;
static bool global = false;
static Uint num_trans = 30;
static string forwardfile;
static string src_given_tgt;
static string tgt_given_src;
static string in_file;
static string out_file;

/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  vector of command line arguments
 */
static void getArgs(int argc, char* argv[]);




/**
 * Program find_sentence_phrases's entry point.
 * @param argc  number of command line arguments.
 * @param argv  vector containing the command line arguments.
 * @return Returns 0 if successful.
 */
int main(int argc, char* argv[])
{
   printCopyright(2005, "find_sentence_phrases");
   getArgs(argc, argv);
   IMagicStream is(in_file.size() ? in_file : "-");
   OMagicStream os(out_file.size() ? out_file : "-");
   OMagicStream fwd(forwardfile.size() ? forwardfile : "-");

   vector<vector<string> > sents;
   InputParser reader(is);
   vector<MarkedTranslation> dummy; // Required to call readMarkedSent()
   while (!reader.eof()) {
      sents.push_back(vector<string>());
      reader.readMarkedSent(sents.back(), dummy);
   }
   reader.reportWarningCounts();
   if ( !sents.empty() && sents.back().empty() ) sents.pop_back();

   CanoeConfig c;
   c.loadFirst = false;
   c.verbosity = 1;
   c.phraseTableSizeLimit = num_trans;

   vector<vector<MarkedTranslation> > marks;
   BasicModelGenerator bmg(c, sents, marks);
   bmg.addTranslationModel(src_given_tgt.c_str(), tgt_given_src.c_str(), 1.0);
   PhraseTable& phrasetable = bmg.getPhraseTable();

   if (global) {
      phrasetable.write(&os, forwardfile.size() ? &fwd : NULL);
      return 0;
   }

   vector<MarkedTranslation> mark;
   TScore tscore;
   string buffer;
   for (Uint s = 0; s < sents.size(); ++s) {
      if (writeseps) {
         buffer.clear();
         os << TMIO::joinTokens(sents[s], buffer, true) << endl;
      }
      PhraseDecoderModel* pdm = bmg.createModel(sents[s], mark);
      vector<PhraseInfo*> **phrases = pdm->getPhraseInfo();

      Uint len = sents[s].size();
      for (Uint i = 0; i < len; ++i) {
         for (Uint j = 0; j < len-i; ++j) {
            for (Uint k = 0; k < phrases[i][j].size(); ++k) {
               string src, tgt;
               PhraseInfo* pi = phrases[i][j][k];
               join(sents[s].begin()+pi->src_words.start,
                    sents[s].begin()+pi->src_words.end, src);
               pdm->getStringPhrase(tgt, pi->phrase);

               bool trans = true;
               if (forwardfile.size()) {
                  if (!phrasetable.getPhrasePair(src, tgt, tscore)) {
                     trans = false;
                     if (warn_untrans)
                        error(ETWarn, "can't find phrase pair in phrasetable: %s ||| %s",
                              src.c_str(), tgt.c_str());
                  }
                  if (trans && tscore.forward.size() == 0)
                     error(ETFatal, "no forward probabilities for phrase pair: %s ||| %s",
                           src.c_str(), tgt.c_str());
                  if (trans)
                     fwd << tgt << " ||| " << src << " ||| " <<
                        (logprobs ? tscore.forward[0] : exp(tscore.forward[0])) << endl;
               }
               if (trans)
                  os << src << " ||| " << tgt << " ||| " <<
                     (logprobs ? pi->phrase_trans_prob : exp(pi->phrase_trans_prob)) <<
                     endl;
            }
         }
      }
      if (writeseps) {
         os << endl;
         if (forwardfile.size())  fwd << endl;
      }
      delete pdm;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "w", "l", "s", "g",  "n:", "forward:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("w", warn_untrans);
   arg_reader.testAndSet("l", logprobs);
   arg_reader.testAndSet("s", writeseps);
   arg_reader.testAndSet("n", num_trans);
   arg_reader.testAndSet("g", global);
   arg_reader.testAndSet("forward", forwardfile);

   arg_reader.testAndSet(0, "src_given_tgt", src_given_tgt);
   arg_reader.testAndSet(1, "tgt_given_src", tgt_given_src);
   arg_reader.testAndSet(2, "infile", in_file);
   arg_reader.testAndSet(3, "outfile", out_file);
}
