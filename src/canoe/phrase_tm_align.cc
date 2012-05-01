/**
 * @author Aaron Tikuisis
 *     **this copied+modified by Matthew Arnold for alignment purposes
 *  **Modified by Nicola Ueffing to use all decoder models and to do fuzzy match
 * @file phrase_tm_align.cc
 * @brief Use the decoder to phrase align source and target text.
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrase_tm_align.h"
#include "inputparser.h"
#include "canoe_general.h"
#include "phrasetable.h"
#include "portage_defs.h"
#include "arg_reader.h"
#include <iostream>
#include "printCopyright.h"

using namespace std;
using namespace Portage;

/// Program phrase_tm_align's usage.
const char *HELP =
"Usage: phrase_tm_align [-noscore][-onlyscore][-n N][-K K][-out FILE]\n\
        -f CANOEFILE -ref TARGETFILE < [SOURCEFILE]\n\
\n\
  Most parameters are specified in CANOEFILE: the target corpus (using [ref] or\n\
  -ref on the command line) the phrase tables and other search settings.  See\n\
  canoe -h for details about the options that can be specified in CANOEFILE.\n\
\n\
  Use the phrase tables to find phrase alignments for the source and target\n\
  files given.  Each source sentence may have a fixed number K of contiguous\n\
  translations in the target file. For each source/target pair, output consists\n\
  of the source sentence followed by N alignments (blank lines if alignments\n\
  can't be found).  Alignments are represented by sequences of source ranges,\n\
  with each range followed by the corresponding target phrase in parentheses.\n\
\n\
  By default the algorithm is exhaustive and slow; to speed it up, use settings\n\
  [s] 100 [b] 0.0001, and optionally a [distortion-limit] of 10 to 15.\n\
\n\
  Fuzzy alignment is performed if [weight-lev] or [weight-ngrams] is provided.\n\
\n\
Options:\n\
  -noscore     Do not output scores with sentences\n\
  -onlyscore   Do not output sentences, only scores\n\
  -n N         The number N of output alignments to print [1]\n\
  -K K         The number of target translations per source sentence.  Required\n\
               if [load-first] is used.  Determined automatically otherwise.\n\
  -out FILE    Output file\n\
  -ref TARGETFILE  Target corpus, to align with sourcefile\n\
\n\
  For more options, see canoe -help message.\n\
  Note: Some features are relevant only if fuzzy alignment is performed:\n\
        Levenshtein, n-grams, length feature (=word penalty), LM, IBM1.\n\
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
int main(int argc, const char * const * argv)
{
   printCopyright(2005, "phrase_tm_align");

   CanoeConfig c;
   static vector<string> args = c.getParamList();

   const char* switches[args.size() + 5];
   for (Uint i = 0; i < args.size(); ++i)
      switches[i] = args[i].c_str();
   switches[args.size()+0] = ("n:");
   switches[args.size()+1] = ("k:");
   switches[args.size()+2] = ("noscore");
   switches[args.size()+3] = ("onlyscore");
   switches[args.size()+4] = ("out:");

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

   // Temporary hack because TPPTs don't work with dynamic model filtering
   if ( !c.tpptFiles.empty() && !c.loadFirst ) {
      error(ETWarn, "Setting -load-first option, since dynamic filtering is not currently compatible with TPPTs.");
      c.loadFirst = true;
   }

   bool noscore=false, onlyscore=false;
   Uint N=1, K=1;
   string outFile = "-";
   argReader.testAndSet("noscore", noscore);
   argReader.testAndSet("onlyscore", onlyscore);
   argReader.testAndSet("n", N);
   argReader.testAndSet("k", K);
   argReader.testAndSet("out", outFile);

   if (noscore && onlyscore) {
      cerr << "Your parameter choices do not make sense! Choose either noscore or onlyscore!" << endl;
      exit(1);
   }

   /*
    * Find out whether to determine exact or fuzzy alignment and set weights
    * accordingly:
    *  - Keep all weights for fuzzy alignment.
    *  - For exact alignment, set weights that are constant given the target
    *    sentence to 0: lm, ibm1, length.
    */
   if ( c.levWeight.empty() && c.ngramMatchWeights.empty()) {
      string weightstr;
      c.getFeatureWeightString(weightstr);
      cerr << "Exact phrase match is calculated using only the phrase table and" << endl
         << "   those features that are not fixed given the target sentence." << endl
         << "   Changing feature weights accordingly" << endl
         << "   Original weights were " << weightstr << endl;
      c.lmWeights.assign(c.lmWeights.size(), 0.0);
      c.ibm1FwdWeights.assign(c.ibm1FwdWeights.size(), 0.0);
      c.lengthWeight = 0.0;

      c.getFeatureWeightString(weightstr);
      cerr << "   Changed weights are    " << weightstr << endl;
   }

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
         }

         if (reader.eof() && src_sents.back().empty()) {
            src_sents.pop_back();
            marked_src_sents.pop_back();
            break;
         }
      }
      reader.reportWarningCounts();
      cerr << "... done reading input source sentences." << endl;
   }

   // Read reference (target) sentences.
   if (c.refFile=="")
      error(ETFatal, "You have to provide a reference file!\n");
   iSafeMagicStream ref(c.refFile);
   vector<vector<string> > tgt_sents;
   readSentences(ref, tgt_sents);

   // Check if numbers of source and target sentences match
   if ( ! c.loadFirst ) {
      if (tgt_sents.size() % src_sents.size() != 0)
         error(ETFatal, "Number of target lines (%d) in %s is not a multiple of the number of source lines (%d) in %s",
               tgt_sents.size(), c.refFile.c_str(), src_sents.size(), c.input.c_str());
      if (K>0) {
         if (tgt_sents.size() / src_sents.size() != K) {
            error(ETWarn, "K specified on command-line (%d) does not match computed K.  Using K=%d.",
                  K, min(K, tgt_sents.size() / src_sents.size()));
            K = min(K, tgt_sents.size() / src_sents.size());
         }
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
      newSrcSentInfo nss_info;
      nss_info.internal_src_sent_seq = i;
      if ( c.loadFirst ) {
         if ( ! reader.readMarkedSent(nss_info.src_sent, nss_info.marks, NULL, &(nss_info.external_src_sent_id)) ) {
            if ( c.tolerateMarkupErrors )
               error(ETWarn, "Tolerating ill-formed markup, but part of the last input line has been discarded.");
            else
               error(ETFatal, "Aborting because of ill-formed markup.");
         }
         else {
            cerr << "Read src sentence of length " << nss_info.src_sent.size() << "." << endl;
         }
         if (reader.eof()) break;
      }
      else {
         if (i == src_sents.size())
            break;
         nss_info.src_sent = src_sents[i];
         nss_info.marks    = marked_src_sents[i];
      }

      // target sentence
      for (Uint k = 0; k < K; k++) {

         nss_info.tgt_sent = &tgt_sents[i * K + k];

         // print source sentence
         if (!onlyscore) {
            for (Uint ind = 0; ind < nss_info.src_sent.size(); ind++)
               out << " " << nss_info.src_sent[ind];
            out << endl;
         }

         if ( !c.levWeight.empty() || !c.ngramMatchWeights.empty())
            aligner.computeFuzzyPhraseTM(nss_info, out, N,
                                         noscore, onlyscore,
                                         c.pruneThreshold, c.maxStackSize,
                                         c.covLimit, c.covThreshold);
         else
            aligner.computePhraseTM(nss_info, out, N,
                                    noscore, onlyscore,
                                    c.pruneThreshold, c.maxStackSize,
                                    c.covLimit, c.covThreshold);

         out << flush;
      } // for
      i++;
   } // while
   if ( !c.loadFirst ) reader.reportWarningCounts();
} // main
