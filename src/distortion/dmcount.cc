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
-hier  Extract hierarchical LDM counts rather than word-based LDM counts.\n\
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
static bool hierarchical = false;

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

// Grunt work of word-based (Moses-style) LDM counting
void word_ldm_count(
                    //const vector<WordAlignerFactory::PhrasePair>& phrases,
                    PhraseTableGen<DistortionCount>& pt,
                    const vector<string>& toks1,
                    const vector<string>& toks2,
                    const vector< vector<Uint> >& sets1,
                    WordAlignerFactory& aligner_factory
                    );

// Grunt work of hierarchical LDM counting (Galley and Manning 2008)
void hier_ldm_count(
                    //const vector<WordAlignerFactory::PhrasePair>& phrases,
                    PhraseTableGen<DistortionCount>& pt,
                    const vector<string>& toks1,
                    const vector<string>& toks2,
                    const vector< vector<Uint> >& sets1,
                    WordAlignerFactory& aligner_factory
                    );

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

            if(hierarchical)
               hier_ldm_count(pt, toks1, toks2, sets1, aligner_factory);
            else
               word_ldm_count(pt, toks1, toks2, sets1, aligner_factory);
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

// extract word-based LDM counts for each phrase-pair in a sentence
void word_ldm_count(
                    PhraseTableGen<DistortionCount>& pt,
                    const vector<string>& toks1,
                    const vector<string>& toks2,
                    const vector< vector<Uint> >& sets1,
                    WordAlignerFactory& aligner_factory
                    )
{
   vector<Uint> earliest2;
   vector<int> latest2;
   DistortionCount dc;
   PhraseTableGen<Uint> dummy;
   vector<WordAlignerFactory::PhrasePair> phrases;

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

struct PhraseCorner {
   // Dim1=b|e (x axis), Dim2=b|e (y axis)
   // bb = begin begin = bottom left
   bool be;  // Grid position a valid top-left corner?
   bool ee;  // Grid position a vlaid top-right corner?
   bool bb;  // Grid position a valid bottom-left corner?
   bool eb;  // Grid position a valid bottom-right corner?

   PhraseCorner() {
      be = ee = bb = eb = false;
   }

   string toString() {
      string toRet = be ? "T" : "F";
      toRet += ee ? "T" : "F";
      toRet += "/";
      toRet += bb ? "T" : "F";
      toRet += eb ? "T" : "F";
      return toRet;
   }
};

bool operator==(const PhraseCorner& x, const PhraseCorner& y)
{
   if(x.be==y.be &&
      x.ee==y.ee &&
      x.bb==y.bb &&
      x.eb==y.eb)
      return true;
   else
      return false;
}

struct Span {
   Uint lt;
   Uint rt;

   Span(Uint l, Uint r) {
      lt = l;
      rt = r;
   }
};

void build_span_mirror(const vector<Span>& pointMirror,
                       vector< vector<Span> >& spanMirror
                       )
{
   for(Uint i=0; i<pointMirror.size(); ++i)
   {
      spanMirror[i][i].lt = pointMirror[i].lt;
      spanMirror[i][i].rt = pointMirror[i].rt;
      for(Uint j=i+1; j<pointMirror.size(); ++j)
      {
         spanMirror[i][j].lt = min(spanMirror[i][j-1].lt, pointMirror[j].lt);
         spanMirror[i][j].rt = max(spanMirror[i][j-1].rt, pointMirror[j].rt);
      }
   }
}

void find_corners_expensive(const vector<string>& toks1,
                  const vector<string>& toks2,
                  const vector< vector<Uint> >& sets1,
                  WordAlignerFactory& aligner_factory,
                  vector< vector<PhraseCorner> >& corners
                  )
{
   const uint BIG = 1000;
   const uint SMALL = 1;
   PhraseTableGen<Uint> dummy;
   vector<WordAlignerFactory::PhrasePair> unlimPhrases;
   aligner_factory.addPhrases(toks1, toks2, sets1,
                              BIG, BIG,
                              BIG,
                              SMALL, SMALL,
                              dummy, Uint(1), &unlimPhrases);
   vector<WordAlignerFactory::PhrasePair>::iterator p;
   for (p = unlimPhrases.begin(); p != unlimPhrases.end(); ++p) {
      // Set corners
      corners[p->beg1][p->beg2].bb = true;
      corners[p->beg1][p->end2-1].be = true;
      corners[p->end1-1][p->beg2].eb = true;
      corners[p->end1-1][p->end2-1].ee = true;
   }
}

void find_corners(const vector<string>& toks1,
                  const vector<string>& toks2,
                  const vector< vector<Uint> >& sets1,
                  WordAlignerFactory& aligner_factory,
                  vector< vector<PhraseCorner> >& corners
                  )
{
   // Collect left/right alignment statistics : O(number of links)=O(n^2)
   vector<bool> aligned1(toks1.size(), false);
   vector<bool> aligned2(toks2.size(), false);
   vector<Span> pointMirror1(toks1.size(), Span(toks2.size(),0));
   vector<Span> pointMirror2(toks2.size(), Span(toks1.size(),0));
   for(Uint i=0; i<sets1.size(); ++i)
   {
      if(sets1[i].size() && i<toks1.size()) {
         aligned1[i] = true;
         pointMirror1[i].lt = sets1[i].front();
         pointMirror1[i].rt = sets1[i].back();
      }
      for(Uint j=0; j<sets1[i].size(); j++)
      {
         Uint jj = sets1[i][j];
         if(jj<toks2.size()) {
            aligned2[jj] = true;
            pointMirror2[jj].lt = min(pointMirror2[jj].lt, i);
            pointMirror2[jj].rt = max(pointMirror2[jj].rt, i);
         }
      }
   }

   // Collect left/right span statistics : O(n^2)
   vector< vector<Span> > spanMirror1(toks1.size(),
                                      vector<Span>(toks1.size(),
                                                   Span(toks2.size(),0)));
   build_span_mirror(pointMirror1, spanMirror1);

   vector< vector<Span> > spanMirror2(toks2.size(),
                                      vector<Span>(toks2.size(),
                                                   Span(toks1.size(),0)));
   build_span_mirror(pointMirror2, spanMirror2);

   // Enumeration of tight phrases : O(n^2)
   for(Uint i=0; i<spanMirror1.size(); ++i)
   {
      if(aligned1[i]) { // If not null/empty
         for(Uint j=i; j<spanMirror1.size(); ++j)
         {
            if(aligned1[j]) { // If not null/empty
               Span mirror2 = spanMirror1[i][j];
               if(mirror2.lt < toks2.size() &&
                  mirror2.rt < toks2.size() &&
                  mirror2.lt <= mirror2.rt) {
                  Span mirror1 = spanMirror2[mirror2.lt][mirror2.rt];
                  if(mirror1.lt < toks1.size() &&
                     mirror1.rt < toks1.size() &&
                     mirror1.lt <= mirror1.rt &&
                     mirror1.lt >= i &&
                     mirror1.rt <= j
                     )
                  {
                     // Valid tight phrase!
                     corners[i][mirror2.lt].bb = true;
                     corners[i][mirror2.rt].be = true;
                     corners[j][mirror2.lt].eb = true;
                     corners[j][mirror2.rt].ee = true;
                  }
               }
            }
         }
      }
   }

   // Corner propagation to handle unaligned words
   Uint len1 = toks1.size();
   Uint len2 = toks2.size();
   if(len1>1)
   {
      // Propogate top-right, bottom-right from left to right : O(n^2)
      for(Uint i=1; i<len1; ++i)
      {
         if(!aligned1[i]) { // Tok i is unaligned
            for(Uint j=0; j<len2; ++j)
            {
               if(corners[i-1][j].eb) corners[i][j].eb=true;
               if(corners[i-1][j].ee) corners[i][j].ee=true;
            }
         }
      }

      // Propogate top-left, bottom-left from right to left : O(n^2)
      for(Uint i=toks1.size()-2; i!=(Uint)-1; --i)
      {
         if(!aligned1[i]) { // Tok i is unaligned
            for(Uint j=0; j<toks2.size(); ++j)
            {
               if(corners[i+1][j].bb) corners[i][j].bb=true;
               if(corners[i+1][j].be) corners[i][j].be=true;
            }
         }
      }
   }

   if(len2>1)
   {
      // Propogate top-left, top-right from bottom to top : O(n^2)
      for(Uint j=1; j<toks2.size(); j++)
      {
         if(!aligned2[j]) { // Tok j is unaligned
            for(Uint i=0;i<toks1.size();++i)
            {
               if(corners[i][j-1].be) corners[i][j].be=true;
               if(corners[i][j-1].ee) corners[i][j].ee=true;
            }
         }
      }

      // Propogate bottom-left, bottom-right from top to bottom : O(n^2)
      for(Uint j=toks2.size()-2; j!=(Uint)-1; --j)
      {
         if(!aligned2[j]) { //Tok j is unaligned
            for(Uint i=0;i<toks1.size();++i)
            {
               if(corners[i][j+1].bb) corners[i][j].bb=true;
               if(corners[i][j+1].eb) corners[i][j].eb=true;
            }
         }
      }
   }
}

// extract hierarchical LDM counts for each phrase-pair in a sentence
void hier_ldm_count(
                    PhraseTableGen<DistortionCount>& pt,
                    const vector<string>& toks1,
                    const vector<string>& toks2,
                    const vector< vector<Uint> >& sets1,
                    WordAlignerFactory& aligner_factory
                    )
{
   DistortionCount dc;

   // pre-processing to figure out what the valid corners are
   vector< vector<PhraseCorner> >
      corners(toks1.size(),
              vector<PhraseCorner>(toks2.size(),
                                   PhraseCorner()));
   find_corners(toks1,toks2,sets1,aligner_factory,corners);

   // not sure if using verbose here is appropriate
   // double-check corner calculations, more than twice as slow, but good
   // for debugging corner propagation
   if(verbose>2)
   {
      vector< vector<PhraseCorner> >
         corners2(toks1.size(),
                  vector<PhraseCorner>(toks2.size(),
                                       PhraseCorner()));
      find_corners_expensive(toks1,toks2,sets1,aligner_factory,corners2);
      if(corners==corners2) {
         ;
      } else {
         // Print the corner graph (1 on x axis, 2 on y axis, bottom left is 0,0)
         for(Uint i=0;i<toks1.size();i++) cerr << toks1[i] << " ";
         cerr << endl;
         for(Uint i=0;i<toks2.size();i++) cerr << toks2[i] << " ";
         cerr << endl;
         aligner_factory.showAlignment(toks1,toks2,sets1);
         for(Uint j=toks2.size()-1;j!=(Uint)-1;j--)
         {
            for(Uint i=0;i<toks1.size();i++)
            {
               if(corners[i][j]==corners2[i][j]) {
                  cerr << corners[i][j].toString() << " ";
               }
               else {
                  cerr
                     << corners[i][j].toString() << "!="
                     << corners2[i][j].toString() << " ";
               }
            }
            cerr << endl;
         }
      }
   }

   // moved finding phrases to inside the method-specific function,
   // just in case someone wants to fold that operation into another task
   // (such as finding corners)
   PhraseTableGen<Uint> dummy;
   vector<WordAlignerFactory::PhrasePair> phrases;
   aligner_factory.addPhrases(toks1, toks2, sets1,
                              max_phrase_len1, max_phrase_len2,
                              max_phraselen_diff,
                              min_phrase_len1, min_phrase_len2,
                              dummy, Uint(1), &phrases);

   // assign mono/swap/disc status to prev- and next-phrase
   // orientations for each extracted phrase pair
   if (verbose > 1) cerr << "---" << endl;
   vector<WordAlignerFactory::PhrasePair>::iterator p;
   for (p = phrases.begin(); p != phrases.end(); ++p) {

      dc.clear();

      if (p->end2 < toks2.size()) { // next-phrase orientation
         if (p->end1 < toks1.size() && corners[p->end1][p->end2].bb)
            dc.nextmono = 1;
         else if (p->beg1 > 0 && corners[p->beg1-1][p->end2].eb)
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
         if (p->beg1 > 0 && corners[p->beg1-1][p->beg2-1].ee)
            dc.prevmono = 1;
         else if (p->end1 < toks1.size() && corners[p->end1][p->beg2-1].be)
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

// arg processing

void getArgs(int argc, char* argv[])
{
   const string alt_help = WordAlignerFactory::help();
   const char* const switches[] = {
      "v", "r", "s", "a:", "w:", "m:", "min:", "d:", "ibm:",
      "lc1:", "lc2:", "giza", "hier",
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
   arg_reader.testAndSet("hier", hierarchical);

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
