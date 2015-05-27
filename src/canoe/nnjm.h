/**
 * @author George Foster
 * @file nnjm.h   BBN-inspired Neural Net Joint Model Feature
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 *
 */

#ifndef _NNJM_H_
#define _NNJM_H_

#include <map>
#include "decoder_feature.h"
#include "basicmodel.h"
#include "unal_feature.h"
#include "nnjm_abstract.h"

namespace Portage {

typedef enum NNJM_FORMAT{nrc=0,udem=1,native=2} NNJM_Format;

class NNJM : public DecoderFeature {

   static const Uint max_srcphrase_len = 255; // using Uchar indexing
   static const char* help;
   static const char* tag_prefix; // <TAG>:
   enum KeyWords {ELID, UNK, BOS, EOS, NUMKEYS};
   static const char* keywords[]; // <ELID>, <UNK>, <BOS>, <EOS>

   BasicModelGenerator* bmg;

   Uint srcwindow;
   Uint ngorder;
   bool have_src_tags;          ///< true if src voc contains <TAG>* entries
   bool have_tgt_tags;          ///< true if tgt or out voc contains <TAG>* entries
   bool dump;                   ///< dump testing info to stderr
   bool caching;                ///< cache logprob scores if true
   bool srcSentCaching;         ///< perform caching when encountering a new source sentence.
   bool selfnorm;               ///< skip lm score normalization if true (native only)
   Uint cache_hits;             ///< cumulative across all sentences
   Uint cache_misses;           ///< cumulative across all sentences

   Voc srcvoc;   // for words in source sentence
   Voc tgtvoc;   // for words in target history
   Voc outvoc;   // for predicted words
   map<string,string> tgttags;  // tgtword -> tag
   map<string, NNJMAbstract*> file_to_nnjm; // filename -> NNJMAbstract object
   vector<NNJMAbstract*> nnjm_wraps; // python pickled NNJM or plain txt native NNJM
                                     // one for each level of backoff (may be the same
                                     // model for each. nnjm_wraps[0] is full context
   NNJMAbstract* nnjm_wrap;     // nnjms_wraps[0]
   NNJM_Format format;  // nnjm_wrap format: 0 for nrc, 1 for udem, 2 for native

   typedef UnalFeature::CacheKey CacheKey;
   typedef UnalFeature::CacheKeyHash CacheHash;
   typedef boost::unordered_map<CacheKey,vector<Uchar>, CacheHash> Cache;
   Cache align_cache;   // slen+tlen+alignid -> (tgtpos->srcpos)

   typedef vector<Uint>::const_iterator VUI;

   vector<Uint> tgtind_map;  // global voc indexes -> tgtvoc indexes
   vector<Uint> outind_map;  // global voc indexes -> outvoc indexes

   vector<Uint> src_pad;        // indexed source sentence, with BOS/EOS padding
   VectorPhrase tgt_pad;        // indexed target-hyp prefix, ""
   VectorPhrase tgt_pad_fut;    // future-score target-hyp prefix, ""

   PTrie<double, Empty, false> score_cache; // ngram,w,spos -> score

   // Read a voc in 'num word' format. Return num words beginning w/ tag_prefix.
   Uint readVoc(const string& dir, const string& filename, Voc& voc);

   // Update mapping ind_map from global voc indexes to ind_voc indexes.  This
   // assumes that existing contents (if any) of ind_map are valid, and creates
   // entries only for any global voc indexes that are greater than ind_map's
   // current size.
   void updateIndexMap(const Voc& ind_voc, vector<Uint>& ind_map);

   // Get tpos -> spos map for curr phrase pair from cache (or add to cache).
   vector<Uchar>* getSposMap(const PhraseInfo& pi);

   // Return the position in [0,src_len) that is congruent to i in [0,tgt_len).
   Uint congruentPos(Uint i, Uint tgt_len, Uint src_len) {
      return i * src_len/tgt_len + src_len/(2 * tgt_len);
   }

   // Get an nnjm from its filename
   NNJMAbstract* getNNJM(string dir, string file, bool useLookup) {
      string fullname = adjustRelativePath(dir, file);
      if(file_to_nnjm.find(fullname) != file_to_nnjm.end()) {
         return file_to_nnjm[fullname];
      } else {
         if(!native) {
            error(ETFatal, "The PyWrap version of NNJM is not in PortageII yet. -native is required.");
            /*
            NNJMAbstract* toRet = NNJMs::new_PyWrap(fullname, srcwindow, ngorder, format);
            file_to_nnjm[fullname]=toRet;
            return toRet;
            */
            return NULL;
         }
         else {
            NNJMAbstract* toRet = NNJMs::new_Native(fullname, selfnorm, useLookup);
            file_to_nnjm[fullname]=toRet;
            return toRet;
         }
      }
   }


private:
   // Cached logprob
   double logprob(NNJMAbstract* nnjm, Uint src_pos, VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w) {
      double p = 0.0;
      if (nnjm) {
         if (caching) {
            const Uint len = hist_end-hist_beg;
            Uint ctxt[len+2];
            for (Uint i = 0; i < len; ++i) ctxt[i] = *(hist_beg+i);
            ctxt[len] = w;
            ctxt[len+1] = src_pos;
            if (!score_cache.find(ctxt, len+2, p)) {
               p = nnjm->logprob(src_beg, src_end, hist_beg, hist_end, w, src_pos);
               score_cache.insert(ctxt, len+2, p);
               ++cache_misses;
            } else
               ++cache_hits;
         } else
            p = nnjm->logprob(src_beg, src_end, hist_beg, hist_end, w, src_pos);
      }
      if (dump) dumpContext(src_beg, src_end, hist_beg, hist_end, w, p);
      return p;
   }

protected:   // maybe

   // Return p(w|src_window, h). Where: src_end-src_beg = src_window and
   // hist_end-hist_beg = ngorder-1. src_pos is the position of the start of
   // the source window, for caching.
   double logprob(Uint src_pos, VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w) {
      return logprob(nnjm_wrap, src_pos, src_beg, src_end, hist_beg, hist_end, w);
   }

   // Return max_x p(w|src_window, x,h). Here 0 <= hist_end-hist_beg <
   // ngorder-1, and 'x' is 1 or more target words that precede this sequence
   // in the fixed-length ngorder-word window.
   double maxLogprob(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w) {
      return 0.0;
   }

   // Return an elided log probability using a specialized elided model
   // The parameter elid indicates how many characters have been elided, with 0
   // being a full-context LM
   double elidLogprob(Uint elid, Uint src_pos, VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w) {
      return logprob(nnjm_wraps[elid], src_pos, src_beg, src_end, hist_beg, hist_end, w);
   }

   // Dump current context and resulting score to stderr
   void dumpContext(VUI src_beg, VUI src_end, VUI hist_beg, VUI hist_end, Uint w,
                    double score);

public:

   /**
    * Construct from a string specification.
    * @param bmg
    * @param arg specification or name of a file containing the specification -
    * see help string for format.
    * @param arg_is_filename
    */
   NNJM(BasicModelGenerator* bmg, const string& arg, bool arg_is_filename=true);

   ~NNJM() {
      for(map<string,NNJMAbstract*>::iterator p = file_to_nnjm.begin();
          p != file_to_nnjm.end();
          p++) {
         delete p->second;
      }
   }

   virtual void finalizeInitialization();
   virtual void newSrcSent(const newSrcSentInfo& info);
   virtual double score(const PartialTranslation& pt);

   virtual double precomputeFutureScore(const PhraseInfo& phrase_info);
   virtual double futureScore(const PartialTranslation &trans) {return 0.0;}
   virtual double partialScore(const PartialTranslation &trans) {return 0.0;}

   // Don't need these, since we're covered by LM + coverage vector:
   virtual Uint computeRecombHash(const PartialTranslation &pt) {return 0.0;}
   virtual bool isRecombinable(const PartialTranslation &pt1,
                               const PartialTranslation &pt2) {return true;}
   virtual Uint lmLikeContextNeeded() { return ngorder-1; }
};
}

#endif
