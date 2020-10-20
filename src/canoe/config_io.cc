/**
 * @author George Foster
 * @file config_io.cc  Implementation of CanoeConfig.
 *
 * Read and write canoe config files
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "config_io.h"
#include "arg_reader.h"
#include "str_utils.h"
#include "file_utils.h"
#include "phrasetable.h"
#include <iomanip>
#include <sstream>
#include "logging.h"
#include "randomDistribution.h"
#include "lm.h"
#include "lmdynmap.h"
#include "bilm_model.h"
#include "tppt.h"
#include "sparsemodel.h"
#include "multiprob_pt_feature.h"
#include "tppt_feature.h"
#include "nnjm.h"
#include <boost/version.hpp>
#if BOOST_VERSION >= 103800
   #include <boost/spirit/include/classic.hpp>
   #include <boost/spirit/include/classic_assign_actor.hpp>
   using namespace boost::spirit::classic;
#else
   #include <boost/spirit.hpp>
   #include <boost/spirit/actor/assign_actor.hpp>
   using namespace boost::spirit;
#endif
#include <boost/bind.hpp>
#include <sys/file.h>

using namespace Portage;

/**
 * Grammar to parse and create the proper random distribution from a canoe.ini file.
 * Valid:  U(real,real) | N(real,real) | real
 */
struct distribution_grammar : public grammar<distribution_grammar>
{
   mutable rnd_distribution*  rnd;  ///< Holds the rand_dist parsed from the string representation
   mutable double       first;      ///< Temp placeholder for random distribution parameters
   mutable double       second;     ///< Temp placeholder for random distribution parameters

   /// Default constructor.
   distribution_grammar()
   : rnd(NULL)
   , first(0.0f)
   , second(0.0f)
   {}

   /// Creates a uniform distribution.
   void make_uniform(char const*, char const*) const{
      //cerr << "Creating uniform" << endl;   // For DEBUGGING
      rnd = new uniform(first, second);
   }

   /// Creates a normal distribution
   void make_normal(char const*, char const*) const{
      //cerr << "Creating normal" << endl;    // For DEBUGGING
      rnd = new normal(first, second);
   }

   /// Creates a constant distribution
   void make_constant(double value) const{
      //cerr << "Creating fix" << endl;     // For DEBUGGING
      rnd = new constant_distribution(value);
   }

   /// Actual grammar's definition
   template <typename ScannerT>
   struct definition
   {
      /// Default constructor.
      /// @param self  grammar object to be able to refer to it if needs be.
      definition(distribution_grammar const& self)
      {
         random   = normal | uniform | fix;
         fix      = real_p[bind(&distribution_grammar::make_constant, &self, _1)];
         normal   = ('N' >> values)[bind(&distribution_grammar::make_normal, &self, _1, _2)];
         uniform  = ('U' >> values)[bind(&distribution_grammar::make_uniform, &self, _1, _2)];
         values   = '(' >> real_p[assign_a(self.first)] >> ',' >> real_p[assign_a(self.second)] >> ')';
      }
      /// Some rules placeholders
      rule<ScannerT>  random, normal, uniform, values, fix;
      /// Default starting point.
      rule<ScannerT> const& start() const { return random; };
   };
};



CanoeConfig::CanoeConfig()
{

   // Default parameter values. WARNING: changing these has impacts on
   // canoe.cc.  Change with care, and consider whether you change should be
   // global (here) or local (there).

   //configFile;
   //multiProbTMFiles;
   //tpptFiles;
   //TTables;
   //LDMFiles;
   //lmFiles;
   lmOrder                = 0;
   maxLmContextSize       = -1;
   minimizeLmContextSize  = false;
   //lmWeights;
   //transWeights;
   //forwardWeights;
   //adirTransWeights;
   randomWeights          = false;
   randomSeed             = 0;
   sentWeights            = "";
   phraseTableSizeLimit   = NO_SIZE_LIMIT;
   phraseTableThreshold   = 0;
   phraseTablePruneType   = "forward-weights";
   phraseTableLogZero     = LOG_ALMOST_0;
   maxStackSize           = 10000;
   maxRegularStackSize    = 1000;
   pruneThreshold         = 0.0001;
   covLimit               = 0;
   covThreshold           = 0.0;
   diversity              = 0;
   diversityStackIncrement = -1;
   levLimit               = NO_MAX_LEVENSHTEIN;
   distLimit              = NO_MAX_DISTORTION;
   itgLimit               = NO_MAX_ITG;
   distLimitExt           = false;
   distLimitSimple        = false;
   distPhraseSwap         = false;
   distLimitITG           = false;
   forceShiftReduce       = false;
   shiftReduceOnlyITG     = false;
   bypassMarked           = false;
   weightMarked           = 1;
   nosent                 = false;
   appendJointCounts      = false;
   oov                    = "pass";
   tolerateMarkupErrors   = false;
   checkInputOnly         = false;
   describeModelOnly      = false;
   trace                  = false;
   walign                 = false;
   ffvals                 = false;
   sfvals                 = false;
   sparseModelAllowNonLocalWts = false;
   masse                  = false;
   verbosity              = 1;
   quietEmptyLines        = false;
   latticeFilePrefix      = "";
   latticeOut             = false;
   latticeOutputOptions   = "carmel";
   latticeDensity         = BIG_LAT_DENSITY;
   latticeMinEdge         = SMALL_MIN_EDGE;
   latticeLogProb         = false;
   latticeSourceDensity   = false;
   nbestFilePrefix        = "";
   nbestSize              = 100;
   nbestOut               = false;
   firstSentNum           = 0;
   backwards              = false;
   loadFirst              = false;
   canoeDaemon            = "";
   input                  = "-";
   vocFile                = "";
   bAppendOutput          = false;
   bLoadBalancing         = false;
   bStackDecoding         = false;
   bCubePruning           = false;
   cubeLMHeuristic        = "incremental";
   futLMHeuristic         = "incremental";
   useFtm                 = false;
   hierarchy              = false;
   forcedDecoding         = false;
   forcedDecodingNZ       = false;
   maxlen                 = 0;
   final_cleanup          = false;  // for speed reason we don't normally delete the bmg
   bind_pid               = -1;
   timing                 = false;
   need_lock              = false;

   // Parameter information, used for input and output. NB: doesn't necessarily
   // correspond 1-1 with actual parameters, as one ParamInfo can set several
   // parameters:

   param_infos.push_back(ParamInfo("config f", "string", &configFile));
   param_infos.push_back(ParamInfo("ttable-multi-prob", "stringVect", &multiProbTMFiles,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("ttable-tppt", "stringVect", &tpptFiles,
      ParamInfo::relative_path_modification | ParamInfo::ttable_check_file_name));
   param_infos.push_back(ParamInfo("ttable", "stringVect", &TTables,
      ParamInfo::relative_path_modification | ParamInfo::ttable_check_file_name));
   param_infos.push_back(ParamInfo("lex-dist-model-file", "stringVect", &LDMFiles,
      ParamInfo::relative_path_modification | ParamInfo::ldm_check_file_name));
   param_infos.push_back(ParamInfo("lmodel-file", "stringVect", &lmFiles,
      ParamInfo::relative_path_modification | ParamInfo::lm_check_file_name));

   param_infos.push_back(ParamInfo("nbestProcessor", "string", &nbestProcessor));
   param_infos.push_back(ParamInfo("Voc-file", "string", &vocFile));
   param_infos.push_back(ParamInfo("lmodel-order", "Uint", &lmOrder));
   param_infos.push_back(ParamInfo("max-lm-context-size", "int", &maxLmContextSize));
   param_infos.push_back(ParamInfo("minimize-lm-context-size", "bool", &minimizeLmContextSize));

   // WEIGHTS of primary features
   param_infos.push_back(ParamInfo("weight-l lm", "doubleVect", &lmWeights));
   param_infos.push_back(ParamInfo("random-l rlm", "stringVect", &rnd_lmWeights));
   param_infos.push_back(ParamInfo("weight-t tm", "doubleVect", &transWeights));
   param_infos.push_back(ParamInfo("random-t rtm", "stringVect", &rnd_transWeights));
   param_infos.push_back(ParamInfo("weight-f ftm", "doubleVect", &forwardWeights));
   param_infos.push_back(ParamInfo("random-f rftm", "stringVect", &rnd_forwardWeights));
   param_infos.push_back(ParamInfo("weight-a atm", "doubleVect", &adirTransWeights)); //boxing
   param_infos.push_back(ParamInfo("random-a ratm", "stringVect", &rnd_adirTransWeights)); //boxing

   param_infos.push_back(ParamInfo("rule-classes ruc", "stringVect", &rule_classes));
   param_infos.push_back(ParamInfo("rule-log-zero rulz", "doubleVect", &rule_log_zero));

   param_infos.push_back(ParamInfo("random-weights r", "bool", &randomWeights));
   param_infos.push_back(ParamInfo("seed", "Uint", &randomSeed));
   param_infos.push_back(ParamInfo("sent-weights", "string", &sentWeights,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));

   param_infos.push_back(ParamInfo("ttable-limit", "Uint", &phraseTableSizeLimit));
   param_infos.push_back(ParamInfo("ttable-threshold", "double", &phraseTableThreshold));
   param_infos.push_back(ParamInfo("ttable-prune-type", "string", &phraseTablePruneType));
   param_infos.push_back(ParamInfo("ttable-log-zero", "double", &phraseTableLogZero));
   param_infos.push_back(ParamInfo("regular-stack rs", "Uint", &maxRegularStackSize));
   param_infos.push_back(ParamInfo("stack s", "Uint", &maxStackSize));
   param_infos.push_back(ParamInfo("beam-threshold b", "double", &pruneThreshold));
   param_infos.push_back(ParamInfo("cov-limit", "Uint", &covLimit));
   param_infos.push_back(ParamInfo("cov-threshold", "double", &covThreshold));
   param_infos.push_back(ParamInfo("diversity", "Uint", &diversity));
   param_infos.push_back(ParamInfo("diversity-stack-increment", "int", &diversityStackIncrement));
   param_infos.push_back(ParamInfo("levenshtein-limit", "int", &levLimit));
   param_infos.push_back(ParamInfo("distortion-limit", "int", &distLimit));
   param_infos.push_back(ParamInfo("itg-limit", "int", &itgLimit));
   param_infos.push_back(ParamInfo("dist-limit-ext", "bool", &distLimitExt));
   param_infos.push_back(ParamInfo("dist-limit-simple", "bool", &distLimitSimple));
   param_infos.push_back(ParamInfo("dist-phrase-swap", "bool", &distPhraseSwap));
   param_infos.push_back(ParamInfo("dist-limit-itg", "bool", &distLimitITG));
   param_infos.push_back(ParamInfo("shift-reduce-only-itg", "bool", &shiftReduceOnlyITG));
   param_infos.push_back(ParamInfo("force-shift-reduce", "bool",  & forceShiftReduce));
   param_infos.push_back(ParamInfo("filter-features", "stringVect", &filterFeatures));
   param_infos.push_back(ParamInfo("bypass-marked", "bool", &bypassMarked));
   param_infos.push_back(ParamInfo("weight-marked", "double", &weightMarked));
   param_infos.push_back(ParamInfo("nosent", "bool", &nosent));
   param_infos.push_back(ParamInfo("append-joint-counts", "bool", &appendJointCounts));
   param_infos.push_back(ParamInfo("oov", "string", &oov));
   param_infos.push_back(ParamInfo("tolerate-markup-errors", "bool", &tolerateMarkupErrors));
   param_infos.push_back(ParamInfo("check-input-only", "bool", &checkInputOnly));
   param_infos.push_back(ParamInfo("describe-model-only", "bool", &describeModelOnly));
   param_infos.push_back(ParamInfo("trace t palign", "bool", &trace));
   param_infos.push_back(ParamInfo("walign", "bool", &walign));
   param_infos.push_back(ParamInfo("ffvals", "bool", &ffvals));
   param_infos.push_back(ParamInfo("sfvals", "bool", &sfvals));
   param_infos.push_back(ParamInfo("sparse-model-allow-non-local-wts", "bool", &sparseModelAllowNonLocalWts));
   param_infos.push_back(ParamInfo("masse", "bool", &masse));
   param_infos.push_back(ParamInfo("verbose v", "Uint", &verbosity));
   param_infos.push_back(ParamInfo("quiet-empty-lines", "bool", &quietEmptyLines));
   param_infos.push_back(ParamInfo("lattice", "lat", &latticeFilePrefix));
   param_infos.push_back(ParamInfo("lattice-output-options", "string", &latticeOutputOptions));
   param_infos.push_back(ParamInfo("lattice-density", "double", &latticeDensity));
   param_infos.push_back(ParamInfo("lattice-min-edge", "double", &latticeMinEdge));
   param_infos.push_back(ParamInfo("lattice-log-prob", "bool", &latticeLogProb));
   param_infos.push_back(ParamInfo("lattice-source-density", "bool", &latticeSourceDensity));
   param_infos.push_back(ParamInfo("nbest", "nb", &nbestFilePrefix));
   param_infos.push_back(ParamInfo("first-sentnum", "Uint", &firstSentNum));
   param_infos.push_back(ParamInfo("backwards", "bool", &backwards));
   param_infos.push_back(ParamInfo("load-first", "bool", &loadFirst));
   param_infos.push_back(ParamInfo("canoe-daemon", "string", &canoeDaemon));
   param_infos.push_back(ParamInfo("input", "string", &input));
   param_infos.push_back(ParamInfo("append", "bool", &bAppendOutput));
   param_infos.push_back(ParamInfo("stack-decoding", "bool", &bStackDecoding));
   param_infos.push_back(ParamInfo("cube-pruning", "bool", &bCubePruning));
   param_infos.push_back(ParamInfo("cube-lm-heuristic", "string", &cubeLMHeuristic));
   param_infos.push_back(ParamInfo("future-score-lm-heuristic", "string", &futLMHeuristic));
   param_infos.push_back(ParamInfo("use-ftm", "bool", &useFtm));
   param_infos.push_back(ParamInfo("lb", "bool", &bLoadBalancing));
   param_infos.push_back(ParamInfo("srctags", "string", &srctags,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("ref", "string", &refFile,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("hierarchy", "bool", &hierarchy));
   param_infos.push_back(ParamInfo("forced", "bool", &forcedDecoding));
   param_infos.push_back(ParamInfo("forced-nz", "bool", &forcedDecodingNZ));
   param_infos.push_back(ParamInfo("maxlen", "Uint", &maxlen));
   param_infos.push_back(ParamInfo("nssiFilename", "string", &nssiFilename));
   param_infos.push_back(ParamInfo("final-cleanup", "bool", &final_cleanup));
   param_infos.push_back(ParamInfo("bind", "int", &bind_pid));
   param_infos.push_back(ParamInfo("timing", "bool", &timing));
   param_infos.push_back(ParamInfo("triangularArrayFilename", "string", &triangularArrayFilename));
   param_infos.push_back(ParamInfo("lock", "bool", &need_lock));



   // List of all parameters that correspond to weights. ORDER IS SIGNIFICANT
   // and must match the order in BasicModelGenerator::InitDecoderFeatures().
   // New entries should be added immediately before "lm".
   const char* weight_names_other[] = {
      // "short name", "long name", "group name",
      "d", "d", "DistortionModel",
      "w", "w", "LengthFeature",
      "sm", "s", "SegmentationModel",
      "unal", "unal", "UnalFeature",
      "sparse", "sparse", "SparseModel",
      "ibm1f", "ibm1-fwd", "IBM1FwdFeature",
      "lev", "lev", "LevFeature",
      "ng", "ngrams", "NgramMatchFeature",
      "ruw", "rules", "RuleFeature",
      "bilm", "bilm", "BiLMModel",
      "nnjm", "nnjm", "NNJM",
      // insert new features above this line - in the same order as in
      // BMG::InitDecoderFeatures()!!!
   };
   for (Uint i = 0; i+2 < ARRAY_SIZE(weight_names_other); i += 3) {
      const string shortname = weight_names_other[i];
      const string longname = weight_names_other[i+1];
      weight_params_other.push_back(shortname);
      FeatureGroup * const f = features[shortname] =
         new FeatureGroup(shortname, weight_names_other[i+2]);

      const string weight_names = "weight-" + longname + " " + shortname;
      const string random_names = "random-" + longname + " r" + shortname;
      param_infos.push_back(ParamInfo(weight_names, "doubleVect", &f->weights));
      param_infos.push_back(ParamInfo(random_names, "stringVect", &f->rnd_weights));
   }

   // For each feature, attach its args element to the appropriate param_info,
   // so the list of models can be read from the canoe.ini or command line.
   param_infos.push_back(ParamInfo("distortion-model", "stringVect", &featureGroup("d")->args));
   featureGroup("w")->need_args = false;
   param_infos.push_back(ParamInfo("segmentation-model", "stringVect", &featureGroup("sm")->args));
   param_infos.push_back(ParamInfo("unal-feature", "stringVect", &featureGroup("unal")->args));
   // relative_path_modification removed for sparse-model because the model
   // needs to be internally relative-path aware.
   param_infos.push_back(ParamInfo("sparse-model", "stringVect", &featureGroup("sparse")->args,
         ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("ibm1-fwd-file", "stringVect", &featureGroup("ibm1f")->args,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   featureGroup("lev")->need_args = false;
   featureGroup("ng")->need_args = true; // but args not via param_infos - created in check()
   param_infos.push_back(ParamInfo("bilm-file", "stringVect", &featureGroup("bilm")->args,
      ParamInfo::relative_path_modification | ParamInfo::bilm_check_file_name));
   param_infos.push_back(ParamInfo("nnjm-file", "stringVect", &featureGroup("nnjm")->args,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));

   // The rule feature is weird, we have to turn off its need_args here, and turn
   // it back on in check() after initializing its args vector.
   featureGroup("ruw")->need_args = false;

   // The "primary" features are each handled in their own specific ways, so
   // they are dealt with separately.
   const char* weight_names_primary[] = {
      "lm",
      "tm",
      "ftm",
      "atm",
   };
   weight_params_primary.assign(weight_names_primary, weight_names_primary + ARRAY_SIZE(weight_names_primary));

   weight_params = weight_params_other;
   weight_params.insert(weight_params.end(), weight_params_primary.begin(), weight_params_primary.end());

   for (Uint i = 0; i < param_infos.size(); ++i) {
      param_infos[i].c = this;
      bool bool_param = (param_infos[i].tconv == "bool");
      string app(bool_param ? "" : ":");
      for (Uint j = 0; j < param_infos[i].names.size(); ++j) {
         param_map[param_infos[i].names[j]] = &param_infos[0] + i;
         param_list.push_back(param_infos[i].names[j] + app);
         if ( bool_param ) {
            param_map["no-" + param_infos[i].names[j]] = &param_infos[0] + i;
            param_list.push_back("no-" + param_infos[i].names[j]);
         }
      }
   }

   for (Uint i = 0; i < weight_params.size(); ++i) {
      map<string,ParamInfo*>::iterator it = param_map.find(weight_params[i]);
      if (it == param_map.end())
         error(ETFatal, "programmer error: weight parameter %s not found in param_infos list",
               weight_params[i].c_str());
      if (it->second->tconv != "double" && it->second->tconv != "doubleVect")
         error(ETFatal, "programmer error: weight parameter %s is not double or doubleVect",
               weight_params[i].c_str());
   }

   for (Uint i = 0; i < weight_params_other.size(); ++i)
      assert(features.at(i).first == weight_params[i] &&
             features.at(i).second->shortname == weight_params[i]);
}

// Implementation note: since there is no way of specifying an empty string in
// the config file, the special string ALT_EMPTY_STRING is mapped to and from
// "" when writing or reading config files. This is done by default for most
// parameters, but may require special handling in certain cases, eg "nbest"
// below.

static const char* ALT_EMPTY_STRING = "--";


// Set parameter value from string specification.

void CanoeConfig::ParamInfo::set(const string& str)
{
   string s(str == ALT_EMPTY_STRING ? "" : str);

   if (tconv == "bool") {
      *(bool*)val = true;
   } else if (tconv == "string") {
      *(string*)val = s;
   } else if (tconv == "Uint") {
      *(Uint*)val = conv<Uint>(s);
   } else if (tconv == "int") {
      *(int*)val = conv<int>(s);
   } else if (tconv == "double") {
      *(double*)val = conv<double>(s);
   } else if (tconv == "stringVect") {
      splitZ(s, *(vector<string>*)val, ":");
   } else if (tconv == "UintVect") {
      splitCheckZ(s, *(vector<Uint>*)val, ":");
   } else if (tconv == "doubleVect") {
      splitCheckZ(s, *(vector<double>*)val, ":");
   } else if (tconv == "lat") {   // lattice params
      c->latticeFilePrefix = s;
      c->latticeOut = (c->latticeFilePrefix != "");
   } else if (tconv == "nb") {   // nbest list params
      vector<string> toks;
      split(s, toks, ":", 2);
      c->nbestFilePrefix = toks[0] == ALT_EMPTY_STRING ? "" : toks[0];
      if (toks.size() == 2) c->nbestSize = conv<Uint>(toks[1]);
      c->nbestOut = (c->nbestFilePrefix != "");
   } else
      error(ETFatal, "programmer error: cannot convert param %s - conversion method %s unknown",
            names[0].c_str(), tconv.c_str());
}


// Set bool parameter value directly from a bool argument

void CanoeConfig::ParamInfo::set(bool value) {
   assert(tconv == "bool");
   *(bool*)val = value;
}


// Get string representation for parameter value.

string CanoeConfig::ParamInfo::get(bool pretty) {
   ostringstream ss;
   ss << setprecision(precision);

   if (tconv == "bool") {
      ss << *(bool*)val;
   } else if (tconv == "string") {
      ss << *(string*)val;
   } else if (tconv == "Uint") {
      ss << *(Uint*)val;
   } else if (tconv == "int") {
      ss << *(int*)val;
   } else if (tconv == "double") {
      ss << *(double*)val;
   } else if (tconv == "stringVect") {
      vector<string>& v = *(vector<string>*)val;
      if (pretty) {
         for (vector<string>::iterator it = v.begin(); it != v.end(); ++it)
            ss << "\n   " << *it;
      } else
         ss << join(v, ":");
   } else if (tconv == "UintVect") {
      vector<Uint>& v = *(vector<Uint>*)val;
      ss << join(v, ":");
   } else if (tconv == "doubleVect") {
      vector<double>& v = *(vector<double>*)val;
      ss << join(v, ":", precision);
   } else if (tconv == "lat") {   // lattice params
      ss << c->latticeFilePrefix;
   } else if (tconv == "nb") {   // nbest list params
      ss << (c->nbestFilePrefix == "" ? ALT_EMPTY_STRING : c->nbestFilePrefix)
         << ":" << c->nbestSize;
   } else
      error(ETFatal, "programmer error: cannot convert param %s - conversion method %s unknown",
            names[0].c_str(), tconv.c_str());

   return ss.str() == "" ? ALT_EMPTY_STRING : ss.str();
}

void CanoeConfig::setFromArgReader(ArgReader& arg_reader)
{
   map<string,ParamInfo*>::iterator it;
   for (it = param_map.begin(); it != param_map.end(); ++it) {
      string val;
      if (arg_reader.getSwitch(it->first.c_str(), &val)) {
         if (!it->second->set_from_cmdline) {
            if ( it->second->tconv == "bool" )
               it->second->set(it->first.compare(0,3,"no-") != 0);
            else
               it->second->set(val);
            it->second->set_from_cmdline = true;
         } else
            error(ETWarn, "ignoring duplicate option -%s on command line",
                  it->first.c_str());
      }
   }
}

/**
 * Get next value from config file. Values consist of one or more
 * space-separated words not beginning with the [ character. If the comment
 * character (#) occurs at the beginning of a word, all text on the line will
 * be ignored.
 * @param configin stream
 * @param val all words found, concatenated with ':' separator
 * @return true if one or more words found
 */
static bool getValue(istream& configin, string& val)
{
   val.clear();
   char c;
   enum {in_white, in_word, in_comment, done} state = in_white;

   while (!configin.eof() && state != done) {
      //      cerr << state << " [" << val << "]" << endl;
      configin.get(c);
      bool is_white = (c == ' ' || c == '\t' || c == '\r' || c == '\n');
      switch (state) {
      case in_white:
         if (c == '#') state = in_comment;
         else if (c == '[') {configin.putback(c); state = done;}
         else if (!is_white) {
            state = in_word;
            if (val.length() != 0) val.append(1, ':');
            val.append(1, c);
         }
         break;
      case in_word:
         if (is_white) state = in_white;
         else val.append(1, c);
         break;
      case in_comment:
         if (c == '\n') state = in_white;
         break;
      case done:      // keep compiler quiet
         break;
      }
   }
   return val.length() > 0;
}

/**
 * Callable Entity used to modify the LMs and TMs file path according to the
 * location of the canoe.ini file to which they belong.
 */
struct extendFileName
{
   const string remoteDirName;  ///< Path of the canoe.ini

   /**
    * Default constructor.
    * @param d  path of the canoe.ini
    */
   extendFileName(const string& d) : remoteDirName(d) {}

   /**
    * Transforms file name and path in accordance to the location of the
    * canoe.ini to which they belong.  Allows us to run remote canoes.
    * @param file LM or TM file name to modify
    */
   void operator()(string& file) {
      // extends the path only if it's not an absolute path
      if (isPrefix(LMDynMap::header, file))
         file = LMDynMap::fix_relative_path(remoteDirName, file);
      else
         file = adjustRelativePath(remoteDirName, file);
   }
};

void CanoeConfig::read(const char* configFile) {
   iSafeMagicStream cfg(configFile);
   read(cfg);
   this->configFile = configFile;

   extendFileName efn(DirName(configFile));

   // Modifying filepath for canoe.relative
   if (efn.remoteDirName != ".") {
      // Look through param_info and see which one needs their path modified
      typedef vector<ParamInfo>::iterator  IT;
      for (IT it(param_infos.begin()); it!=param_infos.end(); ++it) {
         if (it->groups & ParamInfo::bilm_check_file_name) {
            if (it->tconv == "stringVect") {
               vector<string>& v = *((vector<string>*)(it->val));
               for (Uint i = 0; i < v.size(); ++i)
                  v[i] = BiLMModel::fix_relative_path(efn.remoteDirName, v[i]);
            }
         }
         else if (it->groups & ParamInfo::relative_path_modification) {
            if (it->tconv == "stringVect") {
               vector<string>& v = *((vector<string>*)(it->val));
               for_each(v.begin(), v.end(), efn);
               if (false) copy(v.begin(), v.end(), ostream_iterator<string>(cerr, "\n"));
            } else if (it->tconv == "string") {
               string& f = *(string*)(it->val);
               efn(f);
            }
         }
      }
   }
}

void CanoeConfig::read(istream& configin)
{
   while (true) {

      // Read in a string, terminated by either a space, the newline character, or #
      // (indicating a comment)
      string s;
      char c;
      configin.get(c);
      if (configin.eof()) break;
      while (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '#') {
         s.append(1, c);
         configin.get(c);
      }
      if (c == '#')             // discard to EOL
         while (!configin.eof() && c != '\n')
            configin.get(c);

      if (s.length() > 1 && s[0] == '[' && s[s.length() - 1] == ']') {
         s.erase(0, 1);
         s.erase(s.length() - 1);

         map<string,ParamInfo*>::iterator it = param_map.find(s);
         if (it != param_map.end()) {
            string arg;
            if (it->second->tconv != "bool") getValue(configin, arg);
            if (!it->second->set_from_config) {
               if ( it->second->tconv == "bool" )
                  it->second->set(it->first.compare(0,3,"no-") != 0);
               else
                  it->second->set(arg);
               it->second->set_from_config = true;
            } else
               error(ETWarn, "ignoring duplicate option [%s] in config file", s.c_str());

         } else
            error(ETWarn, "ignoring unknown option [%s] in config file", s.c_str());
      } else if (s != "")
         error(ETWarn, "ignoring unknown option %s in config file", s.c_str());
   }
}

/// For logging purposes
static string curTime() {
   time_t now = time(0);
   string ret = asctime(localtime(&now));
   return trim(ret, "\n \t");
}

void CanoeConfig::lock()
{
   int lock_fd = open(configFile.c_str(), O_RDONLY);
   if (lock_fd < 0)
      error(ETFatal, "Cannot open config file %s for locking.%s%s",
         configFile.c_str(),
         (errno != 0 ? ": " : ""), (errno != 0 ? strerror(errno) : ""));
   if (flock(lock_fd, LOCK_SH|LOCK_NB) < 0) {
      cerr << "Waiting for shared lock on " << configFile << " on " << curTime() << endl;
      if (flock(lock_fd, LOCK_SH) < 0)
         error(ETFatal, "Cannot obtain read lock on config file %s.%s%s",
            configFile.c_str(),
            (errno != 0 ? ": " : ""), (errno != 0 ? strerror(errno) : ""));
   }
   if (verbosity >= 1)
      cerr << "Got shared lock on " << configFile << " on " << curTime() << endl;
}

void CanoeConfig::check()
{
   // CAC: Alias processing before file check
   // Enabling aliased lexical-distortion models so that distortion features
   // can refer to specific files by their aliases
   map<string,Uint> aliases;
   Uint iNumAliases=0;
   for(vector<string>::iterator it=LDMFiles.begin();
       it!=LDMFiles.end(); ++it){
      vector<string> toks;
      if(split(*it,toks,"#")>1){
         if(toks.size()!=2)
            error(ETFatal,"Only understand how to handle one # in lex-dist-model-file: %s", it->c_str());
         if(aliases.find(toks[1])!=aliases.end())
            error(ETFatal,"Repeated alias %s in lex-dist-model-file %s", toks[1].c_str(), it->c_str());
         aliases[toks[1]]=iNumAliases++;
         it->assign(toks[0]);
      }
   }
   if(aliases.size()>0 && aliases.size()!=LDMFiles.size())
      error(ETFatal,"Did not provide aliases for all entries in lex-dist-model-file");
   
   
   // Before checking the integrity of the config, we must make sure all files
   // are valid.
   check_all_files();

   // allNonMultiProbPTs initialization: allNonMultiProbPTs = tppt + ttables
   // Must be done early, since some setting of defaults below uses allNonMultiProbPTs.
   for (Uint i = 0; i < tpptFiles.size(); ++i)
      if (!TPPTFeature::isA(tpptFiles[i]))
         error(ETFatal, "TPPT file %s specified in -ttable-tppt does not appear to be a TPPT",
               tpptFiles[i].c_str());
   allNonMultiProbPTs = tpptFiles;
   allNonMultiProbPTs.insert(allNonMultiProbPTs.end(), TTables.begin(), TTables.end());

   // Set defaults:

   FeatureGroup* lengthF = featureGroup("w"); // save this pointer since we reuse it later
   if (lengthF->weights.empty())
      lengthF->weights.push_back(0.0);

   FeatureGroup* distortionF = featureGroup("d"); // save this pointer since we reuse it later
   if (distortionF->args.empty())
      distortionF->args.push_back("WordDisplacement");
   else if (distortionF->args[0] == "none")
      distortionF->args.clear();

   FeatureGroup* segmentationF = featureGroup("sm");
   if (segmentationF->args.size() == 1 && isPrefix("none", segmentationF->args[0])) {
      error(ETFatal, "Please remove obsolete \"[segmentation-model] none\" or \"none#whatever\" from your canoe.ini files and scripts");
      segmentationF->args.clear();
   }

   FeatureGroup *ngF = featureGroup("ng");
   for (Uint i = 0; i < ngF->weights.size(); ++i)
      ngF->args.push_back(join(vector<Uint>(1,i+1))); // lazy way to make a string out of a number

   FeatureGroup* sparseF = featureGroup("sparse");
   if (sparseF->weights.empty())
      sparseModelAllowNonLocalWts = true;

   for (FeatureGroupMap::iterator f_it = features.begin(); f_it != features.end(); ++f_it) {
      if (f_it->second->need_args) {
         if (f_it->second->weights.empty())
            f_it->second->weights.assign(f_it->second->args.size(), 1.0);
         if (f_it->second->weights.size() != f_it->second->args.size())
            error(ETFatal, "Number of %s args does not match number of %s weights.",
                  f_it->second->group.c_str(), f_it->second->group.c_str());
      }
   }

   if (lmWeights.empty())
      lmWeights.resize(lmFiles.size(), 1.0);

   const Uint prob_model_count(getTotalBackwardModelCount());
   const Uint multi_adir_model_count(getTotalAdirectionalModelCount()); //boxing

   if (transWeights.empty())
      transWeights.assign(prob_model_count, 1.0);

   if (forwardWeights.empty() && useFtm)
      forwardWeights.assign(prob_model_count, 1.0);

   if (adirTransWeights.empty())
      adirTransWeights.resize(multi_adir_model_count, 1.0);

   // Rule decoder feature
   FeatureGroup* ruwF = featureGroup("ruw");
   if (ruwF->weights.empty())
      ruwF->weights.resize(rule_classes.size(), 1.0f);
   if (rule_log_zero.empty())
      rule_log_zero.resize(rule_classes.size(), phraseTableLogZero);

   if (rule_classes.size() != ruwF->weights.size())
      error(ETFatal, "number of rule weights does not match number of rule classes");
   if (rule_classes.size() != rule_log_zero.size())
      error(ETFatal, "number of rule log zero does not match number of rule classes");

   for (Uint i = 0; i < ruwF->weights.size(); ++i)
      ruwF->args.push_back(rule_classes[i] + ":" + join(vector<double>(1,rule_log_zero[i])));
   // at this point, ruwF->args is initialized, set need_args so that
   // BasicModel::InitDecoderFeatures() knows to use args.
   ruwF->need_args = true;


   ////////////////////////////////////////////////////////
   // ERRORS:
   if (bLoadBalancing && bAppendOutput)
      error(ETFatal, "Load Balancing cannot run in append mode");

   //if (latticeOut && nbestOut)
   //   error(ETFatal, "Lattice and nbest output cannot be generated simultaneously.");

   // CAC: Sanity check on lattice density
   if(latticeDensity < BIG_LAT_DENSITY && latticeOutputOptions!="overlay")
      error(ETWarn, "Specified lattice density, but only 'overlay' lattices can be pruned");
   if(latticeMinEdge > SMALL_MIN_EDGE && latticeOutputOptions!="overlay")
      error(ETWarn, "Specified minumum lattice edge, but only 'overlay' lattices care");
   if(latticeLogProb && latticeOutputOptions!="overlay")
      error(ETWarn, "Specified lattice log probs, but only 'overlay' lattices care");
   if(latticeSourceDensity && latticeOutputOptions!="overlay")
      error(ETWarn, "Specified lattice source density, but only 'overlay' lattices care");

   // CAC: Sanity check and act on shift-reducer modifiers
   if (shiftReduceOnlyITG) {
      if (!ShiftReducer::usingSR(*this))
         error(ETWarn, "Specified shift-reduce-only-itg with no need for shift-reduce parser");
      ShiftReducer::allowOnlyITG();
   }
   
   // CAC: Here is how you get multiple LDM files into play.
   // Tag each LDM file with a #TAG, and then for each LDM
   // feature, use the same #TAG to indicate what model provides
   // its probabilities. This will be transformed into an index
   // based tag scheme that the features understand how to handle.
   // 
   // [lex-dist-model-file] 
   //    dm.merge.main.ch2en.ldm.gz#LDM
   //    dm.merge.main.ch2en.hldm.gz#HDM
   // [distortion-model] 
   //    WordDisplacement
   //    back-lex#m#LDM
   //    back-lex#s#LDM
   //    back-lex#d#LDM
   //    fwd-lex#m#LDM
   //    fwd-lex#s#LDM
   //    fwd-lex#d#LDM
   //    back-hlex#m#HDM
   //    back-hlex#s#HDM
   //    back-hlex#d#HDM
   //    fwd-hlex#m#HDM
   //    fwd-hlex#s#HDM
   //    fwd-hlex#d#HDM
   
   bool existingLex = false;
   vector<bool> usingLex(LDMFiles.size(),false);
   for (vector<string>::iterator it=distortionF->args.begin(); it != distortionF->args.end(); ++it){
      if (isPrefix("fwd-lex",*it)
          || isPrefix("back-lex",*it) 
          || isPrefix("fwd-hlex",*it) 
          || isPrefix("back-hlex",*it)
          || isPrefix("back-fhlex",*it)
          ) {
         existingLex = true;
         // CAC: More alias handling
         vector<string> toks;
         if(aliases.size()>0) {
            map<string,Uint>::iterator mt;
            if(split(*it,toks,"#")<2 || (mt=aliases.find(toks[toks.size()-1]))==aliases.end())
               error(ETFatal, "Couldn't find a valid alias in dist feature: %s.",it->c_str());
            // Transform alias into an index
            Uint index = mt->second;
            // Replace the aliased feature with an indexed feature
            stringstream feat;
            for(Uint i=0;i<toks.size()-1;i++) {
               if(i>0) feat << "#";
               feat<<toks[i];
            }
            feat << "#" << index;
            it->assign(feat.str());
         }
         else {
            if(split(*it,toks,"#")>2 && !isdigit(toks[2][0]))
               error(ETFatal, "Looks like %s has an alias, without any aliases on lex-dist-model-files",
                     it->c_str());
         }

         // CAC: Make sure all indexes (either provided or inferred by aliases)
         // are less than the number of models
         toks.clear();
         split(*it,toks,"#");
         string snum="";
         if(toks.size()==2) snum = toks[1];
         if(toks.size()==3) snum = toks[2];
         if(!snum.empty() && isdigit(snum[0])) {
            Uint inum = conv<Uint>(snum);
            if(inum >= LDMFiles.size())
               error(ETFatal, "Provided distortion feature index %d >= LDMFile count %d",
                     inum, LDMFiles.size());
            usingLex[inum] = true;
         }
         string direction="";
         if(toks.size()==2 && !isdigit(toks[1][0])) direction = toks[1];
         if(toks.size()==3) direction = toks[1];
         if(!direction.empty() && direction != "s" && direction != "m" && direction != "d")
            error(ETFatal, "Distortion model %s has invalid direction: %s; permitted direction arguments are m, s, and d.", (*it).c_str(), direction.c_str());
      }
   }

   if (LDMFiles.empty() && existingLex)
      error(ETFatal, "No lexicalized distortion model file specified.");
   if (!LDMFiles.empty() && !existingLex)
      error(ETFatal, "No lexicalized distortion model type [fwd-[h]lex and|or back-[h]lex] specified.");
   if (LDMFiles.size() > 1) {
      for(Uint i=0;i<LDMFiles.size();++i) {
         if(!usingLex[i])
            error(ETFatal, "Included LDM %s, but no features use it", LDMFiles[i].c_str());
      }
   }
   if (!LDMFiles.empty()) {
      //text LDM files aren't compatible with tppts or dyn PTs
      for (Uint i = 0; i < LDMFiles.size(); ++i) {
         if (!isSuffix(".tpldm", LDMFiles[i])) {
            if (!allNonMultiProbPTs.empty())
               error(ETFatal, "Text LDM files are not compatible with non-multi-prob phrase tables; convert your LDM (%s) to a TPLDM to proceed.", LDMFiles[0].c_str());
         }
      }
   }

   // TPLDMs must all come after text LDM files.
   if (!LDMFiles.empty()) {
      bool foundTPLDM = false;
      for (Uint i = 0; i < LDMFiles.size(); ++i) {
         if (isSuffix(".tpldm", LDMFiles[i]))
            foundTPLDM = true;
         else if (foundTPLDM)
            error(ETFatal, "When mixing Text LDM and TPLDM files, the Text LDMs must come first in -lex-dist-model-file.");
      }
   }

   if (lmFiles.empty())
      error(ETFatal, "No language model file specified.");
   if (multiProbTMFiles.empty() && allNonMultiProbPTs.empty())
      error(ETFatal, "No phrase table file specified.");
   if (transWeights.empty())
      error(ETFatal, "No translation model in the phrase table file(s) specified.");

   /* This check has to be deferred, since -ref logically belongs on the
      command line, not in the canoe.ini file: we don't want the check to be
      applied in "configtool check", which only checks the canoe.ini file.
   if (forcedDecoding || forcedDecodingNZ || !featureGroup("lev")->weights.empty() || !featureGroup("ng")->weights.empty())
      if (refFile.empty())
         error(ETFatal, "Specified forced decoding, the levenshtein feature or the n-gram match feature but no reference file.");
   */

   if (forcedDecoding && !forcedDecodingNZ) {
      lmWeights.assign(lmWeights.size(), 0.0);
      featureGroup("ibm1f")->weights.assign(featureGroup("ibm1f")->weights.size(), 0.0);
      lengthF->weights.assign(lengthF->weights.size(), 0.0);
   }

   if ((forcedDecoding || forcedDecodingNZ) && bCubePruning)
      error(ETFatal, "Forced decoding and cube pruning are not compatible.");

   if (lmWeights.size() != lmFiles.size())
      error(ETFatal, "Number of language model weights does not match number of language model files.");
   if (transWeights.size() != prob_model_count)
      error(ETFatal, "Number of translation model weights does not match number of translation models.");
   if (forwardWeights.size() > 0 && forwardWeights.size() != prob_model_count)
      error(ETFatal, "Number of forward translation model weights != number of (forward) translation models.");
   if (adirTransWeights.size() != multi_adir_model_count) //boxing
      error(ETFatal, "Number of adirectional translation model weights != number of (adirectional) translation models."); //boxing

   if (oov != "pass" && oov != "write-src-marked" && oov != "write-src-deleted")
      error(ETFatal, "OOV handling method must be one: 'pass', 'write-src-marked', or 'write-src-deleted'");
   if (phraseTablePruneType != "forward-weights" &&
       phraseTablePruneType != "backward-weights" &&
       phraseTablePruneType != "combined" &&
       phraseTablePruneType != "full")
      error(ETFatal, "ttable-prune-type must be one of: 'forward-weights', 'backward-weights', 'combined', or 'full'");

   // if (bypassMarked && weightMarked)
   //   error(ETWarn, "Both -bypass-marked and -weight-marked found.  Only doing -bypass-marked");

   if ( diversity ) {
      if ( diversityStackIncrement < -1 )
         error(ETFatal, "-diversity-stack-increment must be non-negative, or -1 for DSI=I");
      else if ( diversityStackIncrement == -1 )
         diversityStackIncrement = maxRegularStackSize;
   }

   if (!bCubePruning)
      bStackDecoding = true;

   if ((bStackDecoding ? 1 : 0) +
       (bCubePruning ? 1 : 0)
       > 1)
      error(ETFatal, "Can only run one decoder! Chose only one of -stack-decoding, -cube-pruning.");

   if (bCubePruning)
      if (covLimit != 0 || covThreshold != 0.0 || diversity != 0)
         error(ETFatal, "Coverage pruning and diversity are not implemented in the cube-pruning decoder.");

   if ( cubeLMHeuristic != "none" && cubeLMHeuristic != "unigram" &&
        cubeLMHeuristic != "incremental" && cubeLMHeuristic != "simple" )
      error(ETFatal, "cube-lm-heuristic must be one of: 'none', 'unigram', 'incremental', or 'simple'");

   if ( futLMHeuristic != "none" && futLMHeuristic != "unigram" &&
        futLMHeuristic != "incremental" && futLMHeuristic != "simple" )
      error(ETFatal, "future-score-lm-heuristic must be one of: 'none', 'unigram', 'incremental', or 'simple'");

   // Checking weights' value for valid numerical values
   vector<double> wts;
   getFeatureWeights(wts);
   for (Uint i(0); i<wts.size(); ++i)
      if (!isfinite(wts[i]))
         error(ETFatal, "A weight has a non-finite value (id:%d, value:%f).",
               i, wts[i]);

   // loadFirst is required when using TPPTs, dynamic phrase tables or -ttable
   if (!loadFirst && !allNonMultiProbPTs.empty()) {
      error(ETWarn, "Setting -load-first.");
      loadFirst = true;
   }

   if (distLimitExt && distLimitSimple)
      error(ETFatal, "Can't use both -dist-limit-ext and -dist-limit-simple.");

   for (Uint i = 0; i < allNonMultiProbPTs.size(); ++i) {
      if (MultiProbPTFeature::isA(allNonMultiProbPTs[i]))
         error(ETFatal, "Multi-prob phrase table %s must be specified in [ttable-multi-prob], not other [ttable*] options.",
               allNonMultiProbPTs[i].c_str());
   }

   map<string,ParamInfo*>::iterator it_reg_stack = param_map.find("regular-stack");
   map<string,ParamInfo*>::iterator it_stack = param_map.find("stack");
   assert(it_reg_stack != param_map.end());
   assert(it_stack != param_map.end());
   if (bCubePruning) {
      if ((it_reg_stack->second->set_from_config || it_reg_stack->second->set_from_cmdline) &&
          !it_stack->second->set_from_cmdline)
         error(ETFatal, "Cube pruning requires -stack, and does not allow -regular-stack.  This is because the regular stack counts states after recombining, whereas the cube pruning stack counts states before recombining.  Thus, with -cube-pruning, you need a larger stack.  To avoid confusion and keeping inappropriate stack parameters when altering a canoe.ini file, we now require the right type of stack parameter for the decoder you choose.");
   } else {
      if ((it_stack->second->set_from_config || it_stack->second->set_from_cmdline) &&
          !it_reg_stack->second->set_from_cmdline)
         error(ETFatal, "Regular stack decoding requires -regular-stack, and does not allow -stack.  This is because the regular stack counts states after recombining, whereas the cube-pruning stack counts states before recombining.  Thus without cube pruning, you need a smaller stack.  To avoid confusion and keeping inappropriate stack parameters when altering a canoe.ini file, we now require the right type of stack parameter for the decoder you choose.");
   }

   if (tolerateMarkupErrors)
      error(ETFatal, "The -tolerate-markup-errors switch is no longer supported.  Instead, pipe your input through canoe-escapes.pl before passing it to canoe, if you don't use a rule parser that already introduces such escapes.");

   if (appendJointCounts && !allNonMultiProbPTs.empty())
      error(ETFatal, "The -append-joint-counts option is only supported with multi-prob phrase tables, and no other types of phrase tables.");
} //check()

void CanoeConfig::check_all_files() const
{
   bool ok = true;

   extendFileName efn(DirName(configFile));
   // Look through param_info and see which ones need their path checked
   typedef vector<ParamInfo>::const_iterator  IT;
   for (IT it(param_infos.begin()); it!=param_infos.end(); ++it) {
      if (it->groups & ParamInfo::check_file_name) {
         if (it->tconv == "stringVect") {
            vector<string>& v = *((vector<string>*)(it->val));
            for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
               if (f->find("<src>") < f->size()) continue;
               // Paths specified in the config are always relative to the config.
               // Therefore must apply relative path modification here if
               // ParamInfo::relative_path_modification is not set.
               string mf(*f);
               if (efn.remoteDirName != "." && it->set_from_config &&
                   !(it->groups & ParamInfo::relative_path_modification))
                  efn(mf);
               if (!check_if_exists(mf)) {
                  cerr << "Error: Can't access: " << mf << endl;
                  ok = false;
               } else if (is_directory(mf)) {
                  cerr << "Error: Directory found when file expected for "
                       << it->names[0] << " parameter: " << mf << endl;
                  if (it->names[0] == "ttable-multi-prob")
                     cerr << "NOTE: use the ttable-tppt parameter to specify a TPPT directory." << endl;
                  ok = false;
               }
            }
         }
         else if (it->tconv == "string") {
            const string& f = *(string*)(it->val);
            if (f.empty() || f.find("<src>") < f.size()) continue;
            // Paths specified in the config are always relative to the config.
            // Therefore must apply relative path modification here if
            // ParamInfo::relative_path_modification is not set.
            string mf(f);
            if (efn.remoteDirName != "." && it->set_from_config &&
                !(it->groups & ParamInfo::relative_path_modification))
               efn(mf);
            if (!check_if_exists(mf)) {
               cerr << "Error: Can't access: " << mf << endl;
               ok = false;
            } else if (is_directory(mf)) {
               cerr << "Error: Directory found when file expected for "
                    << it->names[0] << " parameter: " << mf << endl;
               ok = false;
            }
         }
      }
      else if (it->groups & ParamInfo::check_dir_name) {
         vector<string> v;
         if (it->tconv == "stringVect")
            v = *((vector<string>*)(it->val));
         else
            v.push_back(*(string*)(it->val));
         for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
            if (!is_directory(*f)) {
               cerr << "Error: " << *f << " is not a directory in " << it->names[0]
                    << " parameter." << endl;
               ok = false;
            }
         }
      }
      else if (it->groups & ParamInfo::lm_check_file_name) {
         assert(it->tconv == "stringVect");
         vector<string>& v = *((vector<string>*)(it->val));
         for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
            if (!PLM::checkFileExists(*f)) {
               if (!is_directory(*f))
                  cerr << "Error: Can't access LM: " << *f << endl;
               else {
                  cerr << "Error: Directory found when file expected for LM: " << *f << endl;
                  cerr << "NOTE: for a TPLM, the directory name must end with '.tplm'." << endl;
               }
               ok = false;
            }
         }
      }
      else if (it->groups & ParamInfo::bilm_check_file_name) {
         assert(it->tconv == "stringVect");
         vector<string>& v = *((vector<string>*)(it->val));
         for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
            if (!BiLMModel::checkFileExists(*f)) {
               cerr << "Error: Can't access BiLM: " << *f << endl;
               ok = false;
            }
         }
      }
      else if (it->groups & ParamInfo::ttable_check_file_name) {
         assert(it->tconv == "stringVect");
         vector<string>& v = *((vector<string>*)(it->val));
         for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
            if (!PhraseTableFeature::checkFileExists(*f)) {
               cerr << "Error: Can't access TTable: " << *f << endl;
               ok = false;
            }
         }
      }
      else if (it->groups & ParamInfo::ldm_check_file_name) {
         assert(it->tconv == "stringVect");
         vector<string>& v = *((vector<string>*)(it->val));
         for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
            if (isSuffix(".tpldm", *f)) {
               if (!ugdiss::TpPhraseTable::checkFileExists(*f)) {
                  cerr << "Error: Can't access TPLDM: " << *f << endl;
                  ok = false;
               }
            } else {
               if (!check_if_exists(*f)) {
                  cerr << "Error: Can't access LDM: " << *f << endl;
                  ok = false;
               } else if (is_directory(*f)) {
                  cerr << "Error: Directory found when file expected for LDM: " << *f << endl;
                  cerr << "NOTE: for a TPLDM, the directory name must end with '.tpldm'." << endl;
                  ok = false;
               }
            }
         }
      }
   }

   // On error, exit with error status so this check can be used in scripts
   if (!ok)
      error(ETFatal, "At least one file error with canoe configuration.");
}

void CanoeConfig::movePTsIntoTTable()
{
   assert(tpptFiles.size()+TTables.size() == allNonMultiProbPTs.size() && "you must call check() before calling movePTsIntoTTable()");

   tpptFiles.clear();
   readStatus("ttable-tppt") = false;

   TTables = allNonMultiProbPTs;
   if (!TTables.empty()) readStatus("ttable") = true;
}

Uint CanoeConfig::getTotalBackwardModelCount() const
{
   Uint count = 0;
   for (Uint i = 0; i < multiProbTMFiles.size(); ++i)
      count += PhraseTable::countProbColumns(multiProbTMFiles[i]);
   for (Uint i = 0; i < allNonMultiProbPTs.size(); ++i)
      count += PhraseTable::countProbColumns(allNonMultiProbPTs[i]);
   count = count / 2;
   return count;
}

Uint CanoeConfig::getTotalMultiProbModelCount() const
{
   Uint count = 0;
   for (Uint i = 0; i < multiProbTMFiles.size(); ++i)
      count += PhraseTable::countProbColumns(multiProbTMFiles[i]);
   return count / 2;
}

//boxing
Uint CanoeConfig::getTotalAdirectionalModelCount() const
{

   Uint count = 0;
   for (Uint i = 0; i < multiProbTMFiles.size(); ++i)
      count += PhraseTable::countAdirScoreColumns(multiProbTMFiles[i]);
   for (Uint i = 0; i < allNonMultiProbPTs.size(); ++i)
      count += PhraseTable::countAdirScoreColumns(allNonMultiProbPTs[i]);
   return count;
} //boxing

Uint CanoeConfig::getTotalTPPTModelCount() const
{
   Uint count = 0;
   for (Uint i = 0; i < tpptFiles.size(); ++i)
      count += PhraseTable::countProbColumns(tpptFiles[i]);
   for (Uint i = 0; i < TTables.size(); ++i) {
      if (TPPTFeature::isA(TTables[i]))
         count += PhraseTable::countProbColumns(TTables[i]);
   }
   return count / 2;
}

string& CanoeConfig::getFeatureWeightString(string& s) const
{
   s.clear();
   map<string,ParamInfo*>::const_iterator it;
   for (Uint i = 0; i < weight_params.size(); ++i) {
      it = param_map.find(weight_params[i]);
      assert (it != param_map.end());
      string val = it->second->get();
      if (val != ALT_EMPTY_STRING) {
         s += " -" + weight_params[i];
         s += " " + val;
      }
   }
   return s;
}

void CanoeConfig::setFeatureWeightsFromString(const string& s)
{
   vector<string> toks;
   split(s, toks);
   map<string,ParamInfo*>::iterator it;
   for (Uint i = 0; i < toks.size(); i += 2) {

      string sw = toks[i][0] == '-' ? toks[i].substr(1) : toks[i];
      it = param_map.find(sw);
      if (it == param_map.end()) {
         error(ETWarn, "unknown weight parameter in weight string: %s", sw.c_str());
         continue;
      }
      if (i+1 >= toks.size()) {
         error(ETWarn, "ignoring last element in weight string: %s", toks[i].c_str());
         continue;
      }

      it->second->set(toks[i+1]);
      it->second->set_from_config = true; // ugh, sorry!
   }
}


void CanoeConfig::getFeatureWeights(vector<double>& weights) const
{
   weights.clear();
   map<string,ParamInfo*>::const_iterator it;
   for (Uint i = 0; i < weight_params.size(); ++i) {
      it = param_map.find(weight_params[i]);
      assert (it != param_map.end());
      if (it->second->tconv == "double")
         weights.push_back(*(double*)it->second->val);
      else if (it->second->tconv == "doubleVect") {
         vector<double>* v = (vector<double>*)it->second->val;
         weights.insert(weights.end(), v->begin(), v->end());
      } else
         assert(false);
   }
}

void CanoeConfig::getSparseFeatureWeights(vector<double>& weights) const
{
   weights.clear();

   // get normal decoder weights, then zero any SparseModel weights

   getFeatureWeights(weights);
   Uint index = 0;
   map<string,ParamInfo*>::const_iterator it;
   for (Uint i = 0; i < weight_params.size(); ++i) {
      it = param_map.find(weight_params[i]);
      assert (it != param_map.end());
      Uint n = it->second->tconv == "double" ? 
         1 : ((vector<double>*)it->second->val)->size();
      if (weight_params[i] == "sparse") {
         for (Uint j = 0; j < n; ++j)
            weights[index++] = 0.0;
      } else
         index += n;
   }

   // append individual SparseModel weights, read from file

   vector<float> smwts;
   const FeatureGroup *sparseF = featureGroup("sparse");
   for (Uint i = 0; i < sparseF->args.size(); ++i) {
      SparseModel::readWeights(sparseF->args[i], DirName(configFile),
                               smwts, sparseModelAllowNonLocalWts);
      weights.insert(weights.end(), smwts.begin(), smwts.end());
   }
}


void CanoeConfig::setFeatureWeights(const vector<double>& weights)
{
   // this is very dumb:
   vector<double> wts;
   getFeatureWeights(wts);
   if (weights.size() != wts.size())
      error(ETFatal, "incoming weight vector doesn't match current number of canoe weights");

   vector<double>::const_iterator wp = weights.begin();
   map<string,ParamInfo*>::iterator it;
   for (Uint i = 0; i < weight_params.size(); ++i) {
      it = param_map.find(weight_params[i]);
      assert (it != param_map.end());
      if (it->second->tconv == "double") {
         *(double*)it->second->val = *wp++;
         it->second->set_from_config = true; // ugh, sorry!
      }
      else if (it->second->tconv == "doubleVect") {
         vector<double>& v = *(vector<double>*)it->second->val;
         for (Uint j = 0; j < v.size(); ++j)  v[j] = *wp++;
         if (!v.empty())
            it->second->set_from_config = true; // ugh, sorry!
      } else
         assert(false);
   }
}

void CanoeConfig::setSparseFeatureWeights(const vector<double>& weights)
{
   // set any SparseModel weights to 1, then set decoder model as usual

   vector<double> wts(weights);
   Uint index = 0;
   map<string,ParamInfo*>::const_iterator it;
   for (Uint i = 0; i < weight_params.size(); ++i) {
      it = param_map.find(weight_params[i]);
      assert (it != param_map.end());
      Uint n = it->second->tconv == "double" ? 
         1 : ((vector<double>*)it->second->val)->size();
      if (weight_params[i] == "sparse") {
         for (Uint j = 0; j < n; ++j)
            wts[index++] = 1.0;
      } else
         index += n;
   }
   wts.resize(index);
   setFeatureWeights(wts);

   // save individual SparseModel weights to file(s)
   vector<float> smwts(weights.begin() + index, weights.end());
   Uint os = 0;
   const FeatureGroup *sparseF = featureGroup("sparse");
   for (Uint i = 0; i < sparseF->args.size(); ++i)
      os += SparseModel::writeWeights(sparseF->args[i], DirName(configFile), smwts, os);
}

void CanoeConfig::write(ostream& configout, Uint what, bool pretty)
{
   configout << setprecision(precision);
   vector<ParamInfo>::iterator it;
   extendFileName efn(DirName(configFile));
   for (it = param_infos.begin(); it != param_infos.end(); ++it) {
      if ((what == 0 && !it->set_from_config) ||
            (what == 1 && !it->set_from_config && !it->set_from_cmdline))
         continue;
      if (it->tconv == "bool") { // boolean params are special case
         configout << "[" << (*(bool*)it->val ? "" : "no-")
                   << it->names[0] << "]" << endl;
      } else {        // normal param
         if (efn.remoteDirName != "." && it->set_from_config &&
             !(it->groups & ParamInfo::relative_path_modification) &&
             it->groups & ParamInfo::check_file_name) {
            string out_vals;
            if (it->tconv == "stringVect") {
               vector<string>& v = *((vector<string>*)(it->val));
               vector<string> save(v);
               for_each(v.begin(), v.end(), efn);
               out_vals = it->get(pretty);
               v.swap(save);
            } else if (it->tconv == "string") {
               string& f = *(string*)(it->val);
               string save(f);
               efn(f);
               out_vals = it->get(pretty);
               f.swap(save);
            } else {
               out_vals = it->get(pretty);
            }
            configout << "[" << it->names[0] << "] " << out_vals << endl;
         } else
            configout << "[" << it->names[0] << "] " << it->get(pretty) << endl;
      }
   }
}

bool& CanoeConfig::readStatus(const string& param)
{
   map<string,ParamInfo*>::iterator it = param_map.find(param);
   if (it == param_map.end())
      error(ETFatal, "no such parameter: %s", param.c_str());
   return it->second->set_from_config;
}


bool CanoeConfig::prime(bool full)
{
   typedef vector<string>::iterator  IT;

   for ( Uint i = 0; i < LDMFiles.size(); ++i )
      if (isSuffix(".tpldm", LDMFiles[i])) {
         cerr << "\tPriming: " << LDMFiles[i] << endl;  // SAM DEBUGGING
         gulpFile(LDMFiles[i] + "/trg.repos.dat");
         gulpFile(LDMFiles[i] + "/cbk");
         if (full) gulpFile(LDMFiles[i] + "/tppt");
      }

   for ( Uint i = 0; i < allNonMultiProbPTs.size(); ++i ) {
      PhraseTableFeature::prime(allNonMultiProbPTs[i], full);
   }

   for (IT it=lmFiles.begin(); it!=lmFiles.end(); ++it) {
      PLM::prime(*it, full);
   }

   FeatureGroup *nnjms = featureGroup("nnjm");
   for (IT it=nnjms->args.begin(); it != nnjms->args.end(); ++it){
      NNJM::prime(*it, true);
   }

   FeatureGroup *bilm = featureGroup("bilm");
   for (IT it=bilm->args.begin(); it != bilm->args.end(); ++it){
      BiLMModel::prime(*it, true);
   }

   FeatureGroup *sparse = featureGroup("sparse");
   for (IT it=sparse->args.begin(); it != sparse->args.end(); ++it){
      SparseModel::prime(*it, DirName(configFile));
   }

   return true;
}


const string CanoeConfig::random_param::default_value = "U(-1.0,1.0)";

rnd_distribution* CanoeConfig::random_param::get(Uint index) const
{
   // if the index is greater than the vector size => the user didn't give
   // enough distribution and we are going to fallback on the default one.
   //if (index >= size()) error(ETWarn, "decoder feature missing random distribution");
   const string s(index < size() ? (*this)[index] : default_value);

   // An empty string representation of a distribution is replaced by the default value.
   if (s.empty()) return new uniform(-1.0, 1.0);

   // Parse and create the random distribution
   distribution_grammar  gram;
   if (!parse(s.c_str(), gram, space_p).full) {
      error(ETFatal, "Unable to parse the following distribution: %s", s.c_str());
   }

   return gram.rnd;
}

