/**
 * @author Samuel Larkin
 * @file lattice_overlay_visitor.h  
 *
 * Object used when processing the lattice overlay that outputs the nbest list,
 * ffvals and pal info.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __LATTICE_OVERLAY_VISITOR_H__
#define __LATTICE_OVERLAY_VISITOR_H__

#include <vector>
#include <set>
#include "sparsemodel.h"        // sigh

namespace Portage {
/**
 * Visitor object of a nbest lattice.  It will extract nbest list, ffvals,
 * sfvals, and pal info from the lattice and writes it to the attached stream.
 */
class NbestPrinter {
   protected:
      vector<bool>* oovs;             ///< source sentence OOVs
      BasicModel&   model;            ///< Model which created the states
      ostream*   nbest_stream;        ///< print nbest to this stream
      ostream*   ffvals_stream;       ///< print ffvals to this stream
      ostream*   sfvals_stream;       ///< print sfvals to this stream
      ostream*   pal_stream;          ///< print pal to this stream
      ostream*   debug_stream;        ///< print debugging information to this stream
      vector<double>  global_ffvals;  ///< We need to cumulate each phrase ffval
      map<Uint,double> global_sfvals; ///< Ditto for sparse values
      Uint pal_counter;               ///< Keeps track of the phrase number for pal
      Uint pal_tgt;                   ///< Keeps track of target word count

   public:
      /**
       * Default constructor.
       * @param model  model used to create the lattice we are about to print its nbest
       * @param oovs   oovs for that source sentence
       */
      NbestPrinter(BasicModel& model, vector<bool>* oovs = NULL)
      : oovs(oovs)
      , model(model)
      , nbest_stream(NULL)
      , ffvals_stream(NULL)
      , sfvals_stream(NULL)
      , pal_stream(NULL)
      , debug_stream(NULL)
      , pal_counter(0)
      , pal_tgt(0)
      {}
      ~NbestPrinter() {}

      /**
       * Attaches a stream to output the Nbest.
       * @param stream  the stream to output the Nbest list
       */
      void attachNbestStream(ostream* stream)  { nbest_stream = stream; }
      /**
       * Attaches a stream to output the ffvals.
       * @param stream  the stream to output the ffvals
       */
      void attachFfvalsStream(ostream* stream) { ffvals_stream = stream; }
      /*
       * Attaches a stream to output the sfvals.
       * @param stream  the stream to output the sfvals
       */
      void attachSfvalsStream(ostream* stream) { sfvals_stream = stream; }

      /**
       * Attaches a stream to output the pal.
       * @param stream  the stream to output the pal
       */
      void attachPalStream(ostream* stream)    { pal_stream = stream; }

      /**
       * Attaches a stream to output the debugging info.
       * @param stream  the stream to output the debugging info
       */
      void attachDebugStream(ostream* stream)    { debug_stream = stream; }

      /**
       * Indicates that the end of a sentence was reached.
       */
      void sentenceEnd()
      {
         if (nbest_stream) *nbest_stream << endl;
         if (pal_stream) {
            pal_counter = 0;
            pal_tgt = 0;
            *pal_stream << endl;
         }
         if (ffvals_stream) {
            for (Uint i(0); i<global_ffvals.size(); ++i) {
               if (i>0) *ffvals_stream << "\t";
               *ffvals_stream << global_ffvals[i];
            }   
            //global_ffvals.clear();
            fill(global_ffvals.begin(), global_ffvals.end(), 0.0f);
            *ffvals_stream << endl;
         }  
         if (sfvals_stream) {
            for (map<Uint,double>::iterator p = global_sfvals.begin(); 
                 p != global_sfvals.end(); ++p) {
               if (p != global_sfvals.begin()) *sfvals_stream << ' ';
               *sfvals_stream << p->first << ':' << p->second;
            }   
            global_sfvals.clear();
            *sfvals_stream << endl;
         }
         if (debug_stream)
            *debug_stream << endl << endl;
      }

      /**
       * Process a phrase which is part of the current sentence we are processing.
       * Important this function doesn't output to a file since some data must be cumulated.
       * @param state  the current phrase
       */
      void operator()(const DecoderState *state)
      {
         if (!state) return;

         // print the the current phrase
         if (nbest_stream) {
            // Insert space if this is not the first phrase of the sentence
            if (state->back->back != NULL) *nbest_stream << ' ';
            *nbest_stream << model.getStringPhrase(state->trans->getPhrase());
         }

         // print the pal
         if (pal_stream) {
            const PhraseInfo *phrase = state->trans->lastPhrase;
            if (pal_counter>0) *pal_stream << " ";
            const Uint size = state->trans->getPhrase().size();
            *pal_stream << ++pal_counter << ":"
                        << phrase->src_words.start << "-" << (phrase->src_words.end - 1) << ":"
                        << pal_tgt << "-" << (pal_tgt + size - 1);
            pal_tgt += size;
         }

         // cumulate the ffvals
         if (ffvals_stream) {
            vector<double> ffvals;
            model.getFeatureFunctionVals(ffvals, *state->trans);
            assert(ffvals.size() > 0);
            if (global_ffvals.size() < ffvals.size())
               global_ffvals.resize(ffvals.size(), 0.0f);
            for (Uint i(0); i< global_ffvals.size(); ++i)   
               global_ffvals[i] += ffvals[i];
         }

         // accumulate sparse values (same basic code as PrintFunc::appendSFValues()
         if (sfvals_stream) {
            vector<double> ffvals;
            set<Uint> smvals;
            model.getFeatureFunctionVals(ffvals, *state->trans);
            assert(ffvals.size() > 0);
            Uint sf_offset = ffvals.size();
            for (Uint i = 0; i < ffvals.size(); ++i) {

               // test if this is a SparseModel feature
               SparseModel* sf = dynamic_cast<SparseModel*>(model.getDecoderFeature(i));
               if (sf) {

                  // add SparseModel sub-features IN PLACE OF ith feature
                  sf->getComponents(*state->trans, smvals);
                  for (set<Uint>::iterator p = smvals.begin(); p != smvals.end(); ++p) {
                     pair < map<Uint,double>::iterator, bool> r = 
                        global_sfvals.insert(make_pair(*p + sf_offset, 0));
                     ++r.first->second;
                  }
                  sf_offset += sf->numFeatures();  // in case > 1 SparseModel
               } else {

                  // add normal feature, if non-zero
                  if (ffvals[i] != 0) {
                     pair < map<Uint,double>::iterator, bool> r = 
                        global_sfvals.insert(make_pair(i, 0));
                     r.first->second += ffvals[i];
                  }
               }
            }
         }

         if (debug_stream) {
            state->display(*debug_stream, &model, model.getSourceLength());
            model.scoreTranslation(*state->trans, 3);
         }
      }
};
};

#endif  // __LATTICE_OVERLAY_VISITOR_H__
