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

using namespace Portage;

CanoeConfig::CanoeConfig() {

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
   distWeight.push_back(1.0);
   lengthWeight           = 0.0;
   // segWeight
   //lmWeights;
   //transWeights;
   //forwardWeights;
   randomWeights          = false;
   randomSeed             = 0;
   phraseTableSizeLimit   = NO_SIZE_LIMIT;
   phraseTableThreshold   = 0;
   phraseTablePruneType   = "forward-weights";
   maxStackSize           = 100;
   pruneThreshold         = 0.0001;
   covLimit               = 0;
   covThreshold           = 0.0;
   distLimit              = NO_MAX_DISTORTION;
   distModelName          = "WordDisplacement";
   distModelArg           = "";
   segModelName           = "none";
   segModelArgs           = "";
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

   // Parameter information, used for input and output. NB: doesn't necessarily
   // correspond 1-1 with actual parameters, as one ParamInfo can set several
   // parameters:

   param_infos.push_back(ParamInfo("config f", "string", &configFile));
   param_infos.push_back(ParamInfo("ttable-file-t2s ttable-file-n2f", "stringVect", &forPhraseFiles));
   param_infos.push_back(ParamInfo("ttable-file-s2t ttable-file ttable-file-f2n", "stringVect", &backPhraseFiles));
   param_infos.push_back(ParamInfo("ttable-multi-prob", "stringVect", &multiProbTMFiles));
   param_infos.push_back(ParamInfo("lmodel-file", "stringVect", &lmFiles));
   param_infos.push_back(ParamInfo("lmodel-order", "Uint", &lmOrder));
   param_infos.push_back(ParamInfo("weight-d d", "doubleVect", &distWeight));
   param_infos.push_back(ParamInfo("weight-w w", "double", &lengthWeight));
   param_infos.push_back(ParamInfo("weight-s sm", "doubleVect", &segWeight));
   param_infos.push_back(ParamInfo("weight-l lm", "doubleVect", &lmWeights));
   param_infos.push_back(ParamInfo("weight-t tm", "doubleVect", &transWeights));
   param_infos.push_back(ParamInfo("weight-f ftm", "doubleVect", &forwardWeights));
   param_infos.push_back(ParamInfo("random-weights r", "bool", &randomWeights));
   param_infos.push_back(ParamInfo("seed", "Uint", &randomSeed));
   param_infos.push_back(ParamInfo("ttable-limit", "Uint", &phraseTableSizeLimit));
   param_infos.push_back(ParamInfo("ttable-threshold", "double", &phraseTableThreshold));
   param_infos.push_back(ParamInfo("ttable-prune-type", "string", &phraseTablePruneType)); 
   param_infos.push_back(ParamInfo("stack s", "Uint", &maxStackSize));
   param_infos.push_back(ParamInfo("beam-threshold b", "double", &pruneThreshold));
   param_infos.push_back(ParamInfo("cov-limit", "Uint", &covLimit));
   param_infos.push_back(ParamInfo("cov-threshold", "double", &covThreshold));
   param_infos.push_back(ParamInfo("distortion-limit", "int", &distLimit));
   param_infos.push_back(ParamInfo("distortion-model", "dm", &distModelName));
   param_infos.push_back(ParamInfo("segmentation-model", "string", &segModelName));
   param_infos.push_back(ParamInfo("segmentation-args", "string", &segModelArgs));
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

   // List of all parameters that correspond to weights. ORDER IS SIGNIFICANT
   // and must match the order in BasicModelGenerator::InitDecoderFeatures().
   // New entries should be added immediately before "lm".

   char* weight_names[] = {"d", "w", "sm", "lm", "tm", "ftm"};
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
   } else if (tconv == "dm") {	// distortion model params
      vector<string> toks;
      split(s, toks, ":", 2);
      c->distModelName = toks[0];
      if (toks.size() == 2) c->distModelArg = toks[1];
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

string CanoeConfig::ParamInfo::get() {
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
      ss << join(v.begin(), v.end(), s, ":");
   } else if (tconv == "UintVect") {
      vector<Uint>& v = *(vector<Uint>*)val;
      ss << join<Uint>(v.begin(), v.end(), ":");
   } else if (tconv == "doubleVect") {
      vector<double>& v = *(vector<double>*)val;
      ss << join<double>(v.begin(), v.end(), ":", precision);
   } else if (tconv == "dm") {	// distortion model params
      ss << c->distModelName;
      if (c->distModelArg != "") ss << ":" << c->distModelArg;
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
   // Set defaults:

   if (distWeight.empty() && distModelName != "none")
      distWeight.push_back(1.0);

   if (segWeight.empty() && segModelName != "none")
      segWeight.push_back(1.0);

   if (lmWeights.empty())
      lmWeights.insert(lmWeights.end(), lmFiles.size(), 1.0);

   const Uint multi_prob_model_count(getTotalMultiProbModelCount());

   if (transWeights.empty())
      transWeights.assign(backPhraseFiles.size() + multi_prob_model_count, 1.0);

   // Errors:

   if (latticeOut && nbestOut)
      error(ETFatal, "Lattice and nbest output cannot be generated simultaneously.");

   if (lmFiles.empty())
      error(ETFatal, "No language model file specified.");
   if (backPhraseFiles.empty() && multiProbTMFiles.empty())
      error(ETFatal, "No phrase table file specified.");

   if (distWeight.size() > 0 && distModelName == "none") {
      error(ETWarn, "You can't specify a distortion weight when using no distortion model - ignoring it");
      distWeight.clear();
   }
   if (segWeight.size() > 0 && segModelName == "none") {
      error(ETWarn, "You can't specify a segmentation weight when using no segmentation model - ignoring it");
      segWeight.clear();
   }

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
}

void CanoeConfig::check_all_files() const
{
   bool ok = true;
   vector<string> allfiles;
   allfiles.insert(allfiles.end(), backPhraseFiles.begin(), backPhraseFiles.end());
   allfiles.insert(allfiles.end(), forPhraseFiles.begin(), forPhraseFiles.end());
   allfiles.insert(allfiles.end(), multiProbTMFiles.begin(), multiProbTMFiles.end());
   allfiles.insert(allfiles.end(), lmFiles.begin(), lmFiles.end());
   for (Uint i = 0; i < allfiles.size(); ++i) {
      if (!check_if_exists(allfiles[i])) {
         cerr << "can't access: " << allfiles[i] << endl;
         ok = false;
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

void CanoeConfig::write(ostream& configout, Uint what)
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
	 configout << "[" << it->names[0] << "] " << it->get() << endl;
   }
}

bool& CanoeConfig::readStatus(const string& param)
{
   map<string,ParamInfo*>::iterator it = param_map.find(param);
   if (it == param_map.end())
      error(ETFatal, "no such parameter: %s", param.c_str());
   return it->second->set_from_config;
}

