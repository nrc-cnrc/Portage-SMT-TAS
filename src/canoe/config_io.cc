/**
 * @author George Foster
 * @file config_io.cc  Implementation of CanoeConfig.
 *
 * $Id$ * 
 * Read and write canoe config files
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "config_io.h"
#include <str_utils.h>
#include <phrasetable.h>
#include <iomanip>
#include <sstream>
#include <logging.h>
#include <randomDistribution.h>
#include <lm.h>
#include <boost/spirit.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/bind.hpp>
#include <cmath>

using namespace Portage;
using namespace boost::spirit;

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
      /// @param self  grammar object to be able to refere to it if needs be.
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
   // canoe.cc, phrase_tm_align.cc, and portage_api.cc.  Change with care, and
   // consider whether you change should be global (here) or local (in one of
   // those places).

   //configFile;
   //forPhraseFiles;
   //backPhraseFiles;
   //multiProbTMFiles;
   //lmFiles;
   lmOrder                = 0;
   //distWeight.push_back(1.0);
   lengthWeight           = 0.0;
   //segWeight
   //ibm1FwdWeights
   //lmWeights;
   //transWeights;
   //forwardWeights;
   randomWeights          = false;
   randomSeed             = 0;
   phraseTableSizeLimit   = NO_SIZE_LIMIT;
   phraseTableThreshold   = 0;
   phraseTablePruneType   = "forward-weights";
   phraseTableLogZero     = LOG_ALMOST_0;
   maxStackSize           = 100;
   pruneThreshold         = 0.0001;
   covLimit               = 0;
   covThreshold           = 0.0;
   distLimit              = NO_MAX_DISTORTION;
   distLimitExt           = false;
   distPhraseSwap         = false;
   //distortionModel        = ("WordDisplacement");
   segmentationModel      = "none";
   obsoleteSegModelArgs   = "";
   //ibm1FwdFiles
   bypassMarked           = false;
   weightMarked           = 1;
   oov                    = "pass";
   tolerateMarkupErrors   = false;
   checkInputOnly         = false;
   trace                  = false;
   ffvals                 = false;
   masse                  = false;
   verbosity              = 1;
   latticeFilePrefix      = "";
   latticeOut             = false;
   nbestFilePrefix        = "";
   nbestSize              = 100;
   nbestOut               = false;
   firstSentNum           = 0;
   backwards              = false;
   loadFirst              = false;
   input                  = "-";
   bAppendOutput          = false;
   bLoadBalancing         = false;
   bCubePruning           = false;
   cubeLMHeuristic        = "incremental";
   futLMHeuristic         = "incremental";

   // Parameter information, used for input and output. NB: doesn't necessarily
   // correspond 1-1 with actual parameters, as one ParamInfo can set several
   // parameters:

   param_infos.push_back(ParamInfo("config f", "string", &configFile));
   param_infos.push_back(ParamInfo("ttable-file-t2s ttable-file-n2f", "stringVect", &forPhraseFiles,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("ttable-file-s2t ttable-file ttable-file-f2n", "stringVect", &backPhraseFiles,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("ttable-multi-prob", "stringVect", &multiProbTMFiles,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("lmodel-file", "stringVect", &lmFiles,
      ParamInfo::relative_path_modification | ParamInfo::lm_check_file_name));
   param_infos.push_back(ParamInfo("lmodel-order", "Uint", &lmOrder));
   // WEIGHTS
   param_infos.push_back(ParamInfo("weight-d d", "doubleVect", &distWeight));
   param_infos.push_back(ParamInfo("random-d rd", "stringVect", &rnd_distWeight));
   param_infos.push_back(ParamInfo("weight-w w", "double", &lengthWeight));
   param_infos.push_back(ParamInfo("random-w rw", "stringVect", &rnd_lengthWeight));
   param_infos.push_back(ParamInfo("weight-s sm", "doubleVect", &segWeight));
   param_infos.push_back(ParamInfo("random-s rsm", "stringVect", &rnd_segWeight));
   param_infos.push_back(ParamInfo("weight-ibm1-fwd ibm1f", "doubleVect", &ibm1FwdWeights));
   param_infos.push_back(ParamInfo("random-ibm1-fwd ribm1f", "stringVect", &rnd_ibm1FwdWeights));
   param_infos.push_back(ParamInfo("weight-l lm", "doubleVect", &lmWeights));
   param_infos.push_back(ParamInfo("random-l rlm", "stringVect", &rnd_lmWeights));
   param_infos.push_back(ParamInfo("weight-t tm", "doubleVect", &transWeights));
   param_infos.push_back(ParamInfo("random-t rtm", "stringVect", &rnd_transWeights));
   param_infos.push_back(ParamInfo("weight-f ftm", "doubleVect", &forwardWeights));
   param_infos.push_back(ParamInfo("random-f rftm", "stringVect", &rnd_forwardWeights));
   param_infos.push_back(ParamInfo("random-weights r", "bool", &randomWeights));
   param_infos.push_back(ParamInfo("seed", "Uint", &randomSeed));
   param_infos.push_back(ParamInfo("ttable-limit", "Uint", &phraseTableSizeLimit));
   param_infos.push_back(ParamInfo("ttable-threshold", "double", &phraseTableThreshold));
   param_infos.push_back(ParamInfo("ttable-prune-type", "string", &phraseTablePruneType)); 
   param_infos.push_back(ParamInfo("ttable-log-zero", "double", &phraseTableLogZero));
   param_infos.push_back(ParamInfo("stack s", "Uint", &maxStackSize));
   param_infos.push_back(ParamInfo("beam-threshold b", "double", &pruneThreshold));
   param_infos.push_back(ParamInfo("cov-limit", "Uint", &covLimit));
   param_infos.push_back(ParamInfo("cov-threshold", "double", &covThreshold));
   param_infos.push_back(ParamInfo("distortion-limit", "int", &distLimit));
   param_infos.push_back(ParamInfo("dist-limit-ext", "bool", &distLimitExt));
   param_infos.push_back(ParamInfo("dist-phrase-swap", "bool", &distPhraseSwap));
   param_infos.push_back(ParamInfo("distortion-model", "stringVect", &distortionModel));
   param_infos.push_back(ParamInfo("segmentation-model", "string", &segmentationModel));
   param_infos.push_back(ParamInfo("segmentation-args", "string", &obsoleteSegModelArgs));
   param_infos.push_back(ParamInfo("ibm1-fwd-file", "stringVect", &ibm1FwdFiles,
      ParamInfo::relative_path_modification | ParamInfo::check_file_name));
   param_infos.push_back(ParamInfo("bypass-marked", "bool", &bypassMarked));
   param_infos.push_back(ParamInfo("weight-marked", "double", &weightMarked));
   param_infos.push_back(ParamInfo("oov", "string", &oov));
   param_infos.push_back(ParamInfo("tolerate-markup-errors", "bool", &tolerateMarkupErrors));
   param_infos.push_back(ParamInfo("check-input-only", "bool", &checkInputOnly));
   param_infos.push_back(ParamInfo("trace t palign", "bool", &trace));
   param_infos.push_back(ParamInfo("ffvals", "bool", &ffvals));
   param_infos.push_back(ParamInfo("masse", "bool", &masse));
   param_infos.push_back(ParamInfo("verbose v", "Uint", &verbosity));
   param_infos.push_back(ParamInfo("lattice l", "lat", &latticeFilePrefix));
   param_infos.push_back(ParamInfo("nbest", "nb", &nbestFilePrefix));
   param_infos.push_back(ParamInfo("first-sentnum", "Uint", &firstSentNum));
   param_infos.push_back(ParamInfo("backwards", "bool", &backwards));
   param_infos.push_back(ParamInfo("load-first", "bool", &loadFirst));
   param_infos.push_back(ParamInfo("input", "string", &input));
   param_infos.push_back(ParamInfo("append", "bool", &bAppendOutput));
   param_infos.push_back(ParamInfo("cube-pruning", "bool", &bCubePruning));
   param_infos.push_back(ParamInfo("cube-lm-heuristic", "string", &cubeLMHeuristic));
   param_infos.push_back(ParamInfo("future-score-lm-heuristic", "string", &futLMHeuristic));
   param_infos.push_back(ParamInfo("lb", "bool", &bLoadBalancing));

   // List of all parameters that correspond to weights. ORDER IS SIGNIFICANT
   // and must match the order in BasicModelGenerator::InitDecoderFeatures().
   // New entries should be added immediately before "lm".

   const char* weight_names[] = {
      "d", "w", "sm", "ibm1f", "lm", "tm", "ftm"
   };
   weight_params.assign(weight_names, weight_names + ARRAY_SIZE(weight_names));

   for (Uint i = 0; i < param_infos.size(); ++i) {
      param_infos[i].c = this;
      string app(param_infos[i].tconv == "bool" ? "" : ":");
      for (Uint j = 0; j < param_infos[i].names.size(); ++j) {
         param_map[param_infos[i].names[j]] = &param_infos[0] + i;
	 param_list.push_back(param_infos[i].names[j] + app);
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
   } else if (tconv == "lat") {	// lattice params
      c->latticeFilePrefix = s;
      c->latticeOut = (c->latticeFilePrefix != "");
   } else if (tconv == "nb") {	// nbest list params
      vector<string> toks;
      split(s, toks, ":", 2);
      c->nbestFilePrefix = toks[0] == ALT_EMPTY_STRING ? "" : toks[0];
      if (toks.size() == 2) c->nbestSize = conv<Uint>(toks[1]);
      c->nbestOut = (c->nbestFilePrefix != "");
   } else
      error(ETFatal, "programmer error: cannot convert param %s - conversion method %s unknown", 
            names[0].c_str(), tconv.c_str());
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
      string s;
      vector<string>& v = *(vector<string>*)val;
      if (pretty) {
         for (vector<string>::iterator it = v.begin(); it != v.end(); ++it)
            ss << "\n   " << *it;
      } else
         ss << join(v.begin(), v.end(), s, ":");
   } else if (tconv == "UintVect") {
      vector<Uint>& v = *(vector<Uint>*)val;
      ss << join<Uint>(v.begin(), v.end(), ":");
   } else if (tconv == "doubleVect") {
      vector<double>& v = *(vector<double>*)val;
      ss << join<double>(v.begin(), v.end(), ":", precision);
   } else if (tconv == "lat") {	// lattice params
      ss << c->latticeFilePrefix;
   } else if (tconv == "nb") {	// nbest list params
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
      case done:		// keep compiler quiet
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
      if (!file.empty() && file[0] != '/')
         file = remoteDirName + "/" + file;
   }
};

void CanoeConfig::read(const char* configFile) {
   IMagicStream cfg(configFile);
   read(cfg);

   char* tmp = strdup(configFile);
   extendFileName efn(DirName(tmp));
   free(tmp);
   
   // Modifying filepath for canoe.relative
   if (efn.remoteDirName != ".") {
      // Look through param_info and see which one needs their path modified
      typedef vector<ParamInfo>::iterator  IT;
      for (IT it(param_infos.begin()); it!=param_infos.end(); ++it) {
         if (it->groups & ParamInfo::relative_path_modification) {
            if (it->tconv == "stringVect") {
               vector<string>& v = *((vector<string>*)(it->val));
               for_each(v.begin(), v.end(), efn);
               if (false) copy(v.begin(), v.end(), ostream_iterator<string>(cerr, "\n"));
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

void CanoeConfig::check()
{
   // Before checking the integrity of the config, we must make sure all files
   // are valid.
   check_all_files();

   // Set defaults:

   if (distortionModel.empty())
      distortionModel.push_back("WordDisplacement");
   else if (distortionModel[0] == "none")
      distortionModel.clear();

   if (distWeight.empty() && !distortionModel.empty())
      distWeight.assign(distortionModel.size(), 1.0);

   // Check if the user didn't write in his config none#somearg which is
   // actually simply none
   if (isPrefix("none#", segmentationModel.c_str()))
      segmentationModel = "none";
   if (segWeight.empty() && segmentationModel != "none")
      segWeight.push_back(1.0);

   if (ibm1FwdWeights.empty())
      ibm1FwdWeights.insert(ibm1FwdWeights.end(), ibm1FwdFiles.size(), 1.0);

   if (lmWeights.empty())
      lmWeights.insert(lmWeights.end(), lmFiles.size(), 1.0);

   const Uint multi_prob_model_count(getTotalMultiProbModelCount());

   if (transWeights.empty())
      transWeights.assign(backPhraseFiles.size() + multi_prob_model_count, 1.0);

   // Errors:
   if (bLoadBalancing && bAppendOutput)
      error(ETFatal, "Load Balancing cannot run in append mode");

   //if (latticeOut && nbestOut)
   //   error(ETFatal, "Lattice and nbest output cannot be generated simultaneously.");

   if (lmFiles.empty())
      error(ETFatal, "No language model file specified.");
   if (backPhraseFiles.empty() && multiProbTMFiles.empty())
      error(ETFatal, "No phrase table file specified.");

   if (distWeight.size() != distortionModel.size())
      error(ETFatal, "Number of distortion models does not match number of distortion model weights.");
   if (segWeight.size() > 0 && segmentationModel == "none") {
      error(ETWarn, "You can't specify a segmentation weight when using no segmentation model - ignoring it");
      segWeight.clear();
   }
   if (obsoleteSegModelArgs != "")
      error(ETFatal, "-segmentation-args is an obsolete option - use -segmentation-model model#args instead.");

   if (ibm1FwdFiles.size() != ibm1FwdWeights.size())
      error(ETFatal, "number of IBM1 forward weights does not match number of IBM forward model files");

   if (lmWeights.size() != lmFiles.size())
      error(ETFatal, "Number of language model weights does not match number of language model files.");
   if (transWeights.size() != backPhraseFiles.size() + multi_prob_model_count)
      error(ETFatal, "Number of translation model weights does not match number of translation model files.");
   if (forPhraseFiles.size() > 0 && forPhraseFiles.size() != backPhraseFiles.size())
      error(ETFatal, "Number of forward translation models !=  number of backward translation model files.");
   if (forwardWeights.size() > 0 && forwardWeights.size() != forPhraseFiles.size() + multi_prob_model_count)
      error(ETFatal, "Number for forward translation model weights != number of (forward) translation model files.");
   if (oov != "pass" && oov != "write-src-marked" && oov != "write-src-deleted")
      error(ETFatal, "OOV handling method must be one: 'pass', 'write-src-marked', or 'write-src-deleted'");
   if (phraseTablePruneType != "forward-weights" &&
       phraseTablePruneType != "backward-weights" &&
       phraseTablePruneType != "combined")
      error(ETFatal, "ttable-prune-type must be one of: 'forward-weights', 'backward-weights', or 'combined'");
   
   // if (bypassMarked && weightMarked)
   //   error(ETWarn, "Both -bypass-marked and -weight-marked found.  Only doing -bypass-marked");
   if (phraseTableSizeLimit != NO_SIZE_LIMIT && forPhraseFiles.empty() && multiProbTMFiles.empty())
      error(ETWarn, "Doing phrase table pruning without forward translation model.");

   if ( bCubePruning )
      if ( covLimit != 0 || covThreshold != 0.0 )
         error(ETWarn, "Coverage pruning is not implemented yet in the cube pruning decoder, ignoring -cov-* options.");

   if ( cubeLMHeuristic != "none" && cubeLMHeuristic != "unigram" && cubeLMHeuristic != "incremental" && cubeLMHeuristic != "simple" )
      error(ETFatal, "cube-lm-heuristic must be one of: 'none', 'unigram', 'incremental', or 'simple'");

   if ( futLMHeuristic != "none" && futLMHeuristic != "unigram" && futLMHeuristic != "incremental" && futLMHeuristic != "simple" )
      error(ETFatal, "future-score-lm-heuristic must be one of: 'none', 'unigram', 'incremental', or 'simple'");


   // Checking weights' value for valid numerical values
   vector<double> wts;
   getFeatureWeights(wts);
   for (Uint i(0); i<wts.size(); ++i)
      if (!isfinite(wts[i]))
         error(ETFatal, "A weight has a non-finite value (id:%d, value:%f).",
               i, wts[i]);
}

void CanoeConfig::check_all_files() const
{
   bool ok = true;
   
   // Look through param_info and see which ones need their path modified
   typedef vector<ParamInfo>::const_iterator  IT;
   for (IT it(param_infos.begin()); it!=param_infos.end(); ++it) {
      if (it->groups & ParamInfo::check_file_name) {
         if (it->tconv == "stringVect") {
            vector<string>& v = *((vector<string>*)(it->val));
            for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
               if (!check_if_exists(*f) && f->find("<src>")>=f->size()) {
                  cerr << "can't access: " << *f << endl;
                  ok = false;
               }
            }
         }
         else if (it->tconv == "string") {
            const string& f = *(string*)(it->val);
            if (!f.empty() && !check_if_exists(f) && f.find("<src>")>=f.size()) {
               cerr << "can't access: " << f << endl;
               ok = false;
            }
         }
      }
      else if (it->groups & ParamInfo::lm_check_file_name) {
         assert(it->tconv == "stringVect");
         vector<string>& v = *((vector<string>*)(it->val));
         for (vector<string>::const_iterator f(v.begin()); f!=v.end(); ++f) {
            if (!PLM::check_file_exists(*f)) {
               cerr << "can't access lm: " << *f << endl;
               ok = false;
            }
         }
      }
   }
   
   // On error, exit with error status so this check can be used in scripts
   if (!ok) exit(1);
}

Uint CanoeConfig::getTotalMultiProbModelCount() const
{
   Uint multi_prob_column_count = 0;
   for (Uint i = 0; i < multiProbTMFiles.size(); ++i) {
      multi_prob_column_count +=
         PhraseTable::countProbColumns(multiProbTMFiles[i].c_str());
      if ( multi_prob_column_count % 2 != 0 )
         error(ETFatal, "Uneven number of probability columns in ttable-multi-prob file %s.",
               multiProbTMFiles[i].c_str());
   }
   return multi_prob_column_count / 2;
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

void CanoeConfig::setFeatureWeights(const vector<double>& weights)
{
   // this is very dumb
   vector<double> wts;
   getFeatureWeights(wts);
   if (weights.size() != wts.size())
      error(ETFatal, "incoming weight vector doesn't match current number of canoe weights");

   vector<double>::const_iterator wp = weights.begin();
   map<string,ParamInfo*>::iterator it;
   for (Uint i = 0; i < weight_params.size(); ++i) {
      it = param_map.find(weight_params[i]);
      assert (it != param_map.end());
      if (it->second->tconv == "double")
         *(double*)it->second->val = *wp++;
      else if (it->second->tconv == "doubleVect") {
         vector<double>& v = *(vector<double>*)it->second->val;
	 for (Uint j = 0; j < v.size(); ++j)  v[j] = *wp++;
      } else
         assert(false);
   }
}

void CanoeConfig::write(ostream& configout, Uint what, bool pretty)
{
   configout << setprecision(precision);
   vector<ParamInfo>::iterator it;
   for (it = param_infos.begin(); it != param_infos.end(); ++it) {
      if (what == 0 && !it->set_from_config || 
	  what == 1 && !it->set_from_config && !it->set_from_cmdline)
	 continue;
      if (it->tconv == "bool") { // boolean params are special case
	 if (*(bool*)it->val) configout << "[" << it->names[0] << "]" << endl;
      } else			// normal param
	 configout << "[" << it->names[0] << "] " << it->get(pretty) << endl;
   }
}

bool& CanoeConfig::readStatus(const string& param)
{
   map<string,ParamInfo*>::iterator it = param_map.find(param);
   if (it == param_map.end())
      error(ETFatal, "no such parameter: %s", param.c_str());
   return it->second->set_from_config;
}


rnd_distribution* CanoeConfig::random_param::get(const Uint index) const
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

