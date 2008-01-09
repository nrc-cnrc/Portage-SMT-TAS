/**
 * @author Aaron Tikuisis / George Foster
 * @file featurefunction.h  K-Best Rescoring Module - Feature functions and
 * sets of 'em.
 *
 * $Id$
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/

#ifndef FEATUREFUNCTION_H
#define FEATUREFUNCTION_H

#include <portage_defs.h>
#include <basic_data_structure.h>
#include <boostDef.h>
#include <errors.h>
#include <boostDef.h>
#include <multiColumnFileFF.h>
#include <vocab_filter.h>
#include <fileReader.h>
#include <randomDistribution.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <boost/shared_ptr.hpp>

// Guide to the perplexed: definitions of the following classes come from
// "basic_data_structure.h", which is included from the eval module:

// class Sentence;      // string of chars + vector<string> of toks
// class Translation;   // Sentence + Alignment
// class Nbest;         // vector<Translation>
// class PhraseRange;   // first + last
// class AlignedPair;   // source first + source last + target first + target last
// class Alignment;     // vector<AlignedPair>

namespace Portage
{

/// Definition of feature function requirements.
enum FFRequires {
   FF_NEEDS_NOTHING       =0,  ///< Feature function needs nothing.
   FF_NEEDS_SRC_TEXT      =1,  ///< Feature function needs source text.
   FF_NEEDS_TGT_TEXT      =2,  ///< Feature function needs target text.
   FF_NEEDS_SRC_TOKENS    =4,  ///< Feature function needs source text tokenized.
   FF_NEEDS_TGT_TOKENS    =8,  ///< Feature function needs target text tokenized.
   FF_NEEDS_ALIGNMENT     =16, ///< Feature function needs alignments.
   FF_NEEDS_TGT_VOCAB     =32  ///< Feature function needs the target vocab from the NBest list.
};

/**
 * Abstract Feature Function interface.
 * The constructor of a featurefunction must take a string so we can pass it
 * general argument.  It should also do the least amount of work with that
 * argument and leave parsing to a later function.  This allows us to query the
 * function for complexity and its requirements without having to load the
 * models (used by the feature_function_tool).
 * To allow argument integrity checking, we separate parsing and model loading
 * into two different functions (parseAndCheckArgs loadModelsImpl).  This allow
 * feature_function_tool to verify the syntax of a rescoring-model without
 * loading any models.
 * To start generating values, each feature function's init will be called once
 * with a list of all source sentences.  For each source we then call source
 * with the source sentence's own nbest.  Finally, we can extract feature
 * values by calling the value(s) function.
 */
class FeatureFunction {

protected:

   const string      argument;  ///< feature function's argument
   const Sentences*  src_sents; ///< source sentences
   const Nbest*      nbest;     ///< nbest list for src_sents[s]
   Uint   s;                    ///< index of current src sent
   Uint   K;                    ///< nbest list size

   VocabFilter* tgt_vocab;      ///< target vocab over all nbests

private:
   /// Deactivated default constructor
   FeatureFunction();

protected:   
   /**
    * Load the necessary models in memory. 
    * This was separated from the constructor to allow query requirements of
    * each FF without having to completely load them in memory. 
    * @return Returns true if everything was loaded fine.
    */
   virtual bool loadModelsImpl() { return true; }

public:

   typedef enum {
      LOW,
      MEDIUM,
      HIGH,
      MAX
   } FF_COMPLEXITY;

   /// Empty constructor.
   FeatureFunction(const string& argument)
   : argument(argument)
   , src_sents(NULL)
   , nbest(NULL)
   , s(0)
   , K(0)
   , tgt_vocab(NULL)
   {}
   /// Destructor.
   virtual ~FeatureFunction() {};

   /////////////////////////////////////////////////////////////////
   // THE FOLLOWING ARE FEATURE FUNCTION STATE QUERIES
   /////////////////////////////////////////////////////////////////

   /**
    * Makes sure the feature is done doing what it was supposed to do.  Mainly
    * used to know if the file feature was done reading the input file.
    * @return Returns true if feature was done completely.
    */
   virtual bool done() { return true; }

   /**
    * Get the requirements for this feature function.  Indicates what the
    * feature function needs in order for it to be calculated.
    * @return Returns the requirements for this feature function, as zero or
    *         more FFRequires values bitwise-or'd together (using | ).
    */
   virtual Uint requires() = 0;

   /**
    * Indicates "how hard" this feature is to calculate.  
    * This will guide gen-feature-parallel.sh in the number of jobs required to
    * speed-up generating its values.
    * @return Returns a hint of complexity for this feature
    */
   virtual FF_COMPLEXITY cost() const {
       return MEDIUM;
   }

   /**
    * Get a string representing the complexity cost
    * @return Returns the complexity cost
    */
   const char* complexitySting() const {
      switch(cost())
      {
         case FeatureFunction::LOW:    return "LOW";
         case FeatureFunction::MEDIUM: return "MEDIUM";
         case FeatureFunction::HIGH:   return "HIGH";
         default: return "INVALID COMPLEXITY";
      }
   }

   /////////////////////////////////////////////////////////////////
   // LOADING THE REQUIRED MODELS
   /////////////////////////////////////////////////////////////////

   /**
    * This makes sure that all the arguments value are there.
    * This doesn't imply checking for file existence but rather that the user
    * have for a valid feature function input in its rescoring-model.
    * @return Returns true if all arguments are present
    */
   virtual bool parseAndCheckArgs() { return true; } 

   /**
    * Load the necessary models in memory. 
    * This was separated from the constructor to allow query requirements of
    * each FF without having to completely load them in memory. 
    * @return Returns true if everything was loaded fine.
    */
   bool loadModels() {
      if (!parseAndCheckArgs())
         return false;
      return loadModelsImpl();
   }

   /////////////////////////////////////////////////////////////////
   // FEATURE FUNCTION VALUE COMPUTATION
   /////////////////////////////////////////////////////////////////

   /**
    * This function is called for all features, provided at least one feature
    * says that it needs the tgt vocab (FF_NEEDS_TGT_VOCAB). It is called for
    * every nbest list, during tgt_vocab compilation, in order to give features
    * the chance to process the ENTIRE set of nbest lists. Yup, this function
    * is insane. See CacheLM (the only feature that uses it at this point).
    * @param tgt_vocab the current partial tgt_vocab, after processing nb
    * @param src_index index of current source sentence 
    * @param nb nbest list for src_index
    */
   virtual void preprocess(VocabFilter* tgt_vocab, Uint src_index, const Nbest& nb) {}

   /**
    * Initial info.
    * @param src_sents  source sentences.
    */
   virtual void init(const Sentences * const src_sents) {
      assert(src_sents);
      this->src_sents = src_sents;
   }

   /**
    * Attaches the tgt vocabs for filtering TMs and LMs
    * @param _tgt_vocab  global vocabulary
    */
   virtual void addTgtVocab(VocabFilter* _tgt_vocab) {
      tgt_vocab = _tgt_vocab;
   }

   /**
    * Start processing a new source sentence.
    * @param s      index of sent within src_sents arg to init().
    * @param nbest  list of target sentences for this source sentence.
    */
   virtual void source(Uint s, const Nbest * const nbest) {
      assert(nbest != NULL);
      this->s     = s;
      this->nbest = nbest;
      this->K     = nbest->size();
   }

   /**
    * Compute feature value for a particular source/target pair.
    * @param k index of target hypothesis within nbest arg to source().
    * @return the feature value.
    */
   virtual double value(Uint k) = 0;

   /**
    * Compute feature values for each target word in the current hypothesis.
    * The default, implemented in the base class, is to return the global value
    * averaged over number of tokens. 
    * @param k index of tgt_sent within nbest arg to source().
    * @param vals vector to fill in with one value per token - will be initialized
    * empty
    */
   virtual void values(Uint k, vector<double>& vals) {
      const double v = value(k);
      const Uint ntoks = (*nbest)[k].getTokens().size();
      vals.assign(ntoks, v / ntoks);
   }

}; // class FeatureFunction

void writeFFMatrix(ostream &out, const vector<uMatrix>& vH);
void readFFMatrix(istream &in, vector<uMatrix>& vH);


/// Definition of a Feature function pointer for the factory.
typedef boost::shared_ptr<FeatureFunction> ptr_FF;

/**
 * A weighted set of feature functions
 */
class FeatureFunctionSet
{
   bool training;               // read() will parse random distn if true
   Uint seed;                   // seed increment for all random generators

public:

   /// Feature function information for FeatureFunctionSet.
   struct ff_info
   {
      string       name;             ///< unique name for the feature
      string       fullDescription;  ///< name + arguments
      double       weight;           ///< weight for this feature
      ptr_FF       function;         ///< Instance of the class for the feature.
      ptr_rnd_gen  rnd_gen;          ///< A random number generator for this feature
    
      /// Empty constructor.
      ff_info() : name("Un-initialised"), fullDescription("NoDesc"), weight(0.0f) {}
      /// Commented feature function Constructor.
      /// @param fd  full description of commented feature function
      ff_info(const string& fd) : name("commented feature funtion"), fullDescription(fd), weight(0.0f) {}

      /**
       * Constructor.
       * @param fd  full description.
       * @param fn  feature's name.
       * @param f   feature function instance.
       * @param weight feature function's weight
       * @param rg      the random number generator for ranges
       */
      explicit ff_info(const string& fd, 
            const string& fn, 
            ptr_FF f, 
            double weight = 0.0f,
            ptr_rnd_gen rg = ptr_rnd_gen(new normal(0.0f, 1.0f)))
         : name(fn)
         , fullDescription(fd)
         , weight(weight)
         , function(f)
         , rnd_gen(rg)
      {}
   };

   VocabFilter*  tgt_vocab;           ///<  Global Vocabulary

   /// Information for each feature function in this set.
   /// We also internally keep the commented feature function to reinsert them
   /// in the outputed model.
   struct FF_INFO : public vector<ff_info>
   {
      /// Keeps track of the commented feature functions in the model
      vector<ff_info>  commented_ff;
      /// Keeps track of the order of the feacture functions read from the
      /// model and also if the feature function is active or commented.
      vector< pair<vector<ff_info>*, Uint> > all_ff;

      /// Insert an active feature function
      /// @param ff  active feature function to insert
      void push_back(const ff_info& ff) {
         all_ff.push_back(make_pair(this, size()));
         vector<ff_info>::push_back(ff_info(ff));
      }
      /// Insert a commented feature function description string
      /// @param ff  commented feature function full description
      void push_back(const string& ff) {
         all_ff.push_back(make_pair(&commented_ff, commented_ff.size()));
         commented_ff.push_back(ff);
      }
      /// Writes the model to a output stream
      /// @param ostr  output stream for the model
      void write(ostream& ostr) const {
         typedef vector< pair<vector<ff_info>*, Uint> >::const_iterator IT;
         for (IT it(all_ff.begin()); it!=all_ff.end(); ++it) {
            assert(it->first);
            const ff_info& info = it->first->at(it->second);
            ostr << info.fullDescription << " " << info.weight << endl;
         }
      }
   } ff_infos;

   /// Constructor. You must call read() before using.
   /// @param training Specifies if this is set should parse the random distn
   /// @param seed     Seed's value.
   FeatureFunctionSet(bool training = false, Uint seed = 0);

   /// Destructor.
   ~FeatureFunctionSet();

   /**
    * Needs to be called after read to create the global and the per sentence
    * vocabulary if needed by any feature function.
    * @param source_sentences  sources sentences for vocabulary
    * @param nbReader  yes a NbestReader because it makes it easier to handle dynamic size list
    */
   void createTgtVocab(const Sentences& source_sentences, NbestReader nbReader);

   /** 
    * Get the number of features.
    * @return Return the number of features.
    */
   Uint M() const { return ff_infos.size(); } 
      
   /// Get the random weights vector.
   /// @param v  returned weights vector.
   void getRandomWeights(uVector& v) const;

   /// Get the weights vector.
   /// @param v  returned weights vector.
   void getWeights(uVector& v) const;

   /// Set the weights vector.
   /// @param v  weights vector.
   void setWeights(const uVector& v);

   /**
    * Read a set of feature functions from a file into the ff_infos member.
    * @param filename
    * @param verbose
    * @param fileff_prefix prefix to use for FileFF features (see create()'s
    *        fileff_prefix arg)
    * @param isDynamic indicates that all File features are dynamic (i.e. FileDFF's)
    * @param useNullDeleter indicates to initialize the shared_ptr with a
    *        NullDeleter, which will cause the FeatureFunction not to be
    *        deleted when the ptr_FF is deleted.  Will cause a memory leak.
    *        Use this only if you know you would delete the ptr_FF right at
    *        program exit time, when the OS is about to clean-up for you
    *        anyway.  We do it by default since that's true of every program at
    *        this point.
    * @param loadModels  should we load the models when reading the rescoring-model
    * @return number of features read
    */
   Uint read(const string& filename,
             bool verbose = false, 
             const char* fileff_prefix = NULL,
             bool isDynamic = false,
	     bool useNullDeleter = true,
             bool loadModels = true);

   /**
    * Write current set of feature functions to a file.
    * @param filename  file name
    */
   void write(const string& filename);

   /**
    * Prepare the FeatureFunction array for computing the FF Matrix.
    * @param src_sents     All source sentences.
    */
   void initFFMatrix(const Sentences& src_sents);

   /**
    * Compute one row in the FF Matrix, corresponding to one source sentence and
    * its nbest list.  It also prunes the empty hypotheses from the nbest and
    * the matrices.
    * @param H             The FF Matrix, i.e., the matrix to be filled with the
    *                      precomputed feature function results
    * @param s             The index number of the source sentence under
    *                      consideration
    * @param nbest         The nbest list for the source sentence (pruned from
    * empty hypotheses).
    */
   void computeFFMatrix(uMatrix& H, const Uint s, Nbest &nbest);

   /**
    * Makes sure all feature are done.  Mainly the reading file feature.
    * @return Returns false if at least one feature was not done.
    */
   bool complete();

   /**
    * Get the requirements for all the feature functions.  Indicates what the
    * feature functions need in order for it to be calculated.
    * @return Returns the requirements for all feature functions.
    */
   Uint requires();

   /**
    * Verifies the syntax for each feature functions
    * @return Returns true if all feature are provided with all their arguments.
    */
   bool check();

   /**
    * Create a new feature function from its name and argument
    * (Feature function factory).
    * @param name of feature function
    * @param arg to feature function
    * @param fileff_prefix if not null, append to "<arg>" before creating
    *        FileFF-type features (FileFF and FileDFF).
    * @param isDynamic indicates the File feature is dynamic (i.e. a FileDFF)
    * @param useNullDeleter indicates to initialize the shared_ptr with a
    *        NullDeleter, which will cause the FeatureFunction not to be
    *        deleted when the ptr_FF is deleted.  Will cause a memory leak.
    *        Use this only if you know you would delete the ptr_FF right at
    *        program exit time, when the OS is about to clean-up for you
    *        anyway.  We do it by default since that's true of every program at
    *        this point.
    * @param loadModels indicates to make the ff load it's model.  Not loading
    *        would indicate that the ff will only be queried.
    * @return NULL if feature creation failed.
    */
   static ptr_FF create(const string& name,
                        const string& arg, 
                        const char* fileff_prefix = NULL,
                        bool isDynamic = false,
                        bool useNullDeleter = true,
                        bool loadModels = true);

   /**
    * Return a help string describing all available features.
    * @return Returns a help string describing all available features.
    */
   static const string& help();

}; // class FeatureFunctionSet


//------------------------------------------------------------------------------
/**
 * Character-length feature function
 */
class LengthFF: public FeatureFunction
{
public:
   LengthFF() : FeatureFunction("") {}
   virtual Uint requires() { return FF_NEEDS_TGT_TEXT; }
   virtual FeatureFunction::FF_COMPLEXITY cost() const { return LOW; }
   virtual double value(Uint k) { return nbest->at(k).size(); }
};


//------------------------------------------------------------------------------
/**
 * Read-from-file feature function.
 */
class FileFF : public FeatureFunction
{
protected:
   string           m_filename;  ///< file name
   Uint             m_column;    ///< column index for this feature function.
   multiColumnUnit  m_info;  // NOT TO BE DELETED

protected:   
   virtual bool loadModelsImpl();
public:

   /**
    * Construct.
    * @param filespec spec of form filename[,i] - if "<i>" specified, use the ith
    * column in file "<filename>", otherwise there must only be one value per line
    */
   FileFF(const string& filespec);

   virtual Uint requires() { return FF_NEEDS_NOTHING; }
   virtual bool parseAndCheckArgs();

   virtual double value(Uint k) {
      return m_info->get(m_column, s*K + k);
   }

   virtual bool done() {
      if (!m_info->eof()) {
         cerr << "Not done: " << m_filename << ":" << m_column << endl;
         return false;
      }
      return true;
   }
};


//------------------------------------------------------------------------------
/**
 * Read-from-dynamic-file feature feature.
 */
class FileDFF : public FeatureFunction
{
   /// Internal definition of a matrix for this feature function.
   typedef vector< vector<double> > matrice;
   string      m_filename;  ///< file name
   Uint        m_column;    ///< column index for this feature function.
   /**
    * m_vals is not an even matrix since we are in dynamic mode this means
    * that we don't have nbest of the same size.
    */
   matrice     m_vals;

protected:
   virtual bool loadModelsImpl();
public:

   /**
    * Construct.
    * @param filespec spec of form filename[,i] - if "<i>" specified, use the ith
    * column in file "<filename>", otherwise there must only be one value per line
    */
   FileDFF(const string& filespec);

   virtual Uint requires() { return FF_NEEDS_NOTHING; }
   virtual bool parseAndCheckArgs();
   virtual void source(Uint s, const Nbest * const nbest);
   virtual double value(Uint k);
};

}

#endif // FEATUREFUNCTION_H
