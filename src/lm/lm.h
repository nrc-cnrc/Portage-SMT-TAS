/**
 * @author Bruno Laferrière / Eric Joanis
 * @file lm.h  Abstract class for a language model.
 * $Id$
 *
 *
 * Superclass LM to do abstracts / concrete implementation of language models
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __PLM_H__
#define __PLM_H__

#include <portage_defs.h>
#include <voc.h>
#include <vector>
#include <map>

using namespace std;

namespace Portage
{


/// Abstract class for a language model
class PLM
{
protected:
   /// Vocabulary for this model, possibly shared with other models
   Voc* vocab;

   /**
    * Whether the LM is open vocabulary.
    *
    * Is the LM tagged with "<unk>" or not?
    * True iff "<unk>" is a special word for OOVs
    */
   bool isUNK_tagged;

   /**
    * Lm order (e.g., 3 or 4 gram).
    *
    * Don't query this variable directly, ask through the getOrder() method
    * Sub classes may set this variable in their constructor if the value is
    * already readily available at construction time.
    */
   Uint gram_order;


   /// value for unigram probability of out-of-vocabulary words
   float oov_unigram_prob;

   /**
    * Ask sub class what is the gram order of the LM.  Must be overriden by
    * subclasses.  Must return the correct answer as soon as the contructor has
    * been executed.
    *
    * @return order of the language model
    */
   virtual Uint getGramOrder() = 0;

   /**
    * Contructor - only called by children classes' constructors
    * @param vocab         shared vocab object for all models
    * @param UNK_tag       whether LM is open-voc, with "<unk>" as OOV symbol
    * @param oov_unigram_prob  the unigram prob of OOVs (if !UNK_tag)
    */
   PLM(Voc* vocab, bool UNK_tag, float oov_unigram_prob);
   /**
    * Default contructor for subclasses that might need it.
    */
   PLM();

   /// Human-readable description of this LM
   string description;

public:
   /**
    * Create a PLM.  The type will be determined by the file name.
    * @param lm_filename   The file where the LM is stored
    * @param vocab         shared vocab object for all models
    * @param UNK_tag       whether LM is open-voc, with "<unk>" as OOV symbol
    * @param limit_vocab   whether to restrict the LM to words already in vocab
    * @param limit_order   if non-zero, will cause the LM to be treated as
    *                      order limit_order, even if it is actually of a
    *                      higher order.
    *                      If lm_filename ends in #N, that will also be
    *                      treated as if limit_order=N was specified.
    *                      [typical value: 0]
    * @param oov_unigram_prob  the unigram prob of OOVs (if !UNK_tag)
    *                      [typical value: -INFINITY]
    * @param os_filtered   Opened stream to output the filtered LM.
    */
   static PLM* Create(const string& lm_filename, Voc* vocab, bool UNK_tag,
                      bool limit_vocab, Uint limit_order,
                      float oov_unigram_prob,
                      ostream *const os_filtered = NULL);

   /**
    * The main LM method: probability of a word in context.
    *
    * Calculate p(word|context), where context is reversed, i.e., for
    * p(w4|w1 w2 w3), you must pass word=w4, and context=(w3,w2,w1).
    * If w4 is not found, uses p(w4) = oov_unigram_prob in the back off
    * calculations, unless isUNK_tagged is true, in which case the LM's
    * predictions for unknown words applies.
    * @param word               word whose prob is desired
    * @param context            context for word, in reverse order
    * @param context_length     length of context
    * @return p(word|context)
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

}; // PLM

} // Portage

#endif // __PLM_H__
