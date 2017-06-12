/**
 * @author George Foster
 * @file config_io.h  Read and write canoe config files, and perform error
 * checking and parameter initialization on them.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, 2006, Her Majesty in Right of Canada
 */

#ifndef CONFIG_IO_H
#define CONFIG_IO_H

#include <map>
#include "canoe_general.h"
#include "errors.h"
#include "file_utils.h"
#include "vector_map.h"

namespace Portage
{

class ArgReader; // No need to actually include arg_reader.h here.
class rnd_distribution; // No need to actually include randomDistribution.h here.

/**
 * @brief Class to represent and manipulate canoe's parameters.
 *
 * To add a new argument to canoe:
 *
 * If the new argument is a feature in the log-linear model:
 *  1) Add its short, long and group name to weight_names_other in config_io.cc
 *
 *  2) If the feature only has a weight, set its need_args value to false; if
 *     it also has a string argument, add a corresponding ParamInfo object to
 *     the param_infos list (see 3 below for more details, and existing
 *     features for examples).  Note that all log-linear weights are now
 *     doubleVect, and their arguments are all stringVect.  Restrictions on the
 *     number of instances are enforced in CanoeConfig::check().
 *
 * If the new argument is not a log-linear feature:
 *  1) Add a parameter member to the set below.
 *
 *  2) Initialize the parameter with a default value in the CanoeConfig
 *     constructor.
 *
 * Either way:
 *  3) Add a corresponding ParamInfo object to the param_infos list in the
 *     CanoeConfig constructor. See examples for existing parameters.  Be sure
 *     to flag your new option in the proper group, for example the LMs and TMs
 *     filename must be modified to be able to run canoe.relative thus they are
 *     flagged with relative_path_modification so they can be modified later to
 *     their correct path.
 *
 *  4) If the parameter is not a standard type (bool, int, Uint, double, string
 *     and vectors of Uint, double, and string), or if it has a special
 *     interpretation, then assign it a new ParamInfo::tconv value, and add
 *     handling for this value in ParamInfo::get() and ParamInfo::set(), to
 *     convert its value(s) to and from a string argument. See the "nbest"
 *     parameter for an example.
 *
 *  5) If necessary, add error checking to CanoeConfig::check().
 *
 *  6) If necessary, set default values in CanoeConfig::check(): simple
 *     constant default values are set in the CanoeConfig constructor, but
 *     default values that depend on other parameters are set in check().
 *
 *  7) Add a description of the argument to the help message in canoe_help.h
 *
 * Once this is done, the parameter may be specified either on canoe's command
 * line, or in a config file, or both. It will also automatically work with
 * cow.sh and rat.sh.  It will also work with any other utility that reads
 * canoe.ini files.
 *
 * Note: binary arguments are special: they are set to true by including the
 * corresponding parameter in the config file or command line, with no
 * argument.  Not including the parameter, or prefixing it with "no-", leaves
 * the value false.
 *
 * Note that the members are only documented briefly here.  See canoe_help.h or
 * type "canoe -h" for more details on the meaning of most of these parameters.
 */
class CanoeConfig {
private:
   /**
    * This keeps track of the string representation of the random distribution.
    * It also creates the proper random distribution from the string
    * representation.  If no string is available for an index, automagically
    * this object will create a default random distribution.
    */
   class random_param : public vector<string>
   {
      private:
         /// Default random distribution's string representation
         static const string default_value; // = "U(-1.0,1.0)";
      public:
         /**
          * Converts the internal string representation to the proper random
          * distribution.  If user didn't specify all random distribution, then
          * this will return a default predefined random distribution.
          * Note that for better randomness each returned distribution should be seed.
          * Note that this returns a new object each time that needs to be deleted.
          * @param  index of random distriubtion's string to convert.
          * @return  Returns a random distribution object.
          */
         rnd_distribution* get(Uint index) const;
   };

public:

   /// Groups together the variables needed to describe a feature: its name,
   /// weight name, weights, random weights, feature names.
   struct FeatureGroup {
      const string shortname;   ///< short name of the feature's weight option
      const string group;       ///< long name of the feature group, for DecoderFeature::create()
      vector<double> weights;   ///< weights for this feature
      random_param rnd_weights; ///< random weight parameters for this feature
      bool need_args;           ///< whether the feature is triggered by its args or its weights (default: yes)
      vector<string> args;      ///< string(s) providing the feature argument(s)

      /// Constructor 
      FeatureGroup(const string& shortname, const char* group)
         : shortname(shortname), group(group), need_args(true) {}
      /// Return weather the config has no features if this group
      bool empty() const { return weights.empty(); }
      /// Return how many feature of this group the config has
      Uint size() const { return weights.size(); }
   };

   /// The FeatureGroupMap could simply be a typedef on its baseclass, but
   /// we want the copy construction, assignment operator and destructor to
   /// work deeply, so the default functions for CanoeConfig itself are deep too.
   class FeatureGroupMap : public vector_map<string, FeatureGroup*> {
      typedef vector_map<string, FeatureGroup*> baseclass;
      FeatureGroupMap& operator=(const FeatureGroupMap&); // intentially not implemented
   public:
      FeatureGroupMap() {}
      FeatureGroupMap(const FeatureGroupMap& x) : baseclass(x) {
         for (baseclass::iterator it = begin(); it != end(); ++it)
            it->second = new FeatureGroup(*it->second);
      }
      ~FeatureGroupMap() {
         for (baseclass::iterator it = begin(); it != end(); ++it)
            delete it->second;
      }
   };

   // Parameters:

   string configFile;               ///< Name of the canoe config file
   vector<string> multiProbTMFiles; ///< Multi-prob phrase table file names
   vector<string> tpptFiles;        ///< TPPT phrase table specified via -ttable-tppt
   vector<string> TTables;          ///< TTables specified via -ttable
   /// TMs that are subclasses of PhraseTableFeature, i.e., all but multiProbTMFiles.
   /// allNonMultiProbPTs = tpptFiles + dynpt + TTables (initialized in check())
   vector<string> allNonMultiProbPTs;
   vector<string> LDMFiles;         ///< Lexicalized distortion model file names
   vector<string> lmFiles;          ///< Language model file names
   string vocFile;                  ///< Vocabulary file name
   Uint lmOrder;                    ///< Maximum LM order (0 == no limit)
   int maxLmContextSize;            ///< Maximum LM context from history (-1 == no limit)
   bool minimizeLmContextSize;      ///< Keep as little context as needed for future queries
   string nbestProcessor;           ///< A script that will be invoked for all nbest.

   // WEIGHTS
   vector<double> lmWeights;        ///< Language model weights
   random_param rnd_lmWeights;      ///< Language model weights
   vector<double> transWeights;     ///< Translation model weights
   random_param rnd_transWeights;   ///< Translation model weights
   vector<double> forwardWeights;   ///< Forward translation model weights
   random_param rnd_forwardWeights; ///< Forward translation model weights
   vector<double> adirTransWeights; ///< Adirectional translation model weights //boxing
   random_param rnd_adirTransWeights;///< Adirectional translation model weights //boxing

   /// The map holding all the other feature weights and models et al
   FeatureGroupMap features;

   // Rule decoder feature arguments
   vector<string> rule_classes;     ///< Rule classes' name
   vector<double> rule_log_zero;    ///< Rule classes' log zero value

   bool randomWeights;              ///< true == use rnd weights for each sent.
   Uint randomSeed;                 ///< Seed for randomWeights
   string sentWeights;              ///< Filename for sentence-specific weights.
   Uint phraseTableSizeLimit;       ///< Num target phrases per source phrase
   double phraseTableThreshold;     ///< Prob threshold for pruning PTs.
   string phraseTablePruneType;     ///< Which probs to use for pruning PTs
   double phraseTableLogZero;       ///< Logprob for missing or 0-prob PT entries
   Uint maxStackSize;               ///< s = stack size limit for cube pruning decoding
   Uint maxRegularStackSize;        ///< rs = stack size limit for regular decoding
   double pruneThreshold;           ///< b = prob-based stack pruning threshold
   Uint covLimit;                   ///< Coverage pruning limit
   double covThreshold;             ///< Coverage pruning prob threshold
   Uint diversity;                  ///< Min states to keep per coverage
   int diversityStackIncrement;     ///< stack size increment due to diversity states
   int levLimit;                    ///< levenshtien limit
   int distLimit;                   ///< Distortion limit
   int itgLimit;                    ///< Limit distance moved from top of SR stack
   bool distLimitExt;               ///< Use the extended definition of dist limit
   bool distLimitSimple;            ///< Use the simple definition of dist limit
   bool distPhraseSwap;             ///< Allow swapping contiguous phrases
   bool distLimitITG;               ///< Enable ITG constraint
   bool forceShiftReduce;           ///< ShiftReducer included regardless of limits/features
   bool shiftReduceOnlyITG;         ///< ShiftReducer can perform only ITG reductions
   vector<string> filterFeatures;   ///< Filter features (DMs used a hard constraints)
   bool bypassMarked;               ///< Look in PT even for marked trans
   double weightMarked;             ///< Constant discount for marked probs
   bool nosent;                     ///< Don't use LM sentence beg/end context.
   bool appendJointCounts;          ///< Append joint counts from different phrase tables
   string oov;                      ///< OOV handling method
   bool tolerateMarkupErrors;       ///< Whether to proceed despite markup err
   bool checkInputOnly;             ///< If true, only check the input for markup errors
   bool describeModelOnly;          ///< If true, only describe the model, don't translate or load models.
   bool trace;                      ///< Whether to output alignment info
   bool walign;                     ///< Whether to include word alignment in alignment info
   bool ffvals;                     ///< Whether to output feature fn values
   bool sfvals;                     ///< Whether to output sparse feature values
   bool sparseModelAllowNonLocalWts;///< Whether canoe is allowed to look for non-local sparse weights
   bool masse;                      ///< Whether to output total lattice weight
   Uint verbosity;                  ///< Verbosity level
   bool quietEmptyLines;            ///< if set, consider empty input lines as normal
   string latticeFilePrefix;        ///< Prefix for all lattice output files
   bool latticeOut;                 ///< Whether to output the lattices
   string latticeOutputOptions;     ///< Style of lattice output
   double latticeDensity;           ///< Density of overlay lattices
   double latticeMinEdge;           ///< Minimum edge score for overlay lattices
   bool latticeLogProb;             ///< Overlay prints log probs instead of probs
   bool latticeSourceDensity;       ///< Calculate overlay density based on length of source sentence
   string nbestFilePrefix;          ///< Prefix for all n-best output files
   Uint nbestSize;                  ///< Number of hypotheses in n-best lists
   bool nbestOut;                   ///< Whether to output n-best hypotheses
   Uint firstSentNum;               ///< Index of the first input sentence
   bool backwards;                  ///< Whether to translate backwards
   bool loadFirst;                  ///< Whether to load models before input
   string canoeDaemon;              ///< Sentence by sentence mode specifications
   string input;                    ///< Source sentences input file name
   string srctags;                  ///< Source sentences tags file name
   string refFile;                  ///< Reference file name
   bool bAppendOutput;              ///< Flag to output one single file instead of multiple files.
   bool bLoadBalancing;             ///< Running in load-balancing mode => parse source sentences ids
   bool bStackDecoding;             ///< Explicitly request the regular stack decoder
   bool bCubePruning;               ///< Run the cube-pruning decoder
   string cubeLMHeuristic;          ///< What LM heuristic to use in cube pruning
   string futLMHeuristic;           ///< What LM heuristic to use when calculating future scores
   bool useFtm;                     ///< Use FTMs even if no weights are given
   bool hierarchy;                  ///< canoe will output its nbest in a hierarchy.
   bool forcedDecoding;             ///< Indicates if decoding is forced to match the reference
   bool forcedDecodingNZ;           ///< Forced decoding without zeroing lm, length and ibm1 weights
   Uint maxlen;                     ///< Skip sentences longer than max len (0 means do all)
   string triangularArrayFilename;  ///< Where should we write each triangular array.

   string nssiFilename;             ///< Triggers outputing, for every input sentence, the newSrcSentInfo into a file in a json format.

   // how to run the software
   bool final_cleanup;              ///< Indicates if canoe should delete its bmg.
   int  bind_pid;                   ///< What pid to monitor.
   bool timing;                     ///< Show per-sentence timing information
   bool need_lock;                  ///< Require a shared lock on config file

   /**
    * Constructor, sets default parameter values.
    */
   CanoeConfig();

private:
   /**
    * Read parameters from a config stream.
    *
    * Input format is parameter name, enclosed in square brackets, followed by
    * a whitespace, then a value. For boolean parameters the value is omitted.
    * This will silently overwrite any existing values.
    * @param configin  the input stream containing the configuration.
    */
   void read(istream& configin);

public:
   /**
    * Read parameters from a config file.
    *
    * See read(istream& configin) for details.
    * @param configFile  file name
    */
   void read(const char* configFile);

   /**
    * Get a list of parameters in ArgReader format.
    * @return list of parameters (each followed by a colon unless boolean).
    */
   const vector<string>& getParamList() const { return param_list; }

   //@{
   /**
    * Access a particular feature's description object
    * @param name  The short name for the feature.  See weight_params_other, or
    * weight_names_other in config_io.cc, for the current list of allowed
    * feature names.
    */
   FeatureGroup* featureGroup(const char* group) { return features.get(group); }
   const FeatureGroup* featureGroup(const char* group) const { return features.get(group); }
   //@}

   /**
    * Set parameters from an ArgReader object.
    * Switches not corresponding to CanoeConfig parameters are ignored.
    * @param arg_reader Object containing command-line switches.
    */
   void setFromArgReader(ArgReader& arg_reader);

   /**
    * Check parameter values and die if not ok.  Must be called after setting
    * parameters because it applies various logical implications between
    * options.  One of several rules applied is setting default weights as
    * needed.
    */
   void check();

   /**
    * Get a read lock on the current config file
    */
   void lock();

   /**
    * Check that all files listed in the parameter values are readable.
    * Call this to provide early error messages, after all file names are
    * fixed.  Dies with an error message if any model file is not readable.
    */
   void check_all_files() const;

   /**
    * Move all phrase table features to the new [ttable] field, from old fields
    */
   void movePTsIntoTTable();

   /**
    * Count how many backward TMs there are, over all PT types.
    */
   Uint getTotalBackwardModelCount() const;

   /**
    * Count the total number of translation models in multi-prob tables.
    * Dies if any table has an odd number of probability columns.
    * @return the number of models in files listed under ttable-multi-prob.
    */
   Uint getTotalMultiProbModelCount() const;

   /**
    * Count the total number of adirectional models, over all PT types
    * @return the number of models in files listed under ttable-multi-prob.
    */
   Uint getTotalAdirectionalModelCount() const; //boxing

   /**
    * Count the total number of translation models in TPPT tables.
    * Dies if any TPPT has an odd number of probability columns.
    * @return the number of models in files listed under ttable-tppt.
    */
   Uint getTotalTPPTModelCount() const;

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
    * Get current sparse feature weights. The first n weights in the returned
    * vector will match the n weights returned by getFeatureWeights(), except
    * those on any SparseModel features, which are set to 0. Component weights
    * for each successive SparseModel are then appended to the vector. The ith
    * weight in the resulting vector will match the ith feature in -sfvals
    * output from canoe.
    * @param weights vector to receive the weights
    */
   void getSparseFeatureWeights(vector<double>& weights) const;

   /**
    * Set current sparse feature weights from a given vector, which should be
    * the same size as returned by getSparseFeatureWeights(). The first n
    * weights in the vector will be transferred to the current decoder model,
    * except for those on any SparseModel features, which are set to 1.
    * Component weights for each successive SparseModel are saved to that
    * SparseModel - this involves disk i/o; see SparseModel::writeWeights().
    */
   void setSparseFeatureWeights(const vector<double>& weights);

   /**
    * Get the list of primary weight names
    * For future use: intended to drive BMG::InitDecoderFeatures(),
    * setWeightsFromString(), and anything else that has to respect the
    * ordering of features in the model.
    */
   const vector<string>* getPrimaryFeatureWeights() const { return &weight_params_primary; }

   /**
    * Get the list of other weight names
    * For future use: intended to drive BMG::InitDecoderFeatures(),
    * setWeightsFromString(), and anything else that has to respect the
    * ordering of features in the model.
    */
   const vector<string>* getOtherFeatureWeights() const { return &weight_params_other; }

   /**
    * Write parameters to stream.
    *
    * The output is written in the format expected by read().
    * Boolean parameters whose value is false are not written.
    * @param configout output stream
    * @param what specifies what gets written: 0 = only parameters set in
    * config file, 1 = parameters set in config file or on command line,
    * 2 = all params.
    * @param pretty  Print pretty?
    */
   void write(ostream& configout, Uint what=0, bool pretty=false);
   /**
    * Write parameters to file.
    *
    * See write(ostream& configout, Uint what) for details.
    * @param cfgFile output file name
    * @param what specifies what gets written: 0 = only parameters set in
    * config file, 1 = parameters set in config file or on command line,
    * 2 = all params.
    * @param pretty if true, write string vector elements one per line
    */
   void write(const char *cfgFile, Uint what=0, bool pretty=false) {
      oSafeMagicStream cfg(cfgFile);
      write(cfg, what, pretty);
   }

   /**
    * Return the 'read' status associated with a parameter.
    * @param param Name of the parameter
    * @return modifiable reference to the 'read' status of param.  Assigning to
    *         the return value will modify the parameter's status.
    */
   bool& readStatus(const string& param);


   bool prime(bool full = false);


private:

   // No, this is not the most elegant way to handle generics...
   /// Generic structure for handling parameters of various types.
   struct ParamInfo {
      /// These flags will associate parameters to groups which need special treatment.
      /// A parameter can belong to more than one group.
      enum GROUPING {
         /// Default, parameters belong to no special groups
         no_group                   = 0,
         /// Apply to path/filename and corrects the path base on canoe.ini path (canoe.relative)
         relative_path_modification = 1 << 0,
         /// When check accessibility of file from configtool check
         check_file_name            = relative_path_modification << 1,
         /// Specific LM file name check because some LMs need a special treatment
         lm_check_file_name         = check_file_name << 1,
         /// Specific file name check for BiLM
         bilm_check_file_name       = lm_check_file_name << 1,
         /// Specific file name check for TTables of various types
         ttable_check_file_name     = bilm_check_file_name << 1,
         /// Specific file name check for LDMs and TPLDMs
         ldm_check_file_name        = ttable_check_file_name << 1,
         /// Check that the name is an accessible directory
         check_dir_name             = ldm_check_file_name << 1,
         /// Indicates the number of groupings
         num_groups                 = check_dir_name << 1
      };

      CanoeConfig* c;        ///< Pointer to parent CanoeConfig object
      vector<string> names;  ///< param name followed by 0 or more alternates
      string tconv;          ///< how to convert to/from string
      void* val;             ///< pointer to current value
      bool set_from_config;  ///< true if set from config file
      bool set_from_cmdline; ///< true if set from command line
      Uint  groups;          ///< parameter belong to which groups

      /**
       * Constructor.
       * @param name_strings  name for the parameter
       * @param tconv         type for the parameter
       * @param val           value of the parameter
       * @param g             group id
       */
      ParamInfo(const string& name_strings, const string& tconv, void* val, Uint g = no_group) :
         tconv(tconv),
         val(val),
         set_from_config(false),
         set_from_cmdline(false),
         groups(g) {
            split(name_strings, names);
         }

      /// Set value from string.
      /// @param s  new value
      void set(const string& s);
      /// Set this bool parameter's value
      /// @param value  new value
      /// @pre tconv == "bool"
      void set(bool value);

      /// Get string representation of current value
      /// @param pretty make the string pretty (caution: not necessarily
      ///    reversible using set())
      /// @return Returns a string representation of current value.
      string get(bool pretty = false);
   };

   static const Uint precision = 10; ///< significant digits for weights

   vector<ParamInfo> param_infos;    ///< main parameter list
   vector<string> weight_params_other; ///< names of params that correspond to regular weights
   vector<string> weight_params_primary; ///< names of special weights: LMs, TMs and their components
   vector<string> weight_params;     ///< names of all params that correspond to weights
   map<string,ParamInfo*> param_map; ///< map : name -> param info
   vector<string> param_list;        ///< list of parameters for ArgReader
};


}

#endif
