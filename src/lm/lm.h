/**
 * @author Bruno Laferriere / Eric Joanis
 * @file lm.h  Abstract class for a language model.
 * $Id$
 *
 * Superclass LM to do abstracts / concrete implementation of language models
 *
 * Note: the convention throughout this module is to use "probability"
 * in place of "log probability". For example, wordProb() actually returns a
 * logprob, the documentation on backoffs in LMText::wordProb() talks about
 * p(w|...) when it means log p(w|...), etc. 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __PLM_H__
#define __PLM_H__

#include "portage_defs.h"
#include "vocab_filter.h"
#include <vector>
#include <boost/shared_ptr.hpp>

using namespace std;

namespace Portage
{


/// Abstract class for a language model
class PLM
{
public:
   /// Keeps track of how many times each N is hit during translation.
   struct Hits {
      private:
         vector<Uint> values;
         Uint latest_hit;
      public:
      /// Default constructor
      Hits() : latest_hit(0) {}
      /// Initializes with the lm order.
      /// @param gram_order  lm order aka max Ngram size.
      void init(Uint gram_order);
      /// Cumulates the hits from different lms.
      /// @param other  ride-hand side operand
      /// @return Returns a new hits containing the sum for each N.
      Hits& operator+=(const Hits& other);
      /// Displays the hits per ngrams.
      /// @param out  where to display [cerr]
      void display(ostream& out = cerr) const;
      /// Clears the hits values
      void clear() { values.clear(); }
      /// Cumulates hits for a particular N.
      /// @param N  length of observed sequence.
      void hit(Uint N) {
         ++values[N];
         latest_hit = N;
      }
      /// Get the value passed to hit() the most recent time it was called.
      Uint getLatestHit() {
         return latest_hit;
      }
   };

protected:
   /// Keeps track of how many times each N is hit over all queries
   Hits hits;

   /// Vocabulary for this model, possibly shared with other models
   VocabFilter* vocab;

   /**
    * Whether the LM is of type OOVHandling::FullOpenVoc.  When false,
    * oov_unigram_prob can be used as the unigram probability of any unknown
    * word.  When true, every unknown word in each wordProb() query must be
    * mapped to \<unk\> before looking up the information in the language
    * model.  (This is handled by those two methods in each LM subclass.)
    */
   bool complex_open_voc_lm;

   /**
    * Lm order (e.g., 3 or 4 gram).
    *
    * Don't query this variable directly, ask through the getOrder() method
    * Sub classes may set this variable in their constructor if the value is
    * already readily available at construction time.
    */
   Uint gram_order;

   /**
    * value for unigram probability of out-of-vocabulary words, for
    * ClosedVoc, SimpleOpenVoc and SimpleAutoVoc LMs.  Initialize from the
    * oov_unigram_prob parameter to Create() or by looking up p(\<unk\>) in
    * the LM, depending on the oov_handling parameter to Create()
    */
   float oov_unigram_prob;

   /// Human-readable description of this LM
   string description;

   /**
    * Ask sub class what is the gram order of the LM.  Must be overriden by
    * subclasses.  Must return the correct answer as soon as the contructor has
    * been executed.
    *
    * @return order of the language model
    */
   virtual Uint getGramOrder() = 0;

   /**
    * Get the word represented by index in this language model.
    * Typically the same as going through the global vocab, but can be
    * overriden by subclasses where this is not the case.
    * @param index  index to look up.
    * @return  the word index maps to, or "" if it is not in the vocabulary
    */
   virtual const char* word(Uint index) const;

public:
   static const char* lm_order_separator;  /// #
   /**
    * Constants used to determine how this LM will handle OOVs.
    * Defensive programming: Use a class instead of an enum so that primitive
    * types are not castable to this type, nor vice-versa.
    */
   struct OOVHandling {
      /// Return true iff this and that are the same type
      bool operator==(const OOVHandling& that) const
      { return type == that.type; }
      /// Display the value of this variable in printable form
      string toString() const;
    private:
      /// The actual enum we're wrapping, documented in the static consts in
      /// PLM.
      const enum Type {
         ClosedVoc, SimpleOpenVoc, SimpleAutoVoc, FullOpenVoc
      } type;
      /// Private constructor - only to be used by PLM to initialize the
      /// static consts.
      OOVHandling(Type type) : type(type) {}
      friend class PLM;
   };
   /**
    * Simplest OOV handling: Assume LM is closed vocabulary, and assign OOVs
    * the unigram probability passed to Create() through the oov_unigram_prob
    * parameter.
    */
   static const OOVHandling ClosedVoc;
   /**
    * Better OOV handling: Assume the LM is open vocabulary, i.e., it assigns a
    * unigram probability to \<unk\>, typically estimated by applying some
    * discounting method during LM training.  This is appropriate for LMs
    * generated using SRILM's ngram-count program with -unk, or CMU Toolkit's
    * idngram2lm program with -vocab_type 2.
    */
   static const OOVHandling SimpleOpenVoc;
   /**
    * Automatically detect a SimpleOpenVoc by looking for p(\<unk\>).  If
    * found in the LM, same as SimpleOpenVoc.  If not, same as ClosedVoc.
    */
   static const OOVHandling SimpleAutoVoc;
   /**
    * Full OOV handling: Assume the LM is open vocabulary and might provide
    * probabilities for \<unk\> in various contexts, and can include \<unk\>
    * in the context of other words.  As far as I (Eric Joanis) know, SRILM
    * never generates such LMs, but I think CMU Toolkit's idngram2lm will do
    * so with -vocab_type 1.  This method makes queries to the language model
    * somewhat more expensive, so it should only be used if the LM really is
    * of this type.
    */
   static const OOVHandling FullOpenVoc;

protected:
   /**
    * Contructor - only called by children classes' constructors
    * @param vocab              shared vocab object for all models
    * @param oov_handling       type of vocabulary
    * @param oov_unigram_prob   the unigram prob of OOVs (if oov_handling ==
    *                           ClosedVoc)
    */
   PLM(VocabFilter* vocab, OOVHandling oov_handling, float oov_unigram_prob);

   /**
    * Default contructor for subclasses that might need it.
    */
   PLM();

   /**
    * Class used to answer queries about subclasses without instanciating them,
    * and also to actually instantiate the objects.  Called Creator in line
    * with Factory Method Design Pattern, despite the fact that it is more than
    * just a Factory.
    *
    * Each subclass of LM must define this embedded class (must be public):
    *   struct Creator : public PLM::Creator
    * and implement or override the virtual methods as described below.
    */
   struct Creator {
      /// The physical file name of the LM, without any #N marker
      const string lm_physical_filename;
      /// If the filename of the LM had a #N marker, N; 0 otherwise
      const Uint naming_limit_order;

      /**
       * This constructor must be called by subclass constructors
       */
      Creator(const string& lm_physical_filename, Uint naming_limit_order);

      /**
       * Checks if a language model's file(s) exist(s)
       * The base class implemention simply checks that lm_physical_filename
       * exists, and should be overridden in classes where that is not a
       * sufficient check.
       * @param  lm_filename  name of the language model file
       * @return Returns true if the file exists (and associated files, if any)
       */
      virtual bool checkFileExists();

      /**
       * Calculate the total size of memory mapped files in lm_filename, if any.
       * The base class implementation returns 0, and should be overridden in
       * classes that use memory mapped IO or embed other PLMs.
       * @param lm_filename  lm whose size is to be determined
       * @return total size of memory mapped files associated with lm_filename;
       *   0 in case of problems or if the model does not use memory mapped IO.
       */
      virtual Uint64 totalMemmapSize();

      /**
       * Create an LM of the enclosing type.
       * See PLM::Create() for a description of the parameters
       * @return a new LM, which must be deleted externally when no longer
       *         needed.
       */
      virtual PLM* Create(VocabFilter* vocab,
                          OOVHandling oov_handling,
                          float oov_unigram_prob,
                          bool limit_vocab,
                          Uint limit_order,
                          ostream *const os_filtered,
                          bool quiet) = 0;
   }; // Creator

private:
   /**
    * Get the appropriate Creator for lm_filename, and parse the filename to
    * extract any marker requesting to limit the order.
    * @param lm_filename          LM filename to analyse
    * @param[out] lm_physical_filename  Set to filename without any #N marker
    * @param[out] lm_limit_order  Set to N if lm_filename has a #N marker, 0
    *                             otherwise.
    * @return a Creator of the appropriate subtype for lm_filename.
    */
   static shared_ptr<Creator> getCreator(const string& lm_filename);

public:
   /**
    * Create a PLM.  The type will be determined by the file name.
    * @param lm_filename   The file where the LM is stored
    * @param vocab         shared vocab object for all models
    * @param oov_handling  whether the LM is open or closed voc.  See enum
    *                      OOVHandling's documentation for details.
    * @param oov_unigram_prob  The unigram log prob of OOVs (only used if
    *                      oov_handling==ClosedVoc) [typical value: -INFINITY]
    * @param limit_vocab   whether to restrict the LM to words already in vocab
    * @param limit_order   if non-zero, will cause the LM to be treated as
    *                      order limit_order, even if it is actually of a
    *                      higher order.
    *                      If lm_filename ends in #N, that will also be
    *                      treated as if limit_order=N was specified.
    *                      [typical value: 0]
    * @param os_filtered   Opened stream to output the filtered LM.
    *                      [typical value: NULL]
    * @param quiet         Suppresses verbose.
    */
   static PLM* Create(const string& lm_filename,
                      VocabFilter* vocab,
                      OOVHandling oov_handling,
                      float oov_unigram_prob,
                      bool limit_vocab,
                      Uint limit_order,
                      ostream *const os_filtered,
                      bool quiet = false);

   /**
    * Calculate the total size of memory mapped files in lm_filename, if any.
    * @param lm_filename  lm whose size is to be determined
    * @return total size of memory mapped files associated with lm_filename; 0
    *         in case of problems or if the model does not use memory mapped IO.
    */
   static Uint64 totalMemmapSize(const string& lm_filename);

   /**
    * The main LM method: log probability of a word in context.
    *
    * Calculate log(p(word|context)), where context is reversed, i.e., for
    * p(w4|w1 w2 w3), you must pass word=w4, and context=(w3,w2,w1).
    * If w4 is not found, uses p(w4) = oov_unigram_prob in the back off
    * calculations, unless isUNK_tagged is true, in which case the LM's
    * predictions for unknown words applies.
    * @param word               word whose prob is desired
    * @param context            context for word, in reverse order
    * @param context_length     length of context
    * @return log(p(word|context))
    */
   virtual float wordProb(Uint word, const Uint context[], Uint context_length) = 0;

   /**
    * Get the order of the language model
    * @return Order of the LM
    */
   Uint getOrder();

   /**
    * Clear the read cache.
    *
    * A subclass may use a cache to speed up its queries.  If so, this function
    * should clear that cache, else do nothing.
    *
    * User classes of the LM should call this every now and then to avoid
    * letting the cache grow excessively and waste memory.  It is best to call
    * this method when the vocabulary of a set of consecutive queries changes,
    * e.g., when moving to a new source sentence in translation.
    */
   virtual void clearCache() = 0;

   /**
    * Read a line from a language model file in Doug Paul's ARPA format.
    * @param in     input stream - assumed to be at or immediately after an
    *               n-gram data line; will error(ETFatal) otherwise
    * @param prob   will be set to the prob value found on the line read
    * @param ph     will be set to the phrase found on the line read
    * @param bo_wt  will be set to the back-off weight found on the line read,
    *               or 0.0 if none found
    * @param blank  will be set iff the line read is blank, in which case none
    *               of the other parameters will be modified.
    * @param bo_present will be set iff a back-off is present on the line read
    */
   static void readLine(istream &in, float &prob, string &ph,
                        float &bo_wt, bool &blank, bool &bo_present);

   static const char* UNK_Symbol; ///< "<unk>";
   static const char* SentStart;  ///< "<s>";
   static const char* SentEnd;    ///< "</s>";

   /**
    * Checks if a language model's file exists
    * @param  lm_filename  canoe.ini LM's filename
    * @return Returns true if the file exists
    */
   static bool checkFileExists(const string& lm_filename);

   /// Destructor, virtual since we have subclasses
   virtual ~PLM() {}

   /**
    * Get a human readable description of the model.  To override the default
    * value for your derived class, set protected member description above to a
    * new value in your create() function or in your constructor.
    *
    * @return A short string describing this language model
    */
   string describeFeature() const { return description; }

   /**
    * Get some stats on how many time each Ngrams size was hit during
    * translation.
    * @return Returns how many times each N was hit.
    */
   virtual Hits getHits() { return hits; }

}; // PLM

} // Portage

#endif // __PLM_H__
