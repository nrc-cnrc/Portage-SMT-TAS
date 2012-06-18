/**
 * @author George Foster
 * @file configtool.cc 
 * @brief Program that reads canoe config file and writes selected information
 * to out.
 *
 * COMMENTS:  Operations on canoe config files, for use with rescoreloop.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "new_src_sent_info.h"
#include "inputparser.h"
#include "str_utils.h"
#include <fstream>
#include "arg_reader.h"
#ifdef Darwin
#include <libgen.h>
#endif
#include "config_io.h"
#include "logging.h"
#include "lm.h"
#include "tppt.h"
#include "basicmodel.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

namespace Portage {

/// config_tool's namespace to prevent global namespace pollution in doxygen.
namespace ConfigTool {

/// Program config_tool's usage.
static char help_message[] = "\n\
configtool [-vpc] cmd [config [out]]\n\
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
  na                 - number of adirectional translation models\n\
  nt-tppt            - number of TPPT translation models\n\
  nd                 - number of distortion models\n\
  segff              - does model contain a segmentation model ff?\n\
  ttable-limit       - value of the ttable-limit parameter\n\
  memmap             - the total size of mem mapped models in MBs\n\
  rep-ttable-limit:v - a copy of <config>, with ttable-limit value replaced by v\n\
  rep-ttable-files:s - a copy of <config>, with s appended to all phrasetable\n\
                       names\n\
  rep-ttable-files-local:s - a copy of <config>, with s appended to all\n\
                       phrasetable names, and their path stripped\n\
  check              - check that all feature files can be read: write ok if\n\
                       so, otherwise list ones that can't\n\
  set-weights:rr     - A copy of <config>, with weights replaced by optimum\n\
                       (best BLEU) values from the 'rescore-results' file <rr>\n\
  set-weights-rm:rm  - A copy of <config>, with weights replaced by corresponding\n\
                       weights from rescoring model <rm>.\n\
  arg-weights:rm     - An argument-string specification of canoe weights\n\
                       corresponding to the contents of the rescore-model file\n\
                       rm.\n\
  rescore-model:ff   - A rescoring model with entries of the form\n\
                       'FileFF:ff,i w' for i = 1..N (num features in\n\
                       configfile), and w is the weight associated with the\n\
                       ith feature. Features are written in the same order\n\
                       that canoe writes them to an ffvals file.\n\
  rule:<file|->      - List all rule classes from <file|->.\n\
  rep-multi-prob:cpt - Replace all multiprobs with cpt.\n\
  applied-weights:tppt:w-tm:w-ftm\n\
                     - Change the forward and backward weights to w-tm and w-ftm\n\
                       respectively and replaces multi-probs for tppt.\n\
  tp                 - Change multiprobs, language models and lexicalized\n\
                       distortion models to their tightly packed version for\n\
                       portageLive.\n\
  list-lm            - List language model file names.\n\
  list-ldm           - List lexicalized distortion model file names.\n\
  list-tm            - List all translation model file names.\n\
  args:<args>        - Apply canoe command-line arguments <args> to <config>, and\n\
                       write resulting new configuration.\n\
\n\
Options:\n\
\n\
-p  Pretty-print output config files [don't]\n\
-c  Skip integrity check on input config file [check]\n\
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
   //printCopyright(2005,"configtool");

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
      os << c.feature("d")->size() << endl;
   } else if (cmd == "segff") {
      os << c.feature("sm")->size() << endl;
   } else if (cmd == "memmap") {
      Uint64 total_memmap_size = 0;
      for ( Uint i = 0; i < c.tpptFiles.size(); ++i )
         total_memmap_size += TPPT::totalMemmapSize(c.tpptFiles[i]);
      for ( Uint i = 0; i < c.lmFiles.size(); ++i )
         total_memmap_size += PLM::totalMemmapSize(c.lmFiles[i]);
      os << (total_memmap_size/1024/1024) << endl;
   } else if (cmd == "nb") {
      int n = 1 // length is always a basic feature
            + c.feature("d")->size()
            + c.feature("sm")->size();
      os << n << endl;
   } else if (cmd == "nl") {
      os << c.lmFiles.size() << endl;
   } else if (cmd == "nt") {
      os << (c.getTotalMultiProbModelCount() +
             c.getTotalTPPTModelCount()) << endl;
   } else if (cmd == "na") {
      os << c.getTotalAdirectionalModelCount() << endl;
   } else if (cmd == "nt-tppt") {
      os << c.getTotalTPPTModelCount() << endl;
   } else if (cmd == "ttable-limit") {
      os << c.phraseTableSizeLimit << endl;
   } else if (isPrefix("rep-ttable-limit:", cmd)) {
      if (split(cmd, toks, ":") != 2 || !conv(toks[1], vi))
         error(ETFatal, "bad format for rep-ttable-limit command");
      c.phraseTableSizeLimit = vi;
      c.write(os,0,pretty);
   } else if (isPrefix("rep-ttable-files:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-ttable-files command");
      for (Uint i = 0; i < c.multiProbTMFiles.size(); ++i)
         c.multiProbTMFiles[i] = addExtension(c.multiProbTMFiles[i], toks[1]);
      c.write(os,0,pretty);
   } else if (isPrefix("rep-ttable-files-local:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-ttable-files command");
      for (Uint i = 0; i < c.multiProbTMFiles.size(); ++i)
         c.multiProbTMFiles[i] = addExtension(BaseName(c.multiProbTMFiles[i].c_str()), toks[1]);
      c.write(os,0,pretty);
   } else if (isPrefix("set-weights:", cmd)) {
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
      //cerr << "BMG constructor" << endl;
      BasicModelGenerator bmg(c); // dies with error if any simple model is bad
      os << "ok" << endl;
   } else if (isPrefix("rescore-model:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rescore-model command");
      vector<double> wts;
      c.getFeatureWeights(wts);
      for (Uint i = 0; i < wts.size(); ++i)
         os << "FileFF:" << toks[1] << "," << i+1 << " " << wts[i] << endl;
   } else if (isPrefix("arg-weights:", cmd)) {
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
   } else if (isPrefix("set-weights-rm:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for set-weights-rm command");
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
      c.write(os,0,pretty);
   } else if (isPrefix("args:", cmd)) {
      if (split(cmd, toks, ":", 2) != 2) 
         error(ETFatal, "bad format for args command");
      vector<string> params = c.getParamList();
      const char* switches[params.size()];
      for (Uint i = 0; i < params.size(); ++i)
         switches[i] = params[i].c_str();
      ArgReader r(ARRAY_SIZE(switches), switches, 0, 0, "", "-h", false);
      r.read(toks[1]);
      c.setFromArgReader(r);
      c.write(os,1,pretty);
   } else if (cmd == "weights") {
      string line;
      os << c.getFeatureWeightString(line) << endl;
   } else if (isPrefix("rule:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rule command");

      newSrcSentInfo nss_info;
      vector<string> rule_classes_names;
      iSafeMagicStream input(toks[1]);
      InputParser reader(input, c.bLoadBalancing);
                     
      // Process all input sentences
      while (!(reader.eof() && nss_info.src_sent.empty())) {
         // All we care about are the rule classes.
         // We can reuse the same new source sentence info.
         nss_info.clear();
         if ( ! reader.readMarkedSent(nss_info.src_sent,
               nss_info.marks,
               &rule_classes_names,
               &nss_info.external_src_sent_id) )
         {
            if ( c.tolerateMarkupErrors )
               error(ETWarn, "Tolerating ill-formed markup.  Source sentence "
                     "%d will be interpreted as having %d valid mark%s and "
                     "this token sequence: %s",
                     nss_info.external_src_sent_id, nss_info.marks.size(),
                     (nss_info.marks.size() == 1 ? "" : "s"),
                     join(nss_info.src_sent).c_str());
            else
               error(ETFatal, "Aborting because of ill-formed markup");
         }
      }

      // Print the rule's info if any class was found
      if (!rule_classes_names.empty()) {
         // Create a default weight vector.
         vector<float> weights(rule_classes_names.size(), 1.0f);

         cout << "[rule-classes] " << join(rule_classes_names, ":") << endl;
         cout << "[rule-weights] " << join(weights, ":") << endl;
      }
   } else if (isPrefix("list-multi-probs", cmd)) {
      cout << join(c.multiProbTMFiles) << endl;
   } else if (isPrefix("rep-multi-prob:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-multi-prob command");

      const string& cpt = toks[1];
      c.multiProbTMFiles.clear();
      if (isSuffix(".tppt", cpt)) {
         c.readStatus("ttable-multi-prob") = false;
         c.readStatus("ttable-tppt") = true;
         c.tpptFiles.push_back(cpt);
      }
      else {
         c.multiProbTMFiles.push_back(cpt);
      }

      c.write(os, 0, pretty);
   } else if (isPrefix("applied-weights", cmd)) {
      if (split(cmd, toks, ":") != 4)
         error(ETFatal, "bad format for applied-weights command");

      const string& cpt = toks[1];
      c.multiProbTMFiles.clear();
      if (isSuffix(".tppt", cpt)) {
         c.readStatus("ttable-multi-prob") = false;
         c.readStatus("ttable-tppt") = true;
         c.tpptFiles.push_back(cpt);
      }
      else {
         c.multiProbTMFiles.push_back(cpt);
      }

      c.transWeights.clear();
      c.transWeights.push_back(conv<double>(toks[2]));
      c.forwardWeights.clear();
      c.forwardWeights.push_back(conv<double>(toks[3]));

      c.write(os, 0, pretty);
   } else if (isPrefix("tp", cmd)) {
      // Translation models.
      for (Uint i=0; i<c.multiProbTMFiles.size(); ++i) {
         c.tpptFiles.push_back(removeZipExtension(c.multiProbTMFiles[i]) + ".tppt");
      }

      c.multiProbTMFiles.clear();
      c.readStatus("ttable-multi-prob") = false;
      if (!c.tpptFiles.empty()) c.readStatus("ttable-tppt") = true;

      // Lexicalized Distortion Models.
      for (Uint i=0; i<c.LDMFiles.size(); ++i) {
         c.LDMFiles[i] = removeZipExtension(c.LDMFiles[i]) + ".tpldm";
      }

      // Language Models.
      for (Uint i=0; i<c.lmFiles.size(); ++i) {
         if (isSuffix(".binlm.gz", c.lmFiles[i])) {
            c.lmFiles[i] = c.lmFiles[i].substr(0, c.lmFiles[i].size()-strlen(".binlm.gz")) + ".tplm";
         }
      }

      c.write(os, 0, pretty);
   } else if (isPrefix("list-lm", cmd)) {
      cout << join(c.lmFiles) << endl;
   } else if (isPrefix("list-ldm", cmd)) {
      cout << join(c.LDMFiles) << endl;
   } else if (isPrefix("list-tm", cmd)) {
      vector<string> alltms(c.multiProbTMFiles);
      alltms.insert(alltms.end(), c.tpptFiles.begin(), c.tpptFiles.end());
      cout << join(alltms) << endl;
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
   const char* switches[] = {"v", "p", "c"};
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
