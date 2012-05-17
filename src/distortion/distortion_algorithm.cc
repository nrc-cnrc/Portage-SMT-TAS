// $Id$
/**
 * @author Samuel Larkin
 * @file distortion_count.cc
 * @brief Briefly describe your program here.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include "distortion_algorithm.h"

using namespace Portage;


// extract word-based LDM counts for each phrase-pair in a sentence
void Portage::word_ldm_count::operator()(
                    const vector<string>& toks1,
                    const vector<string>& toks2,
                    const vector< vector<Uint> >& sets1,
                    PhraseTableGen<DistortionCount>& pt,
                    PhrasePairExtractor& ppe
                    )
{
   vector<Uint> earliest2;
   vector<int> latest2;
   DistortionCount dc;
   PhraseTableUint dummy;
   vector<WordAlignerFactory::PhrasePair> phrases;

   ppe.aligner_factory->addPhrases(toks1, toks2, sets1,
                              ppe.max_phrase_len1, ppe.max_phrase_len2,
                              ppe.max_phraselen_diff,
                              ppe.min_phrase_len1, ppe.min_phrase_len2,
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

   if (ppe.verbose > 1) cerr << "---" << endl;

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

      if (ppe.verbose > 1) {
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
   PhraseTableUint dummy;
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
   const Uint len1 = toks1.size();
   const Uint len2 = toks2.size();
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
void Portage::hier_ldm_count::operator()(
                    const vector<string>& toks1,
                    const vector<string>& toks2,
                    const vector< vector<Uint> >& sets1,
                    PhraseTableGen<DistortionCount>& pt,
                    PhrasePairExtractor& ppe
                    )
{
   DistortionCount dc;

   // pre-processing to figure out what the valid corners are
   vector< vector<PhraseCorner> >
      corners(toks1.size(),
              vector<PhraseCorner>(toks2.size(),
                                   PhraseCorner()));
   find_corners(toks1, toks2, sets1, *ppe.aligner_factory, corners);

   // not sure if using verbose here is appropriate
   // double-check corner calculations, more than twice as slow, but good
   // for debugging corner propagation
   if(ppe.verbose>2)
   {
      vector< vector<PhraseCorner> >
         corners2(toks1.size(),
                  vector<PhraseCorner>(toks2.size(),
                                       PhraseCorner()));
      find_corners_expensive(toks1, toks2, sets1, *ppe.aligner_factory, corners2);
      if(corners==corners2) {
         ;
      }
      else {
         // Print the corner graph (1 on x axis, 2 on y axis, bottom left is 0,0)
         for(Uint i=0;i<toks1.size();i++) cerr << toks1[i] << " ";
         cerr << endl;
         for(Uint i=0;i<toks2.size();i++) cerr << toks2[i] << " ";
         cerr << endl;
         ppe.aligner_factory->showAlignment(toks1, toks2, sets1);
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
   PhraseTableUint dummy;
   vector<WordAlignerFactory::PhrasePair> phrases;
   ppe.aligner_factory->addPhrases(toks1, toks2, sets1,
                              ppe.max_phrase_len1, ppe.max_phrase_len2,
                              ppe.max_phraselen_diff,
                              ppe.min_phrase_len1, ppe.min_phrase_len2,
                              dummy, Uint(1), &phrases);

   // assign mono/swap/disc status to prev- and next-phrase
   // orientations for each extracted phrase pair
   if (ppe.verbose > 1) cerr << "---" << endl;
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

      if (ppe.verbose > 1) {
         dc.dumpSingleton(cerr);
         cerr << ": ";
         p->dump(cerr, toks1, toks2);
         cerr << endl;
      }
   }
}


