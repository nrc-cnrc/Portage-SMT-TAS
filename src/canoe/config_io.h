/**
 * @author George Foster
 * @file config_io.h  Read and write canoe config files, and perform error
 * checking and parameter initialization on them.
 *
 * $Id$
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, 2006, Conseil national de recherches du Canada / National Research Council of Canada
 */

#ifndef CONFIG_IO_H
#define CONFIG_IO_H

#include <map>
#include <arg_reader.h>
#include <canoe_general.h>
#include <errors.h>
#include <file_utils.h>

namespace Portage
{

/**
 * @brief Class to represent and manipulate canoe's parameters.
 *
 * To add a new argument to canoe:
 *
 * 1) Add a parameter member to the set below.
 *
 * 2) Initialize the parameter with a default value in the CanoeConfig
 *    constructor.
 *
 * 3) Add a corresponding ParamInfo object to the param_infos list in the
 *    CanoeConfig constructor. See examples for existing parameters.
 *
 * 4) If the parameter is a loglinear weight, add its name (or an alias) to the
 *    weight_params list in the CanoeConfig constructor. Weights for features
 *    that can be omitted from the model MUST be vectors, even if they take at
 *    most one value. (Empty vectors are used to indicate omission.)
 *
 * 5) If the parameter is not a standard type (bool, int, Uint, double, string
 *    and vectors of Uint, double, and string), or if it has a special
 *    interpretation, then assign it a new ParamInfo::tconv value, and add
 *    handling for this value in ParamInfo::get() and ParamInfo::set(), to
 *    convert its value(s) to and from a string argument. See the "nbest"
 *    parameter for an example.
 *
 * 6) If necessary, add error checking to CanoeConfig::check().
 *
 * 7) Add a description of the argument to the help message in canoe_help.h
 *
 * Once this is done, the parameter may be specified either on canoe's command
 * line, or in a config file, or both. It will also automatically work with
 * cow.sh and rat.sh.  It will also work with any other utility that reads
 * canoe.ini files.
 *
 * Note: binary arguments are special: they are set to true by including the
 * corresponding parameter in the config file or command line, with no
 * argument.  Not including the parameter leaves the value false.
 *
 * Note that the members are only documented briefly here.  See canoe_help.h or
 * type "canoe -h" for more details on the meaning of most of these parameters.
 */
class CanoeConfig {

public:

   // Parameters:

   string configFile;               ///< Name of the canoe config file
   vector<string> forPhraseFiles;   ///< Forward phrase table file names
   vector<string> backPhraseFiles;  ///< Backward phrase table file names
   vector<string> multiProbTMFiles; ///< Multi-prob phrase table file names
   vector<string> lmFiles;          ///< Language model file names
   Uint lmOrder;                    ///< Maximum LM order (0 == no limit)
   vector<double> distWeight;       ///< Distortion model weight
   double lengthWeight;             ///< Length penalty weight
   vector<double> segWeight;        ///< Segmentation model weight
   vector<double> lmWeights;        ///< Language model weights
   vector<double> transWeights;     ///< Translation model weights
   vector<double> forwardWeights;   ///< Forward translation model weights
   bool randomWeights;              ///< true == use rnd weights for each sent.
   Uint randomSeed;                 ///< Seed for randomWeights
   Uint phraseTableSizeLimit;       ///< Num target phrases per source phrase
   double phraseTableThreshold;     ///< Prob threshold for pruning PTs.
   string phraseTablePruneType;     ///< Which probs to use for pruning PTs
   Uint maxStackSize;               ///< s = stack size limit
   double pruneThreshold;           ///< b = prob-based stack pruning threshold
   Uint covLimit;                   ///< Coverage pruning limit
   double covThreshold;             ///< Coverage pruning prob threshold
   int distLimit;                   ///< Distortion limit
   string distModelName;            ///< Distortion model name
   string distModelArg;             ///< Distortion model argument string
   string segModelName;             ///< Segmentation model name
   string segModelArgs;             ///< Segmentation model argument string
   bool bypassMarked;               ///< Look in PT even for marked trans
   double weightMarked;             ///< Constant discount for marked probs
   string oov;                      ///< OOV handling method
   bool tolerateMarkupErrors;       ///< Whether to proceed despite markup err
   bool trace;                      ///< Whether to output alignment info
   bool ffvals;                     ///< Whether to output feature fn values
   bool masse;                      ///< Whether to output total lattice weight
   Uint verbosity;                  ///< Verbosity level
   string latticeFilePrefix;        ///< Prefix for all lattice output files
   bool latticeOut;                 ///< Whether to output the lattices
   string nbestFilePrefix;          ///< Prefix for all n-best output files
   Uint nbestSize;                  ///< Number of hypotheses in n-best lists
   bool nbestOut;                   ///< Whether to output n-best hypotheses
   Uint firstSentNum;               ///< Index of the first input sentence
   bool backwards;                  ///< Whether to translate backwards
   bool loadFirst;                  ///< Whether to load models before input

   /**
    * Constructor, sets default parameter values.
    */
   CanoeConfig();

   /**
    * Read parameters from a config stream.
    *
    * Input format is parameter name, enclosed in square brackets, followed by
    * a whitespace, then a value. For boolean parameters the value is omitted.
    * This will silently overwrite any existing values.
    * @param configin  the input stream containing the configuration.
    */
   void read(istream& configin);
   /**
    * Read parameters from a config file.
    *
    * See read(istream& configin) for details.
    * @param configFile  file name
    */
   void read(const char* configFile) {
      IMagicStream cfg(configFile);
      read(cfg);
   }

   /**
    * Get a list of parameters in ArgReader format.
    * @return list of parameters (each followed by a colon unless boolean).
    */
   const vector<string>& getParamList() const { return param_list; }

   /**
    * Set parameters from an ArgReader object.
    * Switches not corresponding to CanoeConfig parameters are ignored.
    * @param arg_reader Object containing command-line switches.
    */
   void setFromArgReader(ArgReader& arg_reader);

   /**
    * Check parameter values and die if not ok.  Must be called after setting
    * parameters.  Also sets default weights if not specified.
    */
   void check();

   /**
    * Check that all files listed in the parameter values are readable.
    * Call this to provide early error messages, after all file names are
    * fixed.  Dies with an error message if any model file is not readable.
    */
   void check_all_files() const;

   /**
    * Count the total number of translation models in multi-prob tables.
    * Dies if any table has an odd number of probability columns.
    * @return the number of models in files listed under ttable-multi-prob.
    */
   Uint getTotalMultiProbModelCount() const;

   /**
    * Get current feature weights in the form of an argument string, eg:
    * "-d .5 -w 0.8 -lm 1 -tm 0.5"
    * @param s string which will contain the result
    * @return reference to s
    */
   string& getFeatureWeightString(string& s) const;

   /**
    * Set current feature weights from an argument string.
    *
    * Options in the string that aren't weights are ignored with a warning.
    * @param s parameter string, as returned by getFeatureWeightString().
    */
   void setFeatureWeightsFromString(const string& s);

   /**
    * Get current feature weights, in the order written to ffvals.
    * @param weights vector to receive the weights
    */
   void getFeatureWeights(vector<double>& weights) const;

   /**
    * Set current feature weights, in the order written to ffvals.
    * @param weights vector with the weights
    */
   void setFeatureWeights(const vector<double>& weights);

   /**
    * Write parameters to stream.
    *
    * The output is written in the format expected by read().
    * Boolean parameters whose value is false are not written.
    * @param configout output stream
    * @param what specifies what gets written: 0 = only parameters set in
    * config file, 1 = parameters set in config file or on command line,
    * 2 = all params.
    */
   void write(ostream& configout, Uint what=0);
   /**
    * Write parameters to file.
    *
    * See write(ostream& configout, Uint what) for details.
    * @param cfgFile output file name
    * @param what specifies what gets written: 0 = only parameters set in
    * config file, 1 = parameters set in config file or on command line,
    * 2 = all params.
    */
   void write(const char *cfgFile, Uint what=0) {
      OMagicStream cfg(cfgFile);
      write(cfg, what);
   }

   /**
    * Return the 'read' status associated with a parameter.
    * @param param Name of the parameter
    * @return modifiable reference to the 'read' status of param.  Assigning to
    *         the return value will modify the parameter's status.
    */
   bool& readStatus(const string& param);


private:

   // No, this is not the most elegant way to handle generics...
   /// Generic structure for handling parameters of various types.
   struct ParamInfo {
      CanoeConfig* c;        ///< Pointer to parent CanoeConfig object
      vector<string> names;  ///< param name followed by 0 or more alternates
      string tconv;          ///< how to convert to/from string
      void* val;             ///< pointer to current value
      bool set_from_config;  ///< true if set from config file
      bool set_from_cmdline; ///< true if set from command line

      /**
       * Constructor.
       * @param name_strings  name for the parameter
       * @param tconv         type for the parameter
       * @param val           value of the parameter
       */
      ParamInfo(const string name_strings, const string& tconv, void* val) :
         tconv(tconv), val(val),
         set_from_config(false), set_from_cmdline(false) {
            split(name_strings, names);
         }

      /// Set value from string.
      /// @param s  new value
      void set(const string& s);
      /// Get string representation of current value
      /// @return Returns a string representation of current value.
      string get();
   };

   static const Uint precision = 10; ///< significant digits for weights

   vector<ParamInfo> param_infos;    ///< main parameter list
   vector<string> weight_params;     ///< names of params that correspond to weights
   map<string,ParamInfo*> param_map; ///< map : name -> param info
   vector<string> param_list;        ///< list of parameters for ArgReader
};


}

#endif
