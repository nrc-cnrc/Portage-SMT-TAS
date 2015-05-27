/**
 * @author Colin Cherry
 * @file nnjm_native.h   Evaluate a plain text nnjm
 * Does not depend on any python
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#ifndef NNJM_NATIVE_H
#define NNJM_NATIVE_H

#include "nnjm_abstract.h"
#include "errors.h"
#include "file_utils.h"
#include "exp_table.h"

namespace Portage {

typedef enum Activation {none,tanh,sigmoid} Activation;

class NNJMEmbed {
private:
   string name;
   Uint voc_n; // How many items in the vocabulary?
   Uint vec_n; // How many dimensions in the embedding vector per word?
   Uint pos_n; // How many positions to be embedded at a time (source window or n-gram order minus 1)
   double** w; // Actual embedding matrix

public:
   NNJMEmbed(iSafeMagicStream& istr, bool binMode);
   ~NNJMEmbed();

   string getName() const {
      return name;
   }

   Uint getPosN() const {
      return pos_n;
   }

   Uint getVecN() const {
      return vec_n;
   }

   Uint getVocN() const {
      return voc_n;
   }

   const double* getW(Uint iVoc) const {
      if(iVoc >= voc_n)
         error(ETFatal,"voc entry %d out of range (< %d)",iVoc,voc_n);
      return w[iVoc];
   }
};



class NNJMLayer {
private:
   string name;
   Uint   in_n;
   Uint   out_n;
   double** w;
   double*  b;
   Activation act;
   ExpTable const * const table;

   // Support function for evalAt
   double activate_at(double d) const;

public:
   NNJMLayer(Uint inSize, iSafeMagicStream& istr, bool binMode, ExpTable const * const table);
   ~NNJMLayer();

   /**
    * Basic evaluation for stacking layers in a network
    */
   template <typename InputIterator, typename OutputIterator>
   void eval(InputIterator beg, InputIterator end, OutputIterator result) const;

   /**
    * Get the value for exactly one position in this layer's output vector
    */
   template <typename InputIterator>
   double evalAt(Uint pos, InputIterator beg, InputIterator end) const;

   /**
    * Partial evaluation, for handling bottom layer without copies
    */
   // Apply b to an output vector of dimensionality out_n
   template <typename OutputIterator>
   void apply_b(OutputIterator result) const;

   // Take part of an input vector and apply w to it, starting at w[modelOffset]
   template <typename InputIterator, typename OutputIterator>
   Uint part_eval(InputIterator beg, InputIterator end, Uint modelOffset, OutputIterator result) const;

   // Apply the activation function to an output vector with dimensionality out_n
   template <typename OutputIterator>
   void apply_activation(OutputIterator result) const;

   /**
    * Accessors
    */
   string getName() const {
      return name;
   }

   Uint getInN() const {
      return in_n;
   }

   Uint getOutN() const {
      return out_n;
   }

};

typedef vector<double>* VDP;
typedef vector< vector< VDP > > EmbedCache;
typedef vector<vector<double> > SrcCache;

class NNJMNative : public NNJMAbstract {
private:
   NNJMEmbed* src_embed;
   NNJMEmbed* tgt_embed;
   EmbedCache src_cache;
   EmbedCache tgt_cache;
   SrcCache   newSrcSentCache;
   const bool isSelfNormalized;
   ExpTable const * const table;

   vector<NNJMLayer*> layers;

   Uint tgt_embed_offset;  ///< Where target embeds start in the vector.

   Uint wordPos2VecPos(Uint pos) const;

   template <typename OutputIterator>
   void embedThroughLayer(NNJMLayer* layer, Uint word, Uint pos, OutputIterator result);

   template <typename OutputIterator>
   void apply(Uint src_pos, OutputIterator result) const;

public:
   Ulong cache_hits;
   Ulong cache_misses;
   Ulong numProbs;
   double mean_logprob;
   double mean_norm;

   /**
    * Construct from _un_pickled Python model, as produced
    * by theano/optimize/unpickle.py
    * @modelfile     model file
    */
   NNJMNative(const string& modelfile, bool isSelfNormalized, bool useLookup);

   /**
    * Destructor
    */
   ~NNJMNative();

   /**
    * Score word in context.
    * @param [src_beg,src_end) source window
    * @param [hist_beg,hist_end) target history
    * @param w word to score
    */
   typedef vector<Uint>::const_iterator VUI;
   double logprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w, Uint src_pos);
   double logprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w);

   void newSrcSent(const vector<Uint>& src_pad, Uint srcWindow);
};

}
#endif
