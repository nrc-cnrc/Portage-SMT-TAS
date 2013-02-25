/**
 * @author Darlene Stewart
 * @file pylm.cc
 * @brief Provide LM access from python via a C language interface.
 *
 * No corresponding pylm.h file is provided because these functions are
 * intended to be called from python, not from C/C++.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */
#include <iostream>
#include "lm.h"
#include "logging.h"

using namespace Portage;
using namespace std;

//#define DEBUG
#ifdef DEBUG
   #define debug(expr1, expr2) fprintf(stderr, expr1, expr2)
#else
   #define debug(expr1, expr2)
#endif

extern "C" {
   /**
    * Create a new VocabFilter object.
    * @return  returns a pointer to a new VocabFilter object
    */
   VocabFilter* VocabFilter_new()
   {
      // The following work-around of turning off cerr's auto-flushing
      // is done to prevent a segmentation fault when writing to cerr.
      // This is not needed if the calling python script is run
      // with the following environment setting: LD_PRELOAD=libstdc++.so
      // which causes the correct initialization of C++ exceptions and the
      // C++ standard stream objects.
      // Work-around was placed here because VocabFilter_new must be called first.
      cerr.flags(cerr.flags() & (~ios::unitbuf));

      return new VocabFilter(0);
   }

   /**
    * Delete a VocabFilter.
    * @param vf  pointer to the VocabFilter object to delete
    */
   void VocabFilter_delete(VocabFilter* vf)
   {
      delete vf;
   }

   /**
    * Add a word to a VocabFilter.
    * @param vf  pointer to the VocabFilter object
    * @param word  word to add
    */
   Uint VocabFilter_add(VocabFilter* vf, char* word)
   {
     debug("VocabFilter_add: '%s'\n", word);
     return vf->add(word);
   }

   /**
    * Add multiple words to a VocabFilter.
    * @param vf  pointer to the VocabFilter object
    * @param n  number of words to add
    * @param vocab  array of size n of words to add
    */
   void VocabFilter_add_n(VocabFilter* vf, Uint n, char* vocab[])
   {
      for (Uint i = 0; i < n; ++i)
          vf->add(vocab[i]);
   }

   /**
    * Return the number of unique tokens in a VocabFilter.
    * @param vf  pointer to the VocabFilter object
    * @return  returns the number of tokens in the VocabFilter
    */
   Uint VocabFilter_size(VocabFilter* vf)
   {
     return vf->size();
   }

   /**
    * Return the index of a word in a VocabFilter.
    * @param vf  pointer to the VocabFilter object
    * @param word  word to look up
    * @return  returns the index of the word in the VocabFilter
    */
   Uint VocabFilter_index(VocabFilter* vf, char* word)
   {
     return vf->index(word);
   }

   /**
    * Create a new Portage language model (PLM).
    * @param lmfile  name of the LM file
    * @param vf  pointer to the VocabFilter to use to filter the LM
    * @return  returns a pointer to a new PLM object
    */
   PLM* PLM_create(char* lmfile, VocabFilter* vf)
   {
      static bool logging_init = false;
      if (!logging_init) {
         Logging::init();
         logging_init = true;
      }
      return PLM::Create(lmfile, vf, PLM::SimpleAutoVoc, -INFINITY, true, 0, NULL);
   }

   /**
    * Return the order of a language model.
    * @param plm  pointer to the PLM object
    * @return  returns the language model order
    */
   Uint PLM_getOrder(PLM* plm)
   {
      return plm->getOrder();
   }

   /**
    * Return the log probability of a word in a specific context.
    * @param plm  pointer to the PLM object
    * @param vf  pointer to the corresponding VocabFilter object
    * @param word  word whose prob is desired
    * @param context  context for word, in reverse order
    * @param n  length of context
    * @return  returns the log probability of the word in the specified context
    */
   float PLM_wordProb(PLM* plm, VocabFilter* vf, char* word, char* context[], Uint n)
   {
      Uint icontext[n];
      debug("PLM_wordProb: '%s':", word);
      for (Uint i = 0; i < n; ++i) {
         icontext[i] = vf->index(context[i]);
         debug(" '%s'", context[i]);
      }
      debug("%s", "\n");
      return plm->wordProb(vf->index(word), icontext, n);
   }
}
