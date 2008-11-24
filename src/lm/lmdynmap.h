/**
 * @author George Foster
 * @file lmdynmap.h   Wrap a language model whose vocabulary is a many-to-one
 * mapping from words in the global vocabulary, eg from normal text to
 * lowercase or accentless or lemmatized or number-tagged text.
 *
 * COMMENTS:
 *
 * This uses only the PLM interface, and so should work with any language
 * model. Of course, you still need to perform the corresponding mapping when
 * training the model, eg map text to lowercase before running ngram-count. See
 * the constructor for the currently available mappings.
 *
 * Implementation note: This is fairly efficient when limit_vocab is in effect,
 * because words are mapped once and for all on construction, and index mapping
 * for probability calculation takes constant time. There is a space penalty
 * for retaining a separate local vocabulary; this is probably not needed after
 * construction with limit_vocab, but given the generally murky semantics of
 * vocab in PLM, I decided not to risk deleting it. When limit_vocab is NOT in
 * effect, things are slower, because the global vocabulary can expand
 * arbitrarily, and the added words might be known to the base model. To avoid
 * repeatedly re-mapping the same word, the local vocab is lazily re-synched
 * with the global one whenever an unknown index is encountered.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef LMDYNMAP_H
#define LMDYNMAP_H

#include "casemap_strings.h"
#include "lm.h"
#include "number_mapper.h"

namespace Portage
{

class LMDynMap : public PLM
{
   // Define the available mappings

   struct Mapping {
      virtual string& operator()(string& in) = 0;
   };

   struct IdentMap : public Mapping {
      virtual string& operator()(string& in) {return in;}
   };

   struct LowerCaseMap : public Mapping {
      CaseMapStrings cms;
      string out;
      virtual string& operator()(string& in) {return cms.toLower(in, out);}
      LowerCaseMap(const char* loc) : cms(loc) {}
   };

   /**
    * Replaces any digits in a word by @.
    * This is a class to handle all number mapping scheme in a uniform way.
    */
   struct Number : public Mapping {
      NumberMapper::baseNumberMapper* mapper;  ///< Object to map digits
      string out;   ///< workspace which is NOT reentrant

      /**
       * Default constructor.
       * @param map_type specifies what kind of number mapping see number_mapper.h
       * @param token    token to be used to substitute digits.
       */
      Number(const string& map_type, const char token = '@')
      : mapper(NumberMapper::getMapper(map_type, token))
      {
         assert(mapper != NULL);
      }
      /// Default Destructor.
      virtual ~Number() {
         if (mapper) delete mapper;
      }
      /**
       * Makes this class a proper functor for VocabFilter.
       * @param in  the input word.
       * @return Returns the transformed in where the digits get substituted by
       *         token.
       */
      virtual string& operator()(string& in) {
         (*mapper)(in, out);
         return out;
      }
   };

   // contents

   bool limit_vocab;            ///< remember value on construction
   PLM* m;                      ///< base model
   VocabFilter* local_voc;      ///< local voc for base model
   vector<Uint> index_map;      ///< global id -> local id

   Mapping *mapping;            ///< functor for current mapping

   Uint getGramOrder() {return m->getOrder();}
   void resyncLocalVoc();       ///< map new global words to local ones

   // This looks a little redundant, but we can't rely on global_index
   // actually mapping to a word in the global_voc.
   Uint localIndex(Uint global_index) {
      if (!limit_vocab && global_index >= index_map.size())
         resyncLocalVoc();
      return global_index < index_map.size() ?
         index_map[global_index] : global_index;
   }

public:
   struct Creator : public PLM::Creator {
      Creator(const string& lm_physical_filename, Uint naming_limit_order);
      virtual bool checkFileExists();
      virtual Uint64 totalMemmapSize();
      virtual PLM* Create(VocabFilter* vocab,
                          OOVHandling oov_handling,
                          float oov_unigram_prob,
                          bool limit_vocab,
                          Uint limit_order,
                          ostream *const os_filtered,
                          bool quiet);
   };

   static const char* header;     ///< DynMap;
   static const char  separator;  ///< ;

   /**
    * Construct. If you add new mappings, please add a description here and
    * in help_the_poor_user in the .cc file.
    * @param name a string in the format maptype%model, where \<model\> is a
    * valid name for a PLM language model, and \<maptype\> specifies the kind
    * of mapping to be performed, one of:
    *    - ident Map chars to themselves (for debugging)
    *    - lower[-LOC] Map characters to lowercase using a CaseMapStrings
    *      object created with locale \<LOC\>. If no \<LOC\> is supplied, use
    *      "en_US.UTF-8".
    *    - simpleNumber Substitute @ for digits in words that only contains
    *      digits and punctuation.
    *    - prefixNumber Substitute @ for digits which are prefix of a word.
    * @param vocab         shared vocab object for all models
    * @param oov_handling  whether the LM is open or closed voc.  See enum
    *                      OOVHandling's documentation for details.
    * @param oov_unigram_prob  The unigram log prob of OOVs (only used if
    *                      oov_handling==ClosedVoc) [typical value: -INFINITY]
    * @param limit_vocab   whether to restrict the LM to words already in vocab
    * @param limit_order   if non-zero, will cause the LM to be treated as
    *                      order limit_order, even if it is actually of a
    *                      higher order.
    *                      If lm_filename ends in \#N, that will also be
    *                      treated as if limit_order=N was specified.
    *                      [typical value: 0]
    * @param os_filtered   Opened stream to output the filtered LM.
    *                      [typical value: NULL]
    *
    * The remaining parameters are the same as for PLM::Create(), and, with the
    * exception of vocab, are used to construct the model described by
    * \<model\>.
    */
   LMDynMap(const string& name, VocabFilter* vocab,
            OOVHandling oov_handling, float oov_unigram_prob,
            bool limit_vocab, Uint limit_order, ostream *const os_filtered);

   /// Destructor.
   ~LMDynMap();

   virtual float wordProb(Uint word, const Uint context[], Uint context_length);
   virtual float cachedWordProb(Uint word, const Uint context[],
                                Uint context_length);
   virtual void clearCache() { m->clearCache(); }

   /**
    * Fixes the relative path for a dynmap.
    * @param  path  full path without file name
    * @param  file  file name of a dynmap that needs to be fix
    * @return Returns the full path and file name of the dynmap
    */
   static string fix_relative_path(const string& path, string file);

   virtual Hits getHits() { assert(m!=NULL); return m->getHits(); }
};

} // Portage

#endif // LMDYNMAP_H
