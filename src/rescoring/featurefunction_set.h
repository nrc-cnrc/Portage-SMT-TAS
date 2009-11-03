/**
 * @author Aaron Tikuisis / George Foster / Samuel Larkin / Eric Joanis
 * @file featurefunction.h  K-Best Rescoring Module - Sets of Feature functions
 *
 * $Id$
 *
 * This class was moved out of featurefunction.h to remove spurious
 * dependencies that slowed down compiling.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005 - 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005 - 2008, Her Majesty in Right of Canada
*/

#ifndef FEATUREFUNCTION_SET_H
#define FEATUREFUNCTION_SET_H

#include "boostDef.h"
#include "featurefunction.h"
#include "fileReader.h"
#include "randomDistribution.h"
#include <boost/shared_ptr.hpp>

namespace Portage {

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
       * @param fd      full description.
       * @param fn      feature's name.
       * @param f       feature function instance.
       * @param weight  feature function's weight
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
   void computeFFMatrix(uMatrix& H, Uint s, Nbest &nbest);

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

} // Portage

#endif // FEATUREFUNCTION_SET_H
