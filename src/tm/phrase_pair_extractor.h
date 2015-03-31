/**
 * @author Eric Joanis
 * @file phrase_pair_extractor.h
 * @brief Class for doing phrase pair extraction, one sentence pair at a time.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#ifndef PHRASE_PAIR_EXTRACTOR_H
#define PHRASE_PAIR_EXTRACTOR_H

#include "portage_defs.h"
#include "word_align.h"
#include <string>
#include <vector>
#include <boost/optional/optional.hpp>


namespace Portage {

using boost::optional;

class WordAlignerFactory;
class WordAligner;
class WordAlignerStats;
class IBM1;
class PhraseTableUint;

struct PhrasePairExtractor {
   bool check_args_called;  ///< For internal validation
   string max_phrase_string;
   Uint max_phrase_len1;    ///< Maximum l1 phrase length
   Uint max_phrase_len2;    ///< Maximum l2 phrase length
   Uint max_phraselen_diff; ///< Maximum difference in phrase lengths
   string min_phrase_string;
   Uint min_phrase_len1;    ///< Minimum l1 phrase length
   Uint min_phrase_len2;    ///< Minimum l2 phrase length
   vector<string> align_methods; ///< Full alignment methods (combining the two one-directional alignments)
   string model1;           ///< alignment model 1
   string model2;           ///< alignment model 2
   Uint ibm_num;            ///< Are we doing IBM 1 or 2? 42 means uninitialized
   bool use_hmm;            ///< Are we using HMM?
   Uint verbose;            ///< Are we printing debugging info?

   // More arguments so we can use this class from within gen_phrase_tables.cc
   Uint display_alignments; // 0=none, 1=top, 2=all
   bool twist;
   bool add_single_word_phrases;
   bool allow_linkless_pairs;
   Uint add_word_translations;

   optional<double> p0;
   optional<double> up0;
   optional<double> alpha;
   optional<double> lambda;
   optional<bool> anchor;
   optional<bool> end_dist;
   optional<bool> start_dist;
   optional<bool> final_dist;
   optional<Uint> max_jump;

   optional<double> p0_2;
   optional<double> up0_2;
   optional<double> alpha_2;
   optional<double> lambda_2;
   optional<bool> anchor_2;
   optional<bool> end_dist_2;
   optional<bool> start_dist_2;
   optional<bool> final_dist_2;
   optional<Uint> max_jump_2;

   /// Word alignment model for l2 given l1
   IBM1* ibm_1; 
   /// Word alignment model for l1 given l2
   IBM1* ibm_2;

   /// Aligner factory object
   WordAlignerFactory* aligner_factory;
   /// Aligner objects (one per method in align_methods)
   vector<WordAligner*> aligners;

   /// Constructor
   PhrasePairExtractor()
      : check_args_called(false)
      , max_phrase_len1(4)
      , max_phrase_len2(4)
      , max_phraselen_diff(4)
      , min_phrase_len1(1)
      , min_phrase_len2(1)
      , ibm_num(42)
      , use_hmm(false)
      , verbose(0)
      , display_alignments(0)
      , twist(false)
      , add_single_word_phrases(false)
      , allow_linkless_pairs(false)
      , add_word_translations(0)
      , ibm_1(NULL)
      , ibm_2(NULL)
      , aligner_factory(NULL)
   {}

   /// For debugging purposes: dump all parameters to cerr
   void dumpParameters() const;

   /**
    * ARG reader equivalent: parse all the parameters in argv to initialize all
    * argument parameters above.  Does not actually load any models.  Call
    * loadModels() for that.
    * @param argc  number of elements in argv
    * @param argv  the arguments for this phrase pair extractor, in
    *              command-line argument style
    * @param errors_are_fatal  if true, errors cause error(ETFatal), if false
    *                          errors cause error(ETWarn)
    * @return true iff all was OK
    */
   bool getArgs(int argc, const char* const argv[], bool errors_are_fatal = true);

   /** 
    * Call getArgs with the result of splitting the argument string using the
    * specified separator character(s).
    * @param args list of options separated by any one of the characters in sep
    * @param sep  separated characters
    * @param errors_are_fatal  if true, errors cause error(ETFatal), if false
    *                          errors cause error(ETWarn)
    * @return true iff all was OK
    */
   bool getArgs(const string& args, const char* sep, bool errors_are_fatal = true);

   /**
    * Check the integrity of the parameters, and set some defaults.
    * Could be done in getArgs(), but I intend to allow using this class
    * without using getArgs(), so I want to have it separated.
    * getArgs() calls this method, so you only need to call it if you're
    * setting parameters directly.
    * @param errors_are_fatal  if true, errors cause error(ETFatal), if false
    *                          errors cause error(ETWarn)
    * @return true iff not errors were found
    */
   bool checkArgs(bool errors_are_fatal = true);

   /**
    * Initialize all models, including loading IBM/HMM models and constructing
    * the alignment factory and aligners.
    */
   void loadModels(bool createAligner = true);

   /**
    * Extract all the phrase pairs from a given sentence pair.
    * @param toks1 sentence in language 1 (source sentence)
    * @param toks2 sentence in language 2 (target sentence)
    * @param sets1 buffer for alignments - will get modified, but will not
    *              get set to any guaranteed value.
    * @param restrictSourcePhrase  if non-NULL, only phrase pairs involving
    *              toks1[*restrictSourcePhrase] will be kept.
    * @param pt    destination phrase table
    * @param phrase_pairs  destination phrase pair list, if non NULL
    * @param stats if not NULL, alignments statistics are tallied in stats
    */
   void extractPhrasePairs(
         const vector<string>& toks1, const vector<string>& toks2,
         vector< vector<Uint> >& sets1,
         const pair<Uint,Uint>* restrictSourcePhrase,
         PhraseTableUint& pt,
         //vector<WordAlignerFactory::PhrasePair>* phrase_pairs,
         WordAlignerStats* stats);

   /// Replace occurrences of the phrase table separator by its replacement value.
   const string& remap(const string& s);

   /**
    *
    * @param lang  is source language for tt: 1 or 2.
    * @param pt  is the phrasetable.
    * @param src_word_voc
    * @param tgt_word_voc
    * @param os  is non-NULL, the pairs get written to os instead of being inserted into the phrasetable.
    */
   template <class PT>
   void add_ibm1_translations(Uint lang, PT& pt,
         Voc& src_word_voc, Voc& tgt_word_voc,
         ostream* os = NULL)
   {
      const TTable& tt = (lang == 1 ? ibm_1->getTTable() : ibm_2->getTTable());
      vector<string> words, trans;
      vector<float> probs;
      string green_al(display_alignments > 0 ? " a=0" : "");

      tt.getSourceVoc(words);
      for (vector<string>::const_iterator p = words.begin(); p != words.end(); ++p) {
         bool in_phrase_voc = lang == 1 ? pt.inVoc1(*p) : pt.inVoc2(*p);
         if (!in_phrase_voc && src_word_voc.index(p->c_str()) != src_word_voc.size()) {
            tt.getSourceDistnByDecrProb(*p, trans, probs);
            Uint num_added = 0;
            for (Uint i = 0; num_added < add_word_translations && i < trans.size(); ++i) {
               if (tgt_word_voc.index(trans[i].c_str()) == tgt_word_voc.size()) {
                  //if (verbose > 1) cerr << "D:" << *p << "/" << trans[i] << endl;
                  continue;
               }
               if (lang == 1) {
                  if (os) (*os) << remap(*p) << " ||| " << remap(trans[i]) << " ||| " << 1 << green_al << endl;
                  else pt.addPhrasePair(p, p+1, trans.begin()+i, trans.begin()+i+1, 1, "0");
               }
               else {
                  if (os) (*os) << remap(trans[i]) << " ||| " << remap(*p) << " ||| " << 1 << green_al << endl;
                  else pt.addPhrasePair(trans.begin()+i, trans.begin()+i+1, p, p+1, 1, "0");
               }
               ++num_added;
               if (verbose > 1) cerr << *p << "/" << trans[i] << endl;
            }
         }
      }
   }

   /**
    * Reads the line aligned file pair, performs alginment with all aligners
    * and then calls algo with the alignment.
    *
    * @param file1  is the source filename.
    * @param file1  is the target filename.
    * @param pt  is the phrasetable.
    * @param algo  this could either be the algo to extract phrase pairs or an
    *               algo to calculate (hierarchical) lexicalized distortion.
    * @param src_word_voc
    * @param tgt_word_voc
    */
   template <class ALGO, class PT>
   void alignFilePair(const string& file1, const string& file2,
            PT& pt, ALGO& algo,
            Voc& word_voc_1, Voc& word_voc_2
	    ) {
      if (verbose)
         cerr << "reading " << file1 << "/" << file2 << endl;

      iSafeMagicStream in1(file1);
      iSafeMagicStream in2(file2);

      Uint line_no = 0;
      string line1, line2;
      vector<string> toks1, toks2;
      vector< vector<Uint> > sets1;

      while (getline(in1, line1)) {
         if (!getline(in2, line2)) {
            error(ETFatal, "Line counts differ in file pair %s/%s", file1.c_str(), file2.c_str());
            break;
         }
         ++line_no;

         if (verbose > 1) {
            cerr << "--- " << line_no << " ---" << endl;
            cerr << line1 << endl << line2 << endl;
            cerr << "--- " << endl;
         }

         splitZ(line1, toks1);
         splitZ(line2, toks2);

         // keep track of which words occurred in this corpus, for -w switch
         for (Uint i = 0; i < toks1.size(); ++i)
            word_voc_1.add(toks1[i].c_str());
         for (Uint i = 0; i < toks2.size(); ++i)
            word_voc_2.add(toks2[i].c_str());


         for (Uint i = 0; i < aligners.size(); ++i) {

            aligners[i]->align(toks1, toks2, sets1);

            if (verbose > 1) {
               cerr << "---" << align_methods[i] << "---" << endl;
               aligner_factory->showAlignment(toks1, toks2, sets1);
               cerr << "---" << endl;
            }

            algo(toks1, toks2, sets1, pt, *this);
         }
         if (verbose > 1) cerr << endl; // end of block
         if (verbose == 1 && line_no % 10000 == 0)
            cerr << "line: " << line_no << endl;
      }

      if (getline(in2, line2))
         error(ETFatal, "Line counts differ in file pair %s/%s", file1.c_str(), file2.c_str());
   }

}; // class PhrasePairExtractor


} // namespace Portage

#endif // PHRASE_PAIR_EXTRACTOR_H
