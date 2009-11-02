/**
 * @author George Foster
 * @file dmtrain
 * @brief Make counts for a Moishe-style distortion model
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include <boost/optional/optional.hpp>
#include <iostream>
#include <fstream>
#include <file_utils.h>
#include <arg_reader.h>
#include <ibm.h>
#include <hmm_aligner.h>
#include <phrase_table.h>
#include <word_align.h>
#include "dmstruct.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
dmcount [options] ibm_lang2_given_lang1 ibm_lang1_given_lang2 \n\
                  file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
Count reordering events for a lexicalized distortion model over a set of\n\
line-aligned parallel files, using given IBM/HMM models to perform word\n\
alignment for phrase extraction. Lang1 is assumed to be the source unless the\n\
-r switch is supplied. The output distortion counts are written to stdout, in\n\
the format:  'l1-phrase ||| l2-phrase ||| pm ps pd nm ns nd', where the 'p'\n\
counts pertain to the previous target phrase, and the 'n' counts pertain to the\n\
next target phrase; and 'm', 's', and 'd' designate monotonic, swap, and\n\
discontinuous orientations.\n\
\n\
Options:\n\
\n\
-h     Display this help message and quit.\n\
-H     List available word-alignment methods and quit.\n\
-v     Write progress reports to cerr.\n\
-r     Reverse the roles of the languages: assume lang2 is the source. This is\n\
       equivalent to swapping the positions of all language-dependent arguments.\n\
-a     Word-alignment method and optional args. Use -H for list of methods.\n\
       Multiple methods may be specified by using -a repeatedly. [IBMOchAligner]\n\
-w     Add <nw> best IBM1 translations for src and tgt words that occur in the\n\
       given files but don't have translations in phrase table. Each word pair is\n\
       added with 0 counts, so non-zero smoothing parameters should be used in\n\
       the subsequent dmestm step.\n\
-m     Maximum phrase length. <max> can be in the form 'len1,len2' to specify\n\
       lengths for each language, or 'len' to use one length for both\n\
       languages. 0 means no limit. [4]\n\
-min   Minimum phrase length. <min> can be in the form 'len1,len2' to specify\n\
       lengths for each language, or 'len' to use one length for both\n\
       languages. Has to be at least 1. [1]\n\
-d     Max permissible difference in number of words between source and\n\
       target phrases. [4]\n\
-ibm   Type of ibm models given to the -ibm_* switches: 1, 2, or hmm. This\n\
       may be used to force an HMM ttable to be used as an IBM1, for example.\n\
       [determine type of model automatically from filename]\n\
-lc1   Do lowercase mapping of lang 1 words to match IBM/HMM models, using\n\
       locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-lc2   Do lowercase mapping of lang 2 words to match IBM/HMM models, using\n\
       locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-giza  IBM-style alignments are to be read from files in GIZA++ format,\n\
       rather than computed at run-time; corresponding alignment files \n\
       should be specified after each pair of text files, like this: \n\
       fileN_lang1 fileN_lang2 align_1_to_2 align_2_to_1...\n\
       Notes:\n\
        - you still need to provide IBM models as arguments\n\
        - this currently only works with IBMOchAligner\n\
        - this won't work if you specify more than one aligner\n\
\n\
HMM only options:\n\
\n\
       Some HMM parameters can be modified post loading. See gen_phrase_tables -h\n\
       for details.\n\
";

// Switches

static Uint verbose = 0;
static bool rev = false;
static vector<string> align_methods;
static Uint add_word_translations = 0;
static Uint max_phrase_len1 = 4;
static Uint max_phrase_len2 = 4;
static Uint min_phrase_len1 = 1;
static Uint min_phrase_len2 = 1;
static Uint max_phraselen_diff = 4;
static string ibmtype;
static string lc1;
static string lc2;
static bool giza_alignment = false;

// HMM post-load parameters (intentionally left uninitialized).

static optional<double> p0;
static optional<double> up0;
static optional<double> alpha;
static optional<double> lambda;
static optional<bool> anchor;
static optional<bool> end_dist;
static optional<Uint> max_jump;
static optional<double> p0_2;
static optional<double> up0_2;
static optional<double> alpha_2;
static optional<double> lambda_2;
static optional<bool> anchor_2;
static optional<bool> end_dist_2;
static optional<Uint> max_jump_2;

// Main arguments

static string model1, model2;
static vector<string> textfiles;

static void getArgs(int argc, char* argv[]);

void add_ibm1_translations(Uint lang, TTable& tt, PhraseTableGen<DistortionCount>& pt, 
                           Voc& src_word_voc, Voc& tgt_word_voc);


// main

int main(int argc, char* argv[])
{
   printCopyright(2009, "dmcount");
   getArgs(argc, argv);

   if (align_methods.empty())
      align_methods.push_back("IBMOchAligner");

   IBM1* ibm_1 = NULL;
   IBM1* ibm_2 = NULL;

   if (ibmtype == "hmm") {
      if (verbose) cerr << "Loading HMM models" << endl;
      ibm_1 = new HMMAligner(model1, p0, up0, alpha, lambda, anchor, end_dist, max_jump);
      ibm_2 = new HMMAligner(model2, p0_2, up0_2, alpha_2, lambda_2, anchor_2, end_dist_2, max_jump_2);
   } else if (ibmtype == "1") {
      if (verbose) cerr << "Loading IBM1 models" << endl;
      ibm_1 = new IBM1(model1);
      ibm_2 = new IBM1(model2);
   } else if (ibmtype == "2") {
      if (verbose) cerr << "Loading IBM2 models" << endl;
      ibm_1 = new IBM2(model1);
      ibm_2 = new IBM2(model2);
   } else
      error(ETFatal, "unknown IBM/HHM model type: %s", ibmtype.c_str());
   if (verbose) cerr << "models loaded" << endl;

   CaseMapStrings cms1(lc1.c_str());
   CaseMapStrings cms2(lc2.c_str());
   if (lc1 != "") {
      ibm_1->getTTable().setSrcCaseMapping(&cms1);
      ibm_2->getTTable().setTgtCaseMapping(&cms1);
   }
   if (lc2 != "") {
      ibm_1->getTTable().setTgtCaseMapping(&cms2);
      ibm_2->getTTable().setSrcCaseMapping(&cms2);
   }
    
   WordAlignerFactory aligner_factory(ibm_1, ibm_2, verbose, false, false);
   vector<WordAligner*> aligners;
   for (Uint i = 0; i < align_methods.size(); ++i)
      aligners.push_back(aligner_factory.createAligner(align_methods[i]));

   PhraseTableGen<Uint> dummy;
   PhraseTableGen<DistortionCount> pt;
   Voc word_voc_1, word_voc_2;

   for (Uint fno = 0; fno+1 < textfiles.size(); fno += 2) {
      
      if (verbose)
         cerr << "reading " << textfiles[fno] << "/" << textfiles[fno+1] << endl;
      
      iSafeMagicStream in1(textfiles[fno]);
      iSafeMagicStream in2(textfiles[fno+1]);
      
      Uint line_no = 0;
      string line1, line2;
      vector<string> toks1, toks2;
      vector< vector<Uint> > sets1;
      vector<Uint> earliest2;
      vector<int> latest2;
      vector<WordAlignerFactory::PhrasePair> phrases;
      DistortionCount dc;

      while (getline(in1, line1)) {
         if (!getline(in2, line2)) {
            error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
                  textfiles[fno].c_str(), textfiles[fno+1].c_str());
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
               aligner_factory.showAlignment(toks1, toks2, sets1);
               cerr << "---" << endl;
            }

            phrases.clear();
            aligner_factory.addPhrases(toks1, toks2, sets1,
                                       max_phrase_len1, max_phrase_len2, 
                                       max_phraselen_diff,
                                       min_phrase_len1, min_phrase_len2, 
                                       dummy, Uint(1), &phrases);
            
            // get span for each l2 word: spans for unaligned/untranslated
            // words are deliberately chosen so no mono or swap inferences can
            // be based on them

            earliest2.assign(toks2.size(), toks1.size()+1); 
            latest2.assign(toks2.size(), -1);
            for (Uint ii = 0; ii < sets1.size(); ++ii)
               for (Uint k = 0; k < sets1[ii].size(); ++k) {
                  Uint jj = sets1[ii][k];
                  earliest2[jj] = min(earliest2[jj], ii);
                  latest2[jj] = max(latest2[jj], ii);
               }

            // assign mono/swap/disc status to prev- and next-phrase
            // orientations for each extracted phrase pair

            if (verbose > 1) cerr << "---" << endl;

            vector<WordAlignerFactory::PhrasePair>::iterator p;            
            for (p = phrases.begin(); p != phrases.end(); ++p) {

               dc.clear();

               if (p->end2 < toks2.size()) { // next-phrase orientation
                  if (p->end1 < toks1.size() && earliest2[p->end2] == p->end1)
                     dc.nextmono = 1;
                  else if (p->beg1 > 0 && latest2[p->end2] == (int)p->beg1-1)
                     dc.nextswap = 1;
                  else
                     dc.nextdisc = 1;
               } else {
                  if (p->end1 < toks1.size())
                     dc.nextdisc = 1;
                  else          // assign mono if both phrases are last
                     dc.nextmono = 1;
               }
               if (p->beg2 > 0) { // prev-phrase orientation
                  if (p->beg1 > 0 && latest2[p->beg2-1] == (int)p->beg1-1)
                     dc.prevmono = 1;
                  else if (p->end1 < toks1.size() && earliest2[p->beg2-1] == p->end1)
                     dc.prevswap = 1;
                  else
                     dc.prevdisc = 1;
               } else {
                  if (p->beg1 > 0)
                     dc.prevdisc = 1;
                  else
                     dc.prevmono = 1; // assign mono if both phrases are 1st
               }
               pt.addPhrasePair(toks1.begin()+p->beg1, toks1.begin()+p->end1, 
                                toks2.begin()+p->beg2, toks2.begin()+p->end2, dc);

               if (verbose > 1) {
                  dc.dumpSingleton(cerr);
                  cerr << ": ";
                  p->dump(cerr, toks1, toks2);
                  cerr << endl;
               }
            }

         }
         if (verbose > 1) cerr << endl; // end of block
         if (verbose == 1 && line_no % 1000 == 0)
            cerr << "line: " << line_no << endl;
      }
      
      if (getline(in2, line2))
         error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
               textfiles[fno].c_str(), textfiles[fno+1].c_str());
   }

   if (add_word_translations && ibm_1 && ibm_2) {
      if (verbose) cerr << "ADDING IBM1 translations for untranslated words:" << endl;
      add_ibm1_translations(1, ibm_1->getTTable(), pt, word_voc_1, word_voc_2);
      add_ibm1_translations(2, ibm_2->getTTable(), pt, word_voc_2, word_voc_1);
   }



   pt.dump_joint_freqs(cout);
}   


// lang is source language for tt: 1 or 2

void add_ibm1_translations(Uint lang, TTable& tt, PhraseTableGen<DistortionCount>& pt, 
                           Voc& src_word_voc, Voc& tgt_word_voc)
{
   vector<string> words, trans;
   vector<float> probs;

   tt.getSourceVoc(words);
   for (vector<string>::const_iterator p = words.begin(); p != words.end(); ++p) {
      bool in_phrase_voc = lang == 1 ? pt.inVoc1(*p) : pt.inVoc2(*p);
      if (!in_phrase_voc && src_word_voc.index(p->c_str()) != src_word_voc.size()) {
         tt.getSourceDistnByDecrProb(*p, trans, probs);
         Uint num_added = 0;
         for (Uint i = 0; num_added < add_word_translations && i < trans.size(); ++i) {
            if (tgt_word_voc.index(trans[i].c_str()) == tgt_word_voc.size())
               continue;
            if (lang == 1) {
               pt.addPhrasePair(p, p+1, trans.begin()+i, trans.begin()+i+1);
            } else {
               pt.addPhrasePair(trans.begin()+i, trans.begin()+i+1, p, p+1);
            }
            ++num_added;
            if (verbose > 1) cerr << *p << "/" << trans[i] << endl;
         }
      }
   }
}


// arg processing

void getArgs(int argc, char* argv[])
{
   const string alt_help = WordAlignerFactory::help();
   const char* const switches[] = {
      "v", "r", "s", "a:", "w:", "m:", "min:", "d:", "ibm:",
      "lc1:", "lc2:", "giza",
      "p0:", "up0:", "alpha:", "lambda:", "max-jump:",  
      "anchor", "noanchor", "end-dist", "noend-dist",
      "p0_2:", "up0_2:", "alpha_2:", "lambda_2:", "max-jump_2:", 
      "anchor_2", "noanchor_2", "end-dist_2", "noend-dist_2",
   };
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, -1, help_message,
                        "-h", true, alt_help.c_str(), "-H");
   arg_reader.read(argc-1, argv+1);

   string max_phrase_string;
   string min_phrase_string;
   vector<string> verboses;

   arg_reader.testAndSet("v", verboses);
   arg_reader.testAndSet("r", rev);
   arg_reader.testAndSet("a", align_methods);
   arg_reader.testAndSet("w", add_word_translations);
   arg_reader.testAndSet("m", max_phrase_string);
   arg_reader.testAndSet("min", min_phrase_string);
   arg_reader.testAndSet("d", max_phraselen_diff);
   arg_reader.testAndSet("ibm", ibmtype);
   arg_reader.testAndSet("lc1", lc1);
   arg_reader.testAndSet("lc2", lc2);
   arg_reader.testAndSet("giza", giza_alignment);

   arg_reader.testAndSet("p0", p0);
   arg_reader.testAndSet("up0", up0);
   arg_reader.testAndSet("alpha", alpha);
   arg_reader.testAndSet("lambda", lambda);
   arg_reader.testAndSet("max-jump", max_jump);
   arg_reader.testAndSetOrReset("anchor", "noanchor", anchor);
   arg_reader.testAndSetOrReset("end-dist", "noend-dist", end_dist);
   
   arg_reader.testAndSet("p0_2", p0_2);
   arg_reader.testAndSet("up0_2", up0_2);
   arg_reader.testAndSet("alpha_2", alpha_2);
   arg_reader.testAndSet("lambda_2", lambda_2);
   arg_reader.testAndSet("max-jump_2", max_jump_2);
   arg_reader.testAndSetOrReset("anchor_2", "noanchor_2", anchor_2);
   arg_reader.testAndSetOrReset("end-dist_2", "noend-dist_2", end_dist_2);

   model1 = arg_reader.getVar(0);
   model2 = arg_reader.getVar(1);
   arg_reader.getVars(2, textfiles);

   verbose = verboses.size();

   // initialize *_2 parameters from defaults if not explicitly set
   if (!p0_2) p0_2 = p0;
   if (!up0_2) up0_2 = up0;
   if (!alpha_2) alpha_2 = alpha;
   if (!lambda_2) lambda_2 = lambda;
   if (!max_jump_2) max_jump_2 = max_jump;
   if (!anchor_2) anchor_2 = anchor;
   if (!end_dist_2) end_dist_2 = end_dist;


   if (max_phrase_string.length()) {
      vector<Uint> toks;
      if (!split(max_phrase_string, toks, ",") || toks.empty() || toks.size() > 2)
         error(ETFatal, "bad argument for -m switch");
      max_phrase_len1 = toks[0];
      max_phrase_len2 = toks.size() == 2 ? toks[1] : toks[0];
   }
   if (min_phrase_string.length()) {
      vector<Uint> toks;
      if (!split(min_phrase_string, toks, ",") || toks.empty() || toks.size() > 2)
         error(ETFatal, "bad argument for -min switch");
      min_phrase_len1 = toks[0];
      min_phrase_len2 = toks.size() == 2 ? toks[1] : toks[0];
   }

   if (max_phrase_len1 == 0) max_phrase_len1 = 10000000;
   if (max_phrase_len2 == 0) max_phrase_len2 = 10000000;
   if (min_phrase_len1 == 0) min_phrase_len1 = 1;
   if (min_phrase_len2 == 0) min_phrase_len2 = 1;
   if ( min_phrase_len1 > max_phrase_len1 || min_phrase_len2 > max_phrase_len2 )
      error(ETFatal, "min phrase length can't be greater than max length");

   if (ibmtype == "") {
      if (check_if_exists(HMMAligner::distParamFileName(model1)) &&
	  check_if_exists(HMMAligner::distParamFileName(model2)))
         ibmtype = "hmm";
      else if (check_if_exists(IBM2::posParamFileName(model1)) &&
	       check_if_exists(IBM2::posParamFileName(model2)))
         ibmtype = "2";
      else
	 ibmtype = "1";
   }

   if (giza_alignment)
      error(ETFatal, "GIZA alignment reading not yet implemented");

   if (rev) {
      swap(max_phrase_len1, max_phrase_len2);
      swap(min_phrase_len1, min_phrase_len2);
      swap(lc1, lc2);
      swap(p0, p0_2);
      swap(up0, up0_2);
      swap(alpha, alpha_2);
      swap(lambda, lambda_2);
      swap(anchor, anchor_2);
      swap(end_dist, end_dist_2);
      swap(max_jump, max_jump_2);
      swap(model1, model2);
      for (Uint i = 0; i+1 < textfiles.size(); i += 2)
         swap(textfiles[i], textfiles[i+1]);
   }

}
