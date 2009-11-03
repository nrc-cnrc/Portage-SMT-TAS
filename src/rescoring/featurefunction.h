/**
 * @author Aaron Tikuisis / George Foster
 * @file featurefunction.h  K-Best Rescoring Module - Feature functions and
 * sets of 'em.
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/

#ifndef FEATUREFUNCTION_H
#define FEATUREFUNCTION_H

#include "portage_defs.h"
#include "basic_data_structure.h"
#include <string>
#include <vector>

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

// Forward declaraction - include vocab_filter.h if you actually need it.
class VocabFilter;

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

   /// Constructor for use by subclasses.
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
 * Length-ratio feature function
 */
class RatioFF: public FeatureFunction
{
protected:
  double srclen;
public:
   RatioFF() : FeatureFunction("") {}
   virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual FeatureFunction::FF_COMPLEXITY cost() const { return LOW; }
   virtual void source(Uint s, const Nbest * const nbest) {
      this->srclen = double((*src_sents)[s].getTokens().size()-1);
      this->nbest = nbest;
   }
   virtual double value(Uint k) {
     return (*nbest)[k].getTokens().size()/srclen; }
};

}

#endif // FEATUREFUNCTION_H
