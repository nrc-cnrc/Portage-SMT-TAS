/**
 * @author Aaron Tikuisis / George Foster
 * @file featurefunction.h  K-Best Rescoring Module - Feature functions and
 * sets of 'em.
 *
 * $Id$
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
   FF_NEEDS_ALIGNMENT     =16  ///< Feature function needs alignments.
};

/**
 * Abstract Feature Function interface.
 */
class FeatureFunction {

public:

   /// Empty constructor.
   FeatureFunction()
   : src_sents(NULL)
   , nbest(NULL)
   , s(0)
   , K(0)
   {}
   /// Destructor.
   virtual ~FeatureFunction() {};

   /**
    * Get the requirements for this feature function.  Indicates what the
    * feature function needs in order for it to be calculated.
    * @return Returns the requirements for this feature function.
    */
   virtual Uint requires() = 0;

   /**
    * Initial info.
    * @param src_sents  source sentences.
    * @param K          number of target sentences per source sent.
    */
   virtual void init(const Sentences * const src_sents, Uint K) {
      this->src_sents = src_sents;
      this->K = K;
   }

   /**
    * Start processing a new source sentence.
    * @param s      index of sent within src_sents arg to init().
    * @param nbest  list of target sentences for this source sentence.
    */
   virtual void source(Uint s, const Nbest * const nbest) {
      this->s     = s;
      this->nbest = nbest;
   }

   /**
    * Compute feature value for a particular source/target pair.
    * @param k index of tgt_sent within nbest arg to source().
    * @return Returns the compute feature value for a particular source/target
    * pair.
    */
   virtual double value(Uint k) = 0;

   /**
    * Makes sure the feature is done doing what it was supposed to do.  Mainly
    * used to know if the file feature was done reading the input file.
    * @return Returns true if feature was done completely.
    */
   virtual bool done() { return true; }

protected:

   /// Source sentences
   const Sentences*  src_sents;
   /// NBest list for src_sents[s]
   const Nbest*      nbest;
   /// Index of the source sentence we are currently working on.
   Uint   s;
   /// NBest list size.
   Uint   K;
};

void writeFFMatrix(ostream &out, const vector<uMatrix>& vH);
void readFFMatrix(istream &in, vector<uMatrix>& vH);


/// Definition of a Feature function pointer for the factory.
typedef boost::shared_ptr<FeatureFunction> ptr_FF;

/**
 * A weighted set of feature functions
 */
class FeatureFunctionSet
{
private:
   /// Feature function information for FeatureFunctionSet.
   struct ff_info
   {
      string       name;             ///< unique name for the feature
      string       fullDescription;  ///< name + arguments
      double       weight;           ///< weight for this feature
      ptr_FF       function;         ///< Instance of the class for the feature.
    
      /// Empty constructor.
      ff_info() : name("Un-initialised"), fullDescription("NoDesc"), weight(0.0f) {}
      /**
       * Constructor.
       * @param fd  full description.
       * @param fn  feature's name.
       * @param w   weight for the describe feature.
       * @param f   feature function instance.
       */
      explicit ff_info(const string& fd, const string& fn, const double w, ptr_FF f)
         : name(fn)
         , fullDescription(fd)
         , weight(w)
         , function(f)
      {}
   };

public:

   /// Information for each feature function in this set.
   vector<ff_info> ff_infos;

   /// Constructor.
   /// You must call read(const string& filename, bool verbose, const char* fileff_prefix, bool isDynamic, bool useNullDeleter)
   FeatureFunctionSet() {}
   /// Destructor.
   ~FeatureFunctionSet() { ff_infos.clear(); }

   /** 
    * Get the number of features.
    * @return Return the number of features.
    */
   inline Uint M() const { return ff_infos.size(); } 
      
   /// Get the weights vector.
   /// @param v  returned weights vector.
   void getWeights(uVector& v) const;
   /// Set the weights vector.
   /// @param v  weights vector.
   void setWeights(const uVector& v);

   /**
    * Read a set of feature functions from a file.
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
    * @return number of features read
    */
   Uint read(const string& filename, bool verbose = false, 
             const char* fileff_prefix = NULL,
             bool isDynamic = false,
             bool useNullDeleter = true);

   /**
    * Write current set of feature functions to a file.
    * @param filename  file name
    */
   void write(const string& filename);

   /**
    * Prepare the FeatureFunction array for computing the FF Matrix.
    * @param src_sents     All source sentences.
    * @param K             The number of target sentences per source sentence
    */
   void initFFMatrix(const Sentences& src_sents, const int K);

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
    * @return NULL if feature creation failed.
    */
   static ptr_FF create(const string& name, const string& arg, 
                        const char* fileff_prefix = NULL,
                        bool isDynamic = false,
                        bool useNullDeleter = true);

   /**
    * Return a help string describing all available features.
    * @return Returns a help string describing all available features.
    */
   static const string& help();
};


//------------------------------------------------------------------------------
/**
 * Character-length feature function
 */
class LengthFF: public FeatureFunction
{
public:
   virtual inline Uint requires() { return FF_NEEDS_TGT_TEXT; }
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

public:

   /**
    * Construct.
    * @param filespec spec of form filename[,i] - if "<i>" specified, use the ith
    * column in file "<filename>", otherwise there must only be one value per line
    */
   FileFF(const string& filespec);

   virtual inline Uint requires() { return FF_NEEDS_NOTHING; }

   virtual void init(const Sentences * const src_sents, Uint K) {
      FeatureFunction::init(src_sents, K);
   }

   virtual double value(Uint k) {
      return m_info->get(m_column, s*K + k);
   }

   virtual bool done() {
      return m_info->eof();
   }
};


//------------------------------------------------------------------------------
/**
 * Read-from-file feature function which contains arguments that are containing a path.
 * Historical facts: this feature function was added because of rat.sh that
 * would have to create a separate rescoring-model to allows feature function
 * file name with ff that would contain filepath in their arguments.
 */
class VFileFF : public  FileFF 
{
public:
   /**
    * Construct.
    * @param filespec spec of form filename.arg[,i] - if "<i>" specified, use the ith
    * column in file "<filename>", otherwise there must only be one value per line.
    * Where arg can contain a file path that must be converted to a unix file
    * name by replacing slashes for underscores.
    */
   VFileFF(const string& filespec);

   /**
    * Replaces the slashes found after the left most occurrence of "ff." by underscores.
    * @param arg  feature function file name containing a path in its argument.
    * @return Returns the transformed function feature file name.
    */
   static string convert2file(const string& arg);
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
   /**
    * m_vals is not an even matrix since we are in dynamic mode this means
    * that we don't have nbest of the same size.
    */
   matrice     m_vals;

public:

   /**
    * Construct.
    * @param filespec spec of form filename[,i] - if "<i>" specified, use the ith
    * column in file "<filename>", otherwise there must only be one value per line
    */
   FileDFF(const string& filespec);

   virtual inline Uint requires() { return FF_NEEDS_NOTHING; }
   virtual void init(const Sentences * const src_sents, Uint K) {}
   virtual void source(Uint s, const Nbest * const nbest);
   virtual double value(Uint k);
};

}

#endif // FEATUREFUNCTION_H
