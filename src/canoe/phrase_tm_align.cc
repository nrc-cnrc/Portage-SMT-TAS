/**
 * @author Aaron Tikuisis
 *     **this copied+modified by Matthew Arnold for alignment purposes
 * @file phrase_tm_align.cc  This file contains what used to be for
 * phrase_tm_score which is now being re-used for alignment training purposes
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrase_tm_align.h"
#include <canoe_general.h>
#include <phrasetable.h>
#include <str_utils.h>
#include <file_utils.h>
#include <portage_defs.h>
#include <errors.h>
#include <arg_reader.h>
#include <printCopyright.h>
#include <iostream>
#include <ext/hash_map>
#include <sstream>

using namespace std;
using namespace __gnu_cxx;
using namespace Portage;

/// Program phrase_tm_align's usage.
const char *HELP =
"Usage: phrase_tm_align [-v V][-noscore][-onlyscore][-noannoy][-n N][-load-first -K K]\n\
                       [-distortion-limit D][-distortion-model M][-weight-d W]\n\
                       [-segmentation-model M][-weight-s W]\n\
                       [-s S][-b T]\n\
                       phrases.source_given_target source targets [outfile]\n\
\n\
Use a phrase table to find phrase alignments for given source and target files.\n\
Each source sentence may have a fixed number K of contiguous translations in the\n\
target file. For each source/target pair, output consists of the source\n\
sentence followed by N alignments (blank lines if alignments can't be found).\n\
Alignments are represented by sequences of source ranges, with each range followed\n\
by the corresponding target phrase in parentheses.\n\
\n\
By default the algorithm is exhaustive and slow; to speed it up, use switch\n\
settings of -s 100 -b 0.0001 (canoe defaults), and optionally a distortion\n\
limit of 10-15.\n\
\n\
Options:\n\
 -v V                  The verbosity level (1 to 4).  Similar to canoe's verbose output\n\
 -noscore              Do not output scores with sentences\n\
 -onlyscore            Do not output sentences, only scores\n\
 -noannoy              Suppress annoying info line at beginning of output.\n\
 -n N                  The number N of output alignments to print [1]\n\
 -load-first           Loads the phrase table first (so that not all sentences are stored\n\
                       in memory at once).  If this option is used, the value K must be\n\
                       specified.\n\
 -K K                  The number of target translations per source sentence.  This must\n\
                       be specified if -load-first is used.  Otherwise, this value is\n\
                       determined automatically.\n\
 -distortion-model model[#args]\n\
                       The distortion model (see canoe for choices and default).]\n\
 -distortion-limit D   The maximum distortion allowed. [NO_MAX_DISTORTION]\n\
 -weight-d W           The weight given to the distortion cost, relative to the weight\n\
                       given to the translation score.  [1.0]\n\
 -segmentation-model model[#args]\n\
                       The segmentation model (see canoe for choices and default).]\n\
 -weight-s W           The weight given to the segmentation model, relative to the weight\n\
                       given to the translation score.  [1.0]\n\
 -s S                  The hypothesis stack size. [1000]\n\
 -b T                  The hypothesis stack relative threshold [10e-9]\n\
";

/**
 * Reads and tokenizes sentences.
 * @param[in]  in     from what to read the sentences.
 * @param[out] sents  returned tokenized sentences.
 */
void readSentences(istream &in, vector< vector<string> > &sents)
{
   while (true)
   {
      string line;
      getline(in, line);
      if (in.eof() && line == "") break;
      sents.push_back(vector<string>());
      split(line, sents.back(), " ");
   } // while
} // readSentences


/**
 * Program phrase_tm_align's entry point.
 * @param argc  number of command line arguments.
 * @param argv  vector containing the command line arguments.
 * @return Returns 0 if successful.
 */
int main(Uint argc, const char * const * argv)
{
   printCopyright(2005, "phrase_tm_align");
   const char *switches[] = {"n:", "distortion-limit:", "distortion-model:","weight-d:",
                             "segmentation-model:", "segmentation-args:", "weight-s:",
                             "noscore", "onlyscore", "noannoy", "load-first", "K:","s:","b:","v:"};
   ArgReader argReader(ARRAY_SIZE(switches), switches, 3, 4, HELP);
   argReader.read(argc - 1, argv + 1);

   const string &phraseFile = argReader.getVar(0);
   const string &srcFile = argReader.getVar(1);
   const string &tgtFile = argReader.getVar(2);
   string outFile = (argReader.numVars() == 4 ? argReader.getVar(3) : "-");
   Uint N = 1;//

   string distModel="", segModel="", segArgs="";

   CanoeConfig c;
   c.segWeight.resize(1);
   c.segWeight[0] = 1.0;
   c.distWeight.resize(1);
   c.verbosity = 1;

   Uint K;
   bool noscore   = false;
   bool onlyscore = false;
   bool noannoy   = false;

   // Denis: modifications to reduce running time, grabbing hypothesis
   // restrictions from canoe
   // EJJ: to change these defaults for your experiments, use the -b and -s
   // options!
   c.maxStackSize     = 1000;  // NO_SIZE_LIMIT;  // Denis: 100
   c.pruneThreshold   = 10e-9; // 0               // Denis: 0.0001
   argReader.testAndSet("s", c.maxStackSize);
   argReader.testAndSet("b", c.pruneThreshold);

   argReader.testAndSet("noscore", noscore);
   argReader.testAndSet("onlyscore", onlyscore);
   argReader.testAndSet("n", N);
   argReader.testAndSet("load-first", c.loadFirst);
   argReader.testAndSet("distortion-limit", c.distLimit);
   argReader.testAndSet("distortion-model", c.distortionModel);
   argReader.testAndSet("weight-d", c.distWeight[0]);
   argReader.testAndSet("segmentation-model", c.segmentationModel);
   argReader.testAndSet("weight-s", c.segWeight[0]);
   argReader.testAndSet("K", K);
   argReader.testAndSet("v", c.verbosity);
   argReader.testAndSet("v", c.verbosity);
   argReader.testAndSet("noannoy", noannoy);

   if (c.distortionModel.empty())
      c.distWeight.clear();
   else
      cerr << "Distortion model: " << c.distortionModel[0] << " with weight " << c.distWeight[0] << endl;
   if (c.segmentationModel != "")
      cerr << "Segmentation model: " << c.segmentationModel << " with weight " << c.segWeight[0] << endl;

   if (noscore && onlyscore) {
      cerr << "Your parameter choices do not make sense! Choose either noscore or onlyscore!" << endl;
      exit(1);
   }

   // Open output file
   cerr << N << " best alignments will be written to '" << outFile << "'" << endl;
   OMagicStream out(outFile);
   // cerr << " opened" << endl;

   vector<vector<string> > src_sents;
   vector<vector<string> > tgt_sents;
   // cerr << "opened" << endl;
   // Open input files
   IMagicStream sin(srcFile);
   IMagicStream tin(tgtFile);

   // cerr << "input" << endl;
   if ( ! c.loadFirst ) {
      // Read sentences.
      // cerr << "load" << endl;
      readSentences(sin, src_sents);
      readSentences(tin, tgt_sents);
      // cerr << "read" << endl;
      if (tgt_sents.size() % src_sents.size() != 0)
         error(ETFatal, "Number of lines in %s is not a multiple of the number of lines in %s",
               tgtFile.c_str(), srcFile.c_str());
      if (argReader.getSwitch("K")) {
         if (tgt_sents.size() / src_sents.size() != K) {
            error(ETWarn, "K specified on command-line (%d) does not match computed K.  Using K=%d.",
                  K, min(K, tgt_sents.size() / src_sents.size()));
            K = min(K, tgt_sents.size() / src_sents.size());
         } // if
      }
      else
         K = tgt_sents.size() / src_sents.size();
   }
   else if (!argReader.getSwitch("K")) {
      error(ETFatal, "Missing -K switch");
   } // if
   PhraseTMAligner aligner(c, phraseFile, src_sents);
   Uint i = 0;
   string line;
   if (!noannoy)
      out << "#" << N << " alignments per source sentence" << endl;
   cerr << "Scoring";
   while (true) {
      cerr << ".";

      // source sentence
      vector<string> src_sent;
      if ( c.loadFirst ) {
         line.clear();
         getline(sin, line);
         if (line != "" && tin.eof()) {
            error(ETWarn, "Ran out of target sentences prematurely.");
            break;
         }
         if (sin.eof() && line == "") break;
         split(line, src_sent, " ");
      }
      else {
         if (i == src_sents.size())
            break;
         src_sent = src_sents[i];
      }
      for (Uint k = 0; k < K; k++)
      {
         vector<string> tgt_sent;
         if (c.loadFirst)
         {
            line.clear();
            getline(tin, line);
            if (tin.eof() && line == "")
            {
               error(ETWarn, "Ran out of target sentences prematurely.");
               break;
            } // if
            split(line, tgt_sent, " ");
         } else
         {
            tgt_sent = tgt_sents[i * K + k];
         } // if

         stringstream ss;
         aligner.computePhraseTM(src_sent, tgt_sent, ss, N, noscore, onlyscore,
               c.pruneThreshold, c.maxStackSize, c.covLimit, c.covThreshold);
         string s = ss.str();    //s contains the alignments

         // print source sentence
         if (!onlyscore) {
            for (Uint ind = 0; ind < src_sent.size(); ind++)
               out << " " << src_sent[ind];
            out << endl;
         }
         // print alignments
         out << s << flush;
      } // for
      i++;
   } // while
} // main
