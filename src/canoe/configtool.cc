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
#include "bilm_model.h"

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
  rep-sparse-models-local:s - a copy of <config>, with any sparse model names\n\
                       converted to local symlinked versions with s appended.\n\
                       Eg: [sparse-model] ../../d/m -> [sparse-model] d.<s>/m\n\
                       (with ln -s ../../d d.<s> performed automatically).\n\
  rep-sparse-models-suff:s - a copy of <config>, with any sparse model names\n\
                       replaced by the same name but with <s> appended.\n\
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
  get-all-wts:x      - A listing of all weights in the model, in the form 'i w'.\n\
                       If <config> contains no SparseModels, output is identical\n\
                       to rescore-model:ff with 'FileFF:ff,' prefixes omitted and\n\
                       0-based indexing. Otherwise, wts for decoder SparseModel\n\
                       features are set to 0, and SparseModel component wts are\n\
                       appended. The ith wt matches the ith feature in -sfvals\n\
                       output from canoe.\n\
  set-all-wts:sw     - A copy of <config>, with weights replaced by those in\n\
                       <sw>, which is in the format written by get-all-wts.\n\
                       This also replaces weights in any SparseModels referred\n\
                       to by <config>, and saves results to disk.\n\
  rule:<file|->      - List all rule classes from <file|->.\n\
  rep-multi-prob:cpt - Replace all multiprobs with cpt.\n\
  rep-ldm:ldm        - Replace all ldms with ldm.\n\
  applied-weights:tppt:w-tm:w-ftm\n\
                     - Change the forward and backward weights to w-tm and w-ftm\n\
                       respectively and replaces multi-probs for tppt.\n\
  tp                 - Change multiprobs, language models and lexicalized\n\
                       distortion models to their tightly packed version for\n\
                       portageLive.\n\
  list-lm            - List language model file names.\n\
  list-bilm          - List bilm language model file names.\n\
  list-ldm           - List lexicalized distortion model file names.\n\
  list-tm            - List all translation model file names.\n\
  list-all-files     - List all files used by this canoe.ini.\n\
                       Intended for deep copying of models, e.g.:\n\
                       configtool list-all-files canoe.ini.cow > list\n\
                       rsync -aLr --files-from=list --no-implied-dirs . DESTINATION_DIR\n\
  args:<args>        - Apply canoe command-line arguments <args> to <config>, and\n\
                       write resulting new configuration.\n\
  prime_partial      - Quickly loads a minimal set of tpt in memory\n\
  prime_full         - Quickly loads all tpt in memory\n\
\n\
Options:\n\
\n\
-p  Pretty-print output config files [don't]\n\
-c  Skip integrity check on input config file [check]\n\
-l  When using one of list-{lm,bilm,ldm,tm}, display one model per line [don't]\n\
";

// globals

static bool verbose = false;
static bool pretty = false;
static const char * separator = " ";
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
      os << c.featureGroup("d")->size() << endl;
   } else if (cmd == "segff") {
      os << c.featureGroup("sm")->size() << endl;
   } else if (cmd == "memmap") {
      Uint64 total_memmap_size = 0;
      for ( Uint i = 0; i < c.tpptFiles.size(); ++i )
         total_memmap_size += TPPT::totalMemmapSize(c.tpptFiles[i]);
      for ( Uint i = 0; i < c.lmFiles.size(); ++i )
         total_memmap_size += PLM::totalMemmapSize(c.lmFiles[i]);
      for ( Uint i = 0; i < c.LDMFiles.size(); ++i ) {
         if (isSuffix(".tpldm", c.LDMFiles[i])) {
            total_memmap_size += TPPT::totalMemmapSize(c.LDMFiles[i]);
         }
      }
      for (CanoeConfig::FeatureGroupMap::const_iterator it(c.features.begin()),
           end(c.features.end()); it != end; ++it) {
         const CanoeConfig::FeatureGroup *f = it->second;
         for (Uint i(0); i < f->size(); ++i) {
            const string args = f->need_args ? f->args[i] : "";
            total_memmap_size += DecoderFeature::totalMemmapSize(f->group, args);
         }
      }
      os << (total_memmap_size/1024/1024) << endl;
   } else if (cmd == "nb") {
      int n = 1 // length is always a basic feature
            + c.featureGroup("d")->size()
            + c.featureGroup("sm")->size();
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
   } else if (isPrefix("rep-sparse-models-local:", cmd)) { // this is pretty hacky
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-sparse-models-local command");
      vector<string>& sparseModels(c.featureGroup("sparse")->args);
      for (Uint i = 0; i < sparseModels.size(); ++i) {
         string dir = DirName(sparseModels[i]);
         string linkname = BaseName(dir);
         if (linkname == "" || linkname == ".") linkname = toks[1];
         else linkname += toks[1];
         string cmd = "ln -s " + dir + " " + linkname;
         if (check_if_exists(linkname))
            error(ETWarn, "%s exists already - won't create", linkname.c_str());
         else if (system(cmd.c_str()) != 0)
            error(ETFatal, "can't run command: " + cmd);
         sparseModels[i] = linkname + "/" + BaseName(sparseModels[i]);
      }
      c.write(os, 0, pretty);
   } else if (isPrefix("rep-sparse-models-suff:", cmd)) { // this is pretty hacky
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-sparse-models-suff command");
      vector<string>& sparseModels(c.featureGroup("sparse")->args);
      for (Uint i = 0; i < sparseModels.size(); ++i)
         sparseModels[i] += toks[1];
      c.write(os, 0, pretty);
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
   } else if (isPrefix("get-all-wts:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for get-all-wts command");
      vector <double> wts;
      c.getSparseFeatureWeights(wts);
      for (Uint i = 0; i < wts.size(); ++i)
         os << i << ' ' << wts[i] << endl;
   } else if (isPrefix("set-all-wts:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for set-all-wts command");
      vector <double> wts;
      iMagicStream ifs(toks[1].c_str());
      string line;
      vector<string> ltoks;
      while (getline(ifs,line)) {
         if (splitZ(line, ltoks) != 2)
            error(ETFatal, "expecting only 2 tokens in sparse-weights file %s",
                  toks[1].c_str());
         wts.push_back(conv<double>(ltoks[1]));
      }
      c.setSparseFeatureWeights(wts);
      c.write(os, 0, pretty);
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

      vector<string> rule_classes_names;
      iSafeMagicStream input(toks[1]);
      InputParser reader(input, c.bLoadBalancing);

      // Process all input sentences
      // All we care about are the rule classes.
      while (reader.getMarkedSent(&rule_classes_names))
         ;

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
      } else {
         c.multiProbTMFiles.push_back(cpt);
      }
      c.write(os, 0, pretty);
   } else if (isPrefix("rep-ldm:", cmd)) {
      if (split(cmd, toks, ":") != 2)
         error(ETFatal, "bad format for rep-ldm command");
      const string& ldm = toks[1];
      c.LDMFiles.clear();
      c.LDMFiles.push_back(ldm);
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
      cout << join(c.lmFiles, separator) << endl;
   } else if (isPrefix("list-bilm", cmd)) {
      cout << join(c.featureGroup("bilm")->args, separator) << endl;
   } else if (isPrefix("list-ldm", cmd)) {
      cout << join(c.LDMFiles, separator) << endl;
   } else if (isPrefix("list-tm", cmd)) {
      vector<string> alltms(c.multiProbTMFiles);
      alltms.insert(alltms.end(), c.tpptFiles.begin(), c.tpptFiles.end());
      cout << join(alltms, separator) << endl;
   } else if (isPrefix("list-all-files", cmd)) {
      // arghhh. I (EJ) wrote this, but it's gross.  I should use the ParamInfo
      // flags to determine which things to print here, not have specific code
      // here knowing what to print.  Maybe I should piggy back on ::check()
      // for everything, the way I do with PLM::checkFileExists() below...
      bool ok = true;
      cout << config_in << nf_endl;
      // LMs
      vector<string> lmlist;
      for (Uint i = 0; i < c.lmFiles.size(); ++i)
         PLM::checkFileExists(c.lmFiles[i], &lmlist) || (ok = false);
      cout << join(lmlist, nf_endl) << nf_endl;
      // (H)LDMs
      for (Uint i = 0; i < c.LDMFiles.size(); ++i) {
         cout << c.LDMFiles[i] << nf_endl;
         if (!isSuffix(".tpldm", c.LDMFiles[i]))
            cout << removeZipExtension(c.LDMFiles[i]) << ".bkoff" << nf_endl;
      }
      // BILMs
      vector<string>& bilm_args = c.featureGroup("bilm")->args;
      if (!bilm_args.empty()) {
         vector<string> bilmlist;
         for (Uint i = 0; i < bilm_args.size(); ++i)
            BiLMModel::checkFileExists(bilm_args[i], &bilmlist) || (ok = false);
         cout << join(bilmlist, nf_endl) << nf_endl;
      }
      // Other files
      vector<string> otherfiles;
      otherfiles.insert(otherfiles.end(), c.multiProbTMFiles.begin(), c.multiProbTMFiles.end());
      otherfiles.insert(otherfiles.end(), c.tpptFiles.begin(), c.tpptFiles.end());
      otherfiles.insert(otherfiles.end(), c.featureGroup("sparse")->args.begin(), c.featureGroup("sparse")->args.end());
      otherfiles.insert(otherfiles.end(), c.featureGroup("ibm1f")->args.begin(), c.featureGroup("ibm1f")->args.end());
      otherfiles.insert(otherfiles.end(), c.featureGroup("nnjm")->args.begin(), c.featureGroup("nnjm")->args.end());
      cout << join(otherfiles, nf_endl) << nf_endl;
      if (!c.sentWeights.empty()) cout << c.sentWeights << nf_endl;
      if (!c.srctags.empty()) cout << c.srctags << nf_endl;
      if (!c.refFile.empty()) cout << c.refFile << nf_endl;
      if (!c.featureGroup("sparse")->args.empty()) {
         error(ETWarn, "list-all-files does not yet know how to list sparse model components");
         ok = false;
      }
      if (!ok)
         error(ETFatal, "There was some problem, the list is probably incomplete");
   } else if (cmd == "prime_partial" or cmd == "prime_full") {
      c.prime(cmd == "prime_full");
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
   const char* switches[] = {"v", "p", "c", "l"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("p", pretty);
   if (arg_reader.getSwitch("l"))
      separator = "\n";

   arg_reader.testAndSet(0, "cmd", cmd);
   arg_reader.testAndSet(1, "config", config_in);
   arg_reader.testAndSet(2, "out", &osp, ofs);
}

}  // ends namespace ConfigTool
}  // ends namespace Portage
