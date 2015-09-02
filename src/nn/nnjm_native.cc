/**
 * @author Colin Cherry
 * @file nnjm_native.cc  Evaluate a plain text nnjm
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#include <iostream>
#include <numeric>
#include <iterator>
#include <functional>
#include <algorithm>
#include <fstream>
#include <cmath>
#include "nnjm_native.h"
#include "binio.h"
#include <iterator>  // std::iterator_traits


// To activate use -DNNJM_MEMORY_FOOTPRINT flag at compile time
#ifdef NNJM_MEMORY_FOOTPRINT
#define NNJM_MEMORY_FOOTPRINT_PRINT(expr) expr
#else
#define NNJM_MEMORY_FOOTPRINT_PRINT(expr)
#endif

using namespace Portage;

// log(exp(x)+exp(y)), dodging underflow and overflow
struct logadd : binary_function<double,double,double> {

   static const double LogBig;  // Will not exp() beyond this value
   static const double LogZero; // Effective log(0)

   double operator() (const double& x, const double& y) const {

      if(x <= -LogBig && y <= -LogBig) return -LogBig;

      // Sort x and y into min and max
      double dMin = x, dMax = y;
      if(dMin > dMax) {
         dMin = y; dMax = x;
      }

      // Numerically stable log add
      if(dMax - dMin > LogBig) {
         return dMax;
      }
      else {
         double dRet = dMin + log(1.0 + exp(dMax-dMin));
         return dRet;
      }
   }
};

const double logadd::LogBig = 50;
const double logadd::LogZero = -1000.0;

NNJMEmbed::NNJMEmbed(iSafeMagicStream& istr, bool binMode)
: voc_n(0) // How many items in the vocabulary?
, vec_n(0)// How many dimensions in the embedding vector per word?
, pos_n(0) // How many positions to be embedded at a time (source window or n-gram order minus 1)
, w(NULL) // Actual embedding matrix
{
   string line;
   vector<string> toks;
   // Get summary from first line
   getline(istr,line);
   splitZ(line,toks);
   name = toks[0];
   voc_n = conv<Uint>(toks[1]);
   vec_n = conv<Uint>(toks[2]);
   pos_n = conv<Uint>(toks[3]);
   // Get embedding parameters from next voc_n lines
   w = new double*[voc_n];
   for(Uint i=0;i<voc_n;i++) {
      w[i] = new double[vec_n];
      if (binMode) {
         BinIO::readbin(istr, w[i], vec_n);
      }
      else {
         getline(istr,line);
         splitZ(line,toks);
         if(toks.size()!=vec_n)
            error(ETFatal,"mismatching vector for w line of embedding %s",name.c_str());
         for(Uint j=0;j<vec_n;j++) {
            w[i][j] = conv<double>(toks[j]);
         }
      }
   }
}

NNJMEmbed::~NNJMEmbed() {
   for(Uint i=0;i<voc_n;i++) {
      delete [] w[i];
   }
   delete [] w;
}



NNJMLayer::NNJMLayer(Uint inSize, iSafeMagicStream& istr, bool binMode, ExpTable const * const table)
: in_n(inSize)
, out_n(0)
, w(NULL)
, b(NULL)
, act(none)
, table(table)
{
   string line;
   vector<string> toks;
   // Get summary from first line
   getline(istr,line);
   splitZ(line,toks);
   name = toks[0];
   if (toks[1].find("tanh")!=string::npos) {
      act = tanh;
   }
   else if(toks[1].find("sigmoid")!=string::npos) {
      act = sigmoid;
   }
   else {
      /* Empty for now */
   }
   out_n = conv<Uint>(toks[2]);

   // Get b from next line
   b = new double[out_n];
   if (binMode) {
      BinIO::readbin(istr, b, out_n);
   }
   else {
      getline(istr,line);
      splitZ(line,toks);
      if(toks.size()!=out_n)
         error(ETFatal,"mismatching vector for b of layer %s",name.c_str());
      for(Uint j=0;j<out_n;j++) {
         b[j] = conv<double>(toks[j]);
      }
   }

   // Get w from next in_n lines
   w = new double*[in_n];
   for(Uint i=0;i<in_n;i++) {
      w[i] = new double[out_n];
      if (binMode) {
         BinIO::readbin(istr, w[i], out_n);
      }
      else {
         getline(istr,line);
         splitZ(line,toks);
         if(toks.size()!=out_n)
            error(ETFatal,"mismatching vector for w line of layer %s",name.c_str());
         for(Uint j=0;j<out_n;j++) {
            w[i][j] = conv<double>(toks[j]);
         }
      }
   }
}

NNJMLayer::~NNJMLayer() {
   delete [] b;
   for(Uint i=0;i<in_n;i++)
      delete[] w[i];
   delete[] w;
}


template <typename InputIterator, typename OutputIterator>
void NNJMLayer::eval(InputIterator beg, InputIterator end, OutputIterator result) const {

   apply_b(result);

   // Apply w
   const Uint in_i = part_eval(beg,end,0,result);
   if(in_i!=in_n) {
      error(ETFatal,"incorrect input length for %s, expected %d, got %d",name.c_str(),in_n,in_i);
   }

   apply_activation(result);
}

template <typename InputIterator>
double NNJMLayer::evalAt(Uint out_i, InputIterator beg, InputIterator end) const {
   double toRet = b[out_i];
   Uint in_i = 0;
   for(InputIterator it_in(beg); it_in!=end; ++it_in) {
      toRet += *it_in * w[in_i][out_i];
      ++in_i;
   }
   return activate_at(toRet);
}


template <typename OutputIterator>
void NNJMLayer::apply_b(OutputIterator result) const {
   std::copy(b,b+out_n,result);
}

template <typename InputIterator, typename OutputIterator>
Uint NNJMLayer::part_eval(InputIterator beg, InputIterator end, Uint modelOffset, OutputIterator result) const {
   // Apply w
   Uint in_i = modelOffset;
   typedef typename std::iterator_traits<InputIterator>::value_type ValueType;
   for (InputIterator it_in(beg); it_in!=end; ++it_in) {
      const ValueType it_in_val = *it_in;
      const double* const w_in_i = w[in_i];
      int out_i = 0;
      for (OutputIterator out(result), out_end(result+out_n); out<out_end; ++out) {
         *out += it_in_val * w_in_i[out_i];
         ++out_i;
      }
      ++in_i;
   }
   return in_i - modelOffset;
}

template <typename OutputIterator>
void NNJMLayer::apply_activation(OutputIterator result) const {
   // Apply non-linear activation function
   if (act == tanh) {
      if (table) {
         for(OutputIterator out(result), out_end(result+out_n); out<out_end; ++out) {
            *out = table->tanh(*out);
         }
      }
      else {
         for(OutputIterator out(result), out_end(result+out_n); out<out_end; ++out) {
            *out = std::tanh(*out);
         }
      }
   }
   else if (act == sigmoid) {
      if (table) {
         for(OutputIterator out(result), out_end(result+out_n); out<out_end; ++out) {
            *out = table->sig(*out);
         }
      }
      else {
         for(OutputIterator out(result), out_end(result+out_n); out<out_end; ++out) {
            *out = 1.0/(1.0+exp(-(*out)));
         }
      }
   }
   else {
      error(ETFatal, "Unsupported activation function.");
   }
}

double NNJMLayer::activate_at(double d) const {
   // Apply non-linear activation function
   if(act==tanh) {
      return std::tanh(d);
   } else if(act==sigmoid) {
      return 1.0/(1.0+exp(-(d)));
   } else {
      return d;
   }
}

static bool isBinary(const string& filename) {
   const size_t dot = filename.rfind(".");
   return dot != string::npos && filename.substr(dot) == ".bin";
}


NNJMNative::NNJMNative(const string& modelfile, bool selfNormalized, bool useLookup)
   : isSelfNormalized(selfNormalized)
   , table(useLookup ? new ExpTable(8.0, 100000) : NULL)
   , tgt_embed_offset(0)
   , cache_hits(0)
   , cache_misses(0)
   , numProbs(0)
   , mean_logprob(0.0)
   , mean_norm(0.0)
{
   // Get ready to read file
   iSafeMagicStream istr(modelfile);
   const bool binMode = isBinary(modelfile);

   // Source embedding
   src_embed = new NNJMEmbed(istr, binMode);
   if(src_embed->getName()!="[source]")
      error(ETFatal, "expected to find [source] embedding first");

   // Target embedding
   tgt_embed = new NNJMEmbed(istr, binMode);
   if(tgt_embed->getName()!="[target]")
      error(ETFatal, "expected to find [target] embedding second");

   // Layers
   Uint next_layer_in =
      (src_embed->getPosN()+tgt_embed->getPosN())*src_embed->getVecN();
   while(!istr.eof() && (layers.empty() || layers.back()->getName()!="[output]")) {
      layers.push_back(new NNJMLayer(next_layer_in, istr, binMode, table));
      next_layer_in = layers.back()->getOutN();
   }
   // Make sure last layer is output layer
   if(layers.back()->getName()!="[output]")
      error(ETFatal, "expected final layer to be [output]");
   // Print any remainder
   string line;
   Uint iRead=0;
   while(getline(istr, line)) iRead++;
   if(iRead!=0) error(ETFatal, "Error, extra line at end of nnjm model file %s", modelfile.c_str());

   // Build caches
   src_cache.clear();
   src_cache.resize(src_embed->getPosN(), vector<VDP>(src_embed->getVocN(),NULL));
   tgt_cache.clear();
   tgt_cache.resize(tgt_embed->getPosN(), vector<VDP>(tgt_embed->getVocN(),NULL));

   tgt_embed_offset = src_embed->getVecN() * src_embed->getPosN();
}

NNJMNative::~NNJMNative() {
   delete table;

   for(Uint i=0; i<src_cache.size(); i++) {
      for(Uint j=0; j<src_cache[i].size(); j++) {
         if(src_cache[i][j]!=NULL) delete src_cache[i][j];
      }
   }

   for(Uint i=0; i<tgt_cache.size(); i++) {
      for(Uint j=0; j<tgt_cache[i].size(); j++) {
         if(tgt_cache[i][j]!=NULL) delete tgt_cache[i][j];
      }
   }

   delete src_embed;
   delete tgt_embed;
   for(Uint i=0;i<layers.size();i++) {
      delete layers[i];
   }

   cerr << "nnjm hidden layer cache hits: " << cache_hits << " misses: " << cache_misses << endl;
   if(mean_norm>1e-8 || abs(mean_logprob)>1e-8)
      cerr << "avgerage logprob: " << mean_logprob << " average norm: " << mean_norm << endl;
}

Uint NNJMNative::wordPos2VecPos(Uint pos) const {
   if(pos < src_embed->getPosN()) {
      // src position
      return pos * src_embed->getVecN();
   }
   else if(pos < src_embed->getPosN() + tgt_embed->getPosN()) {
      // tgt position
      return tgt_embed_offset + (pos - src_embed->getPosN())*tgt_embed->getVecN();
   }
   else {
      // invalid position
      error(ETFatal,"Invalid pos to wordPos2VecPos: %d",pos);
      return -1;
   }
}

template <typename OutputIterator>
void NNJMNative::embedThroughLayer(NNJMLayer* layer, Uint word, Uint pos, OutputIterator result) {

   NNJMEmbed* embed = NULL;
   EmbedCache* cache = NULL;
   Uint prel = -1;
   if(pos < src_embed->getPosN()) {
      embed = src_embed;
      cache = &src_cache;
      prel = pos;
   } else {
      embed = tgt_embed;
      cache = &tgt_cache;
      prel = pos - src_embed->getPosN();
   }

   VDP toAdd = (*cache)[prel][word];
   if(toAdd==NULL) {
      ++cache_misses;
      toAdd = new vector<double>(layer->getOutN(), 0.0);

      layer->part_eval(embed->getW(word),
                       embed->getW(word)+embed->getVecN(),
                       wordPos2VecPos(pos),
                       toAdd->begin());

      (*cache)[prel][word] = toAdd;
   } else ++cache_hits;

   std::transform(toAdd->begin(),toAdd->end(),result,result,std::plus<double>());
}

double NNJMNative::logprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w) {
   if (!newSrcSentCache.empty()) {
      error(ETFatal, "You shouldn't call this logprob when caching is enabled.");
   }
   return logprob(src_beg, src_end, hist_beg, hist_end, w, 0);
}

double NNJMNative::logprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w, Uint src_pos) {
   // Find embedding and immediately apply bottom layer
   NNJMLayer* bottom = layers.front();
   vector<double>* firstHidden = new vector<double>(bottom->getOutN(), 0.0);
   bottom->apply_b(firstHidden->begin());
   Uint pos = 0;
   if (newSrcSentCache.empty()) {
      for(VUI it=src_beg;it!=src_end;it++)
         embedThroughLayer(bottom, *it, pos++, firstHidden->begin());
   }
   else {
      pos = src_embed->getPosN();
      apply(src_pos, firstHidden->begin());
   }

   for(VUI it=hist_beg;it!=hist_end;it++)
      embedThroughLayer(bottom, *it, pos++, firstHidden->begin());
   bottom->apply_activation(firstHidden->begin());

   // Feed through each remaining layer of the network
   NNJMLayer* top = layers.back();
   Uint numFeedForward = layers.size();
   if( isSelfNormalized ) {
      // Self normalized networks can stop one layer early
      numFeedForward = layers.size()-1;
   }
   vector<double>* next = firstHidden;
   for(Uint i=1;i<numFeedForward;i++) {
      vector<double>* output = new vector<double>(layers[i]->getOutN(),0.0);
      layers[i]->eval(next->begin(),next->end(),output->begin());
      delete next; next = output;
   }

   // Special handling for output layer
   double score_w;
   if( isSelfNormalized ) {
      // Self-normalized:
      // We stop one layer early and evaluate the top layer for only one word
      score_w = top->evalAt(w,next->begin(),next->end());
   }
   else {
      // Standard:
      // We have evaluated the top layer for all words, now we normalize the score of w
      const double norm = std::accumulate(next->begin(),next->end(),logadd::LogZero,logadd());
      score_w = next->at(w) - norm;
      numProbs++;
      mean_logprob = mean_logprob + (score_w-mean_logprob)/numProbs;
      mean_norm = mean_norm + (abs(norm) - mean_norm)/numProbs;
   }
   delete next;
   return score_w;
}

template <typename OutputIterator>
void NNJMNative::apply(Uint src_pos, OutputIterator result) const {
   assert(src_pos < newSrcSentCache.size());
   const vector<double>& c = newSrcSentCache[src_pos];
   std::transform(c.begin(), c.end(), result, result, std::plus<double>());
}

void NNJMNative::newSrcSent(const vector<Uint>& src_pad, Uint srcWindow) {
   const Uint src_len(src_pad.size() - srcWindow + 1);
   NNJMLayer const * const bottom = layers.front();

   NNJM_MEMORY_FOOTPRINT_PRINT(
   if (true) {
      Uint size = 0;
      for(Uint i=0; i<src_cache.size(); i++) {
         for(Uint j=0; j<src_cache[i].size(); j++) {
            if (src_cache[i][j] != NULL)
               size += sizeof(double) * src_cache[i][j]->capacity() + sizeof(src_cache[i][j]);
         }
      }
      cerr << "src_embed size: " << size << " Bytes" << endl;

      size = 0;
      for(Uint i=0; i<tgt_cache.size(); i++) {
         for(Uint j=0; j<tgt_cache[i].size(); j++) {
            if (tgt_cache[i][j] != NULL)
               size += sizeof(double) * tgt_cache[i][j]->capacity() + sizeof(tgt_cache[i][j]);
         }
      }
      cerr << "tgt_embed size: " << size << " Bytes" << endl;
   }
   )


   newSrcSentCache.clear();
   newSrcSentCache.resize(src_len, vector<double>(bottom->getOutN(), 0.0));

   for(Uint src_pos(0); src_pos<src_len; ++src_pos) {
      vector<double>& result = newSrcSentCache[src_pos];
      for(Uint i(0); i<srcWindow; ++i) {
         Uint wordIndex = src_pos+i;
         assert(wordIndex < src_pad.size());
         Uint word = src_pad[wordIndex];
         bottom->part_eval(src_embed->getW(word),
                           src_embed->getW(word)+src_embed->getVecN(),
                           wordPos2VecPos(i),
                           result.begin());
      }
   }
   NNJM_MEMORY_FOOTPRINT_PRINT(cerr << "newSrcSentCache: " << sizeof(double) * newSrcSentCache.size() * newSrcSentCache.front().size() << " Bytes" << endl;)
   //cerr << "nnjm hidden layer cache hits: " << cache_hits << " misses: " << cache_misses << endl;  // DEBUGGING
}

