/**
 * @author George Foster
 * @file configtool.cc  Program that reads canoe config file and writes selected information to out.
 *
 * COMMENTS:  Operations on canoe config files, for use with rescoreloop.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"
#ifdef Darwin
#include <libgen.h>
#endif
#include <string.h>
#include "config_io.h"
#include "logging.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

namespace Portage {

/// config_tool's namespace to prevent global namespace pollution in doxygen.
namespace ConfigTool {

/// Program config_tool's usage.
static char help_message[] = "\n\
configtool [-vp] cmd [config [out]]\n\
\n\
Read canoe config file <config> and write selected information to <out>,\n\
depending on <cmd>, one of:\n\
\n\
  weights            - An argument-string specification of canoe weights\n\
                       corresponding to the contents of config.\n\
  dump               - all parameter settings\n\
  nf                 - total number of features\n\
  nb                 - number of basic feature functions\n\
  nl                 - number of language models\n\
  nt                 - number of translation models\n\
  nt-text            - number of single-prob text translation model files\n\
  nd                 - number of distortion models\n\
  segff              - does model contain a segmentation model ff?\n\
  ttable-file:i      - the ith pair of text phrase table names (backward\n\
                       forward)\n\
  ttable-limit       - value of the ttable-limit parameter\n\
  rep-ttable-limit:v - a copy of <config>, with ttable-limit value replaced by v\n\
  rep-ttable-files:s - a copy of <config>, with s appended to all phrasetable\n\
                       names\n\
  rep-ttable-files-local:s - a copy of <config>, with s appended to all\n\
                       phrasetable names, and their path stripped\n\
  filt-ttables:s     - a copy of <config>, with: s appended to backwards\n\
                       phrasetable names; forward phrasetable parameters\n\
                       removed; and ttable-limit set to 0.\n\
  filt-ttables-local:s - a copy of <config>, with: s appended to backwards\n\
                       phrasetable names; their path stripped; forward phrase-\n\
                       table parameters removed; and ttable-limit set to 0.\n\
  check              - check that all feature files can be read: write ok if so,\n\
                       otherwise list ones that can't\n\
  set-weights:rr     - A copy of <config>, with weights replaced by optimum\n\
                       (best BLEU) values from the 'rescore-results' file <rr>\n\
  arg-weights:rm     - An argument-string specification of canoe weights\n\
                       corresponding to the contents of the rescore-model file rm.\n\
  rescore-model:ff   - A rescoring model with entries of the form 'FileFF:ff,i w'\n\
                       for i = 1..N (num features in configfile), and w is the\n\
                       weight associated with the ith feature. Features are written\n\
                       in the same order than canoe writes them to an ffvals file.\n\
\n\
Options:\n\
\n\
-p  Pretty-print output config files [don't]\n\
";

// globals

static bool verbose = false;
static bool pretty = false;
static string cmd;
static string config_in = "-";
static ofstream ofs;
static ostream* osp = &cout;


/// Keeps track of the BLEU score associated with a weight vector
struct RescoreResult {
   double bleu;   ///< BLEU score
   string wtvec;  ///< weight vector
   /**
    * Constructor.
    * @param bleu   BLEU score associated with wtvec
    * @param wtvec  weight vector
    */
   RescoreResult(double bleu=0, const string& wtvec="") : bleu(bleu), wtvec(wtvec) {}
};

/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  vector of command line arguments
 */
static void getArgs(int argc, char* argv[]);

/**
 * Parses the rescore results and returns it.
 * line format is "bleu score: xxxx -x xxxx -x xxxx ..."
 * @param line  line to parse
 * @return Returns a RescoreResult which contains the BLEU score and the associated weights
 */
static RescoreResult parseRescoreResultsLine(const string& line);

} // ends namespace ConfigTool
} // ends namespace Portage
using namespace Portage::ConfigTool;



/**
 * Program configtool's entry point.
 * @param argc  number of command line arguments.
 * @param argv  vector containing the command line arguments.
 * @return Returns 0 if successful.
 */
int main(int argc, char* argv[])
{
   Logging::init();
   printCopyright(2005,"configtool");

   getArgs(argc, argv);
   ostream& os = *osp;
   os << setprecision(10);

   CanoeConfig c;
   c.read(config_in.c_str());
   c.check();

   vector<string> toks;
   Uint vi;

   if (cmd == "dump") {
      c.write(os,3,pretty);
   } else if (cmd == "nf") {
      vector<double> wts;
      c.getFeatureWeights(wts);
      os << wts.size() << endl;
   } else if (cmd == "nd") {
      os << c.distWeight.size() << endl;
   } else if (cmd == "segff") {
      os << c.segWeight.size() << endl;
   } else if (cmd == "nb") {
      int n = 1 // length is always a basic feature
            + c.segWeight.size()
            + c.distWeight.size();
      os << n << endl;
   } else if (cmd == "nl") {
      os << c.lmFiles.size() << endl;
   } else if (cmd == "nt") {
      os << (c.backPhraseFiles.size() + c.getTotalMultiProbModelCount())
         << endl;
   } else if (cmd == "nt-text") {
      os << c.backPhraseFiles.size() << endl;
   } else if (isPrefix("ttable-file:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2 || !conv(toks[1], vi))
         error(ETFatal, "bad format for ttable-file command");
      if (vi > c.backPhraseFiles.size() || vi == 0)
         error(ETFatal, "bad ttable-file index");
      os << c.backPhraseFiles[vi-1];
      if (c.forPhraseFiles.size() > vi-1) os << " " << c.forPhraseFiles[vi-1];
      os << endl;
   } else if (cmd == "ttable-limit") {
      os << c.phraseTableSizeLimit << endl;
   } else if (isPrefix("rep-ttable-limit:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2 || !conv(toks[1], vi))
         error(ETFatal, "bad format for rep-ttable-limit command");
      c.phraseTableSizeLimit = vi;
      c.write(os,0,pretty);
   } else if (isPrefix("rep-ttable-files:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-ttable-files command");
      for (Uint i = 0; i < c.backPhraseFiles.size(); ++i)
         c.backPhraseFiles[i] = addExtension(c.backPhraseFiles[i], toks[1]);
      for (Uint i = 0; i < c.forPhraseFiles.size(); ++i)
         c.forPhraseFiles[i] = addExtension(c.forPhraseFiles[i], toks[1]);
      for (Uint i = 0; i < c.multiProbTMFiles.size(); ++i)
         c.multiProbTMFiles[i] = addExtension(c.multiProbTMFiles[i], toks[1]);
      c.write(os,0,pretty);
   } else if (isPrefix("rep-ttable-files-local:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-ttable-files command");
      for (Uint i = 0; i < c.backPhraseFiles.size(); ++i)
         c.backPhraseFiles[i] = addExtension(BaseName(c.backPhraseFiles[i].c_str()), toks[1]);
      for (Uint i = 0; i < c.forPhraseFiles.size(); ++i)
         c.forPhraseFiles[i] = addExtension(BaseName(c.forPhraseFiles[i].c_str()), toks[1]);
      for (Uint i = 0; i < c.multiProbTMFiles.size(); ++i)
         c.multiProbTMFiles[i] = addExtension(BaseName(c.multiProbTMFiles[i].c_str()), toks[1]);
      c.write(os,0,pretty);
   } else if (isPrefix("filt-ttables:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for filt-ttables command");
      for (Uint i = 0; i < c.backPhraseFiles.size(); ++i)
         c.backPhraseFiles[i] = addExtension(c.backPhraseFiles[i], toks[1]);
      c.readStatus("ttable-file-t2s") = false;
      c.readStatus("ttable-limit") = true;
      c.phraseTableSizeLimit = 0;
      c.write(os,0,pretty);
   } else if (isPrefix("filt-ttables-local:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for filt-ttables command");
      for (Uint i = 0; i < c.backPhraseFiles.size(); ++i) {
         c.backPhraseFiles[i] = addExtension(BaseName(c.backPhraseFiles[i].c_str()), toks[1]);
      }
      c.readStatus("ttable-file-t2s") = false;
      c.readStatus("ttable-limit") = true;
      c.phraseTableSizeLimit = 0;
      c.write(os,0,pretty);
   } else if (isPrefix("set-weights:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for set-weights command");

      // get best line in rescore-results file
      iSafeMagicStream ifs(toks[1].c_str());
      string line;
      RescoreResult best_rr(-1);
      while (getline(ifs, line))
	 if (parseRescoreResultsLine(line).bleu > best_rr.bleu)
	    best_rr = parseRescoreResultsLine(line);
      if (best_rr.bleu == -1.0)
         error(ETFatal, "no bleu scores > -1 in rescore-results? I quit!");

      c.setFeatureWeightsFromString(best_rr.wtvec);
      c.write(os,0,pretty);
   } else if (cmd == "check") {
      c.check_all_files(); // dies with error if any files is not readable
      os << "ok" << endl;
   } else if (isPrefix("rescore-model:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rescore-model command");
      vector<double> wts;
      c.getFeatureWeights(wts);
      for (Uint i = 0; i < wts.size(); ++i)
         os << "FileFF:" << toks[1] << "," << i+1 << " " << wts[i] << endl;
   } else if (isPrefix("arg-weights:", cmd.c_str())) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for arg-weights command");
      vector<double> wts;
      iSafeMagicStream ifs(toks[1].c_str());
      string line;
      vector<string> ltoks;
      while (getline(ifs,line)) {
         if (splitZ(line, ltoks) != 2)
            error(ETFatal, "expecting only 2 tokens in rescore-model file %s", toks[1].c_str());
         wts.push_back(conv<double>(ltoks[1]));
      }
      c.setFeatureWeights(wts);
      os << c.getFeatureWeightString(line) << endl;
   } else if (cmd == "weights") {
      string line;
      os << c.getFeatureWeightString(line) << endl;
   } else
      error(ETFatal, "unknown command: %s", cmd.c_str());
}




namespace Portage {
namespace ConfigTool {
// parse rr line
RescoreResult parseRescoreResultsLine(const string& line)
{
   // line format is "bleu score: xxxx -x xxxx -x xxxx ..."

   vector<string> ltoks;
   split(line, ltoks, " \n\t", 4);
   if ( ltoks.size() < 4 || !(ltoks[0] == "BLEU" || ltoks[0] == "PER" || ltoks[0] == "WER") || ltoks[1] != "score:" ) {
      error(ETWarn, "Ignoring ill-formatted rescore result line: %s",
         line.c_str());
      return RescoreResult(0, "");
   } else {
      if (ltoks[0] == "BLEU") {
         return RescoreResult(conv<double>(ltoks[2]), ltoks[3]);
      }
      else {
         return RescoreResult(-conv<double>(ltoks[2]), ltoks[3]);
      }
   }
   
//    int word_wts_count = 0;
//    rr.seg_active = rr.dist_active = false;
//    rr.bleu = conv<double>(ltoks[2]);

//    int i;
//    for (i = 3; i < ltoks.size(); i++) {
//      if (ltoks[i] == "-d" && i+1 < ltoks.size()) {
//        rr.dist_wt = conv<double>(ltoks[++i]);
//        rr.dist_active = true;
//      } else if (ltoks[i] == "-sm" && i+1 < ltoks.size()) {
//        rr.seg_wt = conv<double>(ltoks[++i]);
//        rr.seg_active = true;
//      } else if (ltoks[i] == "-w" && i+1 < ltoks.size()) {
//        rr.word_wt = conv<double>(ltoks[++i]);
//        word_wts_count++;
//      } else if (ltoks[i] == "-lm" && i+1 < ltoks.size()) {
//        split(ltoks[++i], rr.lm_wts, ":");
//      } else if (ltoks[i] == "-tm" && i+1 < ltoks.size()) {
//        split(ltoks[++i], rr.tm_wts, ":");
//      }
//    }

//    if (word_wts_count != 1 || rr.lm_wts.size() < 1 || rr.tm_wts.size() < 1) {
//       error(ETFatal, "missing bits in rescore results line:", line.c_str());
//    }

//   return rr;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "p"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("p", pretty);

   arg_reader.testAndSet(0, "cmd", cmd);
   arg_reader.testAndSet(1, "config", config_in);
   arg_reader.testAndSet(2, "out", &osp, ofs);
}
}
}
