/**
 * @author Aaron Tikuisis
 *     **this copied+modified by Matthew Arnold for alignment purposes
 *  **Modified by Nicola Ueffing to use all decoder models
 * @file phrase_tm_align.cc  Use the decoder to phrase align source and target text.
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
#include "inputparser.h"
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
"Usage: phrase_tm_align [-noscore][-onlyscore][-n N][-K K][-out file]\n\
        -f canoefile -ref targetfile < [sourcefile]\n\
\n\
Specify phrase tables and search settings in canoefile.\n\
Use the phrase tables to find phrase alignments for given source and target files.\n\
Each source sentence may have a fixed number K of contiguous translations in the\n\
target file. For each source/target pair, output consists of the source\n\
sentence followed by N alignments (blank lines if alignments can't be found).\n\
Alignments are represented by sequences of source ranges, with each range followed\n\
by the corresponding target phrase in parentheses.\n\
\n\
By default the algorithm is exhaustive and slow; to speed it up, use switch\n\
settings of -s 100 -b 0.0001 (canoe defaults), and optionally a distortion\n\
limit of 10 to 15.\n\
\n\
Options:\n\
 -noscore              Do not output scores with sentences\n\
 -onlyscore            Do not output sentences, only scores\n\
 -n N                  The number N of output alignments to print [1]\n\
 -K K                  The number of target translations per source sentence.  This must\n\
                       be specified if -load-first is used.  Otherwise, this value is\n\
                       determined automatically.\n\
 -out file             Output file\n\
 -ref file             Target corpus, to align with sourcefile\n\
\n\
  For more options, see canoe -help message.\n\
  Note: Since the target sentence is fixed, not all decoder features are\n\
        relevant, although their scores can still be calculated and displayed\n\
        if -ffvals is specified.\n\
\n\
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

   CanoeConfig c;
   static vector<string> args = c.getParamList();

   const char* switches[args.size() + 6];
   for (Uint i = 0; i < args.size(); ++i)
      switches[i] = args[i].c_str();
   switches[args.size()] = ("n:");
   switches[args.size()+1] = ("k:");
   switches[args.size()+2] = ("noscore");
   switches[args.size()+3] = ("onlyscore");
   switches[args.size()+4] = ("out:");
   switches[args.size()+5] = ("ref:");

   char help[strlen(HELP) + strlen(argv[0])];
   sprintf(help, HELP, argv[0]);
   ArgReader argReader(ARRAY_SIZE(switches), switches, 0, 0, help, "-h", false);
   argReader.read(argc - 1, argv + 1);

   if (!argReader.getSwitch("f", &c.configFile) &&
       !argReader.getSwitch("config", &c.configFile))
      error(ETFatal, "No config file given.  Use -h for help.");

   c.read(c.configFile.c_str());  // set parameters from config file
   c.setFromArgReader(argReader); // override from cmd line
   c.check();
   PhraseTable::log_almost_0 = c.phraseTableLogZero;

   if (c.verbosity >= 2)
      c.write(cerr, 2);

   bool noscore=false, onlyscore=false;
   Uint N=1, K=1;
   string outFile = "-";
   string refFile;
   argReader.testAndSet("noscore", noscore);
   argReader.testAndSet("onlyscore", onlyscore);
   argReader.testAndSet("n", N);
   argReader.testAndSet("k", K);
   argReader.testAndSet("out", outFile);
   argReader.testAndSet("ref", refFile);

   if (noscore && onlyscore) {
      cerr << "Your parameter choices do not make sense! Choose either noscore or onlyscore!" << endl;
      exit(1);
   }

   // Since we are decoding to a fixed target sentence, the LM, IBM1 and length
   // features cannot be relevant, so we give them a weight of 0.
   string weightstr;
   c.getFeatureWeightString(weightstr);
   cerr << "Using only phrase table, distortion and segmentation features." << endl
        << "   Changing feature weights accordingly" << endl
        << "   Original weights were " << weightstr << endl;
   c.lmWeights.assign(c.lmWeights.size(), 0.0);
   c.ibm1FwdWeights.assign(c.ibm1FwdWeights.size(), 0.0);
   c.lengthWeight = 0.0;

   c.getFeatureWeightString(weightstr);
   cerr << "   Changed weights are    " << weightstr << endl;

   // Read source sentences.
   vector<vector<string> > src_sents;
   vector<vector<MarkedTranslation> > marked_src_sents;
   iSafeMagicStream input(c.input);
   InputParser reader(input);
   if ( ! c.loadFirst) {
      cerr << "Reading input source sentences..." << endl;
      while (true) {
         src_sents.push_back(vector<string>());
         marked_src_sents.push_back(vector<MarkedTranslation>());
         if ( ! reader.readMarkedSent(src_sents.back(), marked_src_sents.back()) ) {
            if ( c.tolerateMarkupErrors )
               error(ETWarn, "Tolerating ill-formed markup, but part of the last input line has been discarded.");
            else
               error(ETFatal, "Aborting because of ill-formed markup.");
         } // if

         if (reader.eof() && src_sents.back().empty()) {
            src_sents.pop_back();
            marked_src_sents.pop_back();
            break;
         } // if
      } // while
      reader.reportWarningCounts();
      cerr << "... done reading input source sentences." << endl;
   } // if

   // Read reference (target) sentences.
   if (refFile=="")
      error(ETFatal, "You have to provide a reference file!\n");
   iSafeMagicStream ref(refFile);
   vector<vector<string> > tgt_sents;
   readSentences(ref, tgt_sents);

   // Check if numbers of source and target sentences match
   if ( ! c.loadFirst ) {
      if (tgt_sents.size() % src_sents.size() != 0)
         error(ETFatal, "Number of target lines (%d) in %s is not a multiple of the number of source lines (%d) in %s",
               tgt_sents.size(), refFile.c_str(), src_sents.size(), c.input.c_str());
      if (K>0) {
         if (tgt_sents.size() / src_sents.size() != K) {
            error(ETWarn, "K specified on command-line (%d) does not match computed K.  Using K=%d.",
                  K, min(K, tgt_sents.size() / src_sents.size()));
            K = min(K, tgt_sents.size() / src_sents.size());
         } // if
      }
      else
         K = tgt_sents.size() / src_sents.size();
   }
   cerr << "Determining alignments for " << K << " target sentences per source sentence."
      << endl;

   time_t start;
   time(&start);

   PhraseTMAligner aligner(c, src_sents, marked_src_sents);
   cerr << "Loaded data structures in " << difftime(time(NULL), start)
        << " seconds." << endl;

   if (!c.loadFirst)
      cerr << "Aligning " << src_sents.size() << " source sentences." << endl;
   else
      cerr << "Reading and aligning sentences..." << endl;

   time(&start);

   // Open output file
   cerr << N << " best alignments per sentence pair will be written to '" << outFile << "'" << endl;
   oSafeMagicStream out(outFile);

   Uint i = 0;
   while (true) {
      cerr << ".";

      // source sentence
      vector<string> src_sent;
      vector<MarkedTranslation> marked_src_sent;
      if ( c.loadFirst ) {
         if ( ! reader.readMarkedSent(src_sent, marked_src_sent) ) {
            if ( c.tolerateMarkupErrors )
               error(ETWarn, "Tolerating ill-formed markup, but part of the last input line has been discarded.");
            else
               error(ETFatal, "Aborting because of ill-formed markup.");
         }
         else {
            cerr << "Read src sentence of length " << src_sent.size() << "." << endl;
         }
         if (reader.eof()) break;
      }
      else {
         if (i == src_sents.size())
            break;
         src_sent = src_sents[i];
         marked_src_sent = marked_src_sents[i];
      }

      // target sentence
      for (Uint k = 0; k < K; k++) {

         vector<string> tgt_sent = tgt_sents[i * K + k];

         stringstream ss;
         aligner.computePhraseTM(src_sent, marked_src_sent, tgt_sent, ss, N,
                                 noscore, onlyscore,
                                 c.pruneThreshold, c.maxStackSize,
                                 c.covLimit, c.covThreshold);
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
   if ( !c.loadFirst ) reader.reportWarningCounts();
} // main
