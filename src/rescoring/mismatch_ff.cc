/**
 * @author Nicola Ueffing
 * @file mismatch_ff.cc  Implementation of ParMismatch and QuotMismatch
 * 
 * 
 * COMMENTS: 
 *
 * Mismatch of punctuation marks.
 *   ParMismatchFF:  mismatch between opening and closing brackets within a hypothesis
 *   QuotMismatchFF: mismatch between of quotation marks between source and hypothesis
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "mismatch_ff.h"

using namespace Portage;

ParMismatchFF::ParMismatchFF(const string& dontcare) 
: FeatureFunction(dontcare)
{}

bool ParMismatchFF::parseAndCheckArgs()
{
   if (!argument.empty())
      error(ETWarn, "ParMismatchFF doesn't require arguments");
   return true;
}

void ParMismatchFF::source(Uint s, const Nbest * const nbest)
{
   FeatureFunction::source(s, nbest);

   scores.clear();

   for (Uint i = 0; i < K; ++i) {

      const Tokens& hypi = (*nbest)[i].getTokens();
      Uint   bro = 0, mis = 0;

      // count closing brackets which have never been opened and unclosed opening brackets
      for (Uint j=0; j<hypi.size(); j++) {
         if (hypi[j]==")")
            if (bro==0) mis++;
            else        bro--;
         else 
            if (hypi[j]=="(") bro++;
      } // for j
      mis += bro;
      scores.push_back(mis);
   } // for i
}

/****************************************************************************/
QuotMismatchFF::QuotMismatchFF(const string& args) 
: FeatureFunction(args)
{}

bool QuotMismatchFF::parseAndCheckArgs()
{
   if (argument.size() != 2) {
      error(ETWarn, "QuotMismatchFF feature requires 2-character argument for src/tgt languages");
      return false;
   }

   srclang = argument[0];
   tgtlang = argument[1];
   cerr << "Quotation match for translation from " << srclang << " into " << tgtlang << endl
      << "NOTE: This feature detects only very few Chinese quotation marks, namely those used in GALE!" << endl
      << "      For other languages, only '\"' is recognized" << endl;
   if (srclang!='c' && srclang!='e' && srclang!='f' && srclang!='s') {
      error(ETWarn, "Source language has to be one of the following: c(hinese), e(nglish), f(rench), s(panish)");
      return false;
   }
   if (tgtlang!='c' && tgtlang!='e' && tgtlang!='f' && tgtlang!='s') {
      error(ETWarn, "Target language has to be one of the following: c(hinese), e(nglish), f(rench), s(panish)");
      return false;
   }

   return true;
}


void QuotMismatchFF::source(Uint s, const Nbest * const nbest)
{
   FeatureFunction::source(s, nbest);

   scores.clear();

   // Numbers of opening, closing and 'neutral' quotation marks in source and hypothesis
   int src_quo = 0, src_quc = 0, src_qun = 0; 
   int hyp_quo = 0, hyp_quc = 0, hyp_qun = 0;

   const Tokens& src = (*src_sents)[s].getTokens();
   // Determine counts for source sentence
   findQuot(src, srclang, src_quo, src_quc, src_qun);

   // If source contains only 'neutral' quotation marks
   if (src_quo+src_quc==0) {
      //cerr << "Source contains only 'neutral' quotation marks: " << src_qun << endl << "===" << endl;
      for (Uint k = 0; k < K; ++k) {
         hyp_quo = 0;
         hyp_quc = 0;
         hyp_qun = 0;
         const Tokens& hypk = (*nbest)[k].getTokens();
         findQuot(hypk, tgtlang, hyp_quo, hyp_quc, hyp_qun);       
         scores.push_back(abs(src_qun - hyp_qun - hyp_quo -hyp_quc));
      } // for k
   } // if
   // Non-Chinese target language: only 'neutral' quotation marks
   else if (tgtlang!='c') {
      //cerr << "Source contains " << src_quo << " opening, " << src_quc << " closing, and " << src_qun << " neutral quotation marks" << endl << "===" << endl;
      for (Uint k = 0; k < K; ++k) {
         hyp_quo = 0;
         hyp_quc = 0;
         hyp_qun = 0;
         const Tokens& hypk = (*nbest)[k].getTokens();
         findQuot(hypk, tgtlang, hyp_quo, hyp_quc, hyp_qun);     
         scores.push_back(abs(src_qun + src_quo + src_quc - hyp_qun));
      } // for k
   }
   else {
      for (Uint k = 0; k < K; ++k) {
         hyp_quo = 0;
         hyp_quc = 0;
         hyp_qun = 0;
         const Tokens& hypk = (*nbest)[k].getTokens();
         findQuot(hypk, tgtlang, hyp_quo, hyp_quc, hyp_qun);     
         // If hyp contains only 'neutral' quotation marks       
         if (hyp_quo+hyp_quc==0) {
            scores.push_back(abs(src_qun + src_quo + src_quc - hyp_qun));
            //cerr << "hyp contains only 'neutral' quotation marks : " << scores.back() << endl;
         }
         // Both source and hyp contain different types of quotation marks
         // -> each of them has to match
         else
            scores.push_back(abs(src_qun - hyp_qun) + abs(src_quo - hyp_quo) + abs(src_quc - hyp_quc));
      } // for k
   }
}


void QuotMismatchFF::findQuot(const Tokens& sent, const char lang, int &quo, int &quc, int &qun) {
   if (lang!='c') {
      for (Tokens::const_iterator w=sent.begin(); w!=sent.end(); w++) {
         if (*w=="\"")
            ++qun; 
      }
   }
   // Chinese
   else {
      // Chinese has to be encoded in utf-8, assuming GALE conventions here
      /*
         for (uint i=0; i<sent.size(); i++) {
         Token w = sent[i];
         cerr << i << "-";
         for (uint k=0; k<w.size(); k++) {
         int c = (int)(unsigned char)(w[k]);
         cerr << c << ":";
         }
         cerr << " ";
         }
         cerr << endl;
       */

      for (Uint i=0; i<sent.size(); ++i) {
         const Token& w = sent[i];

         for (Uint k=0; k<w.size(); ++k) {
            const int c = (int)(unsigned char)(w[k]);

            if (c>223 && c<240) { // first byte in 3-byte character

               //cerr << "Looking for quot. mark in " << i << "-th word, pos. " << k << " " << (int)(unsigned char)(w[k]) << ":" << (int)(unsigned char)(w[k+1]) << ":" << (int)(unsigned char)(w[k+2]) << " " << endl;

               const int c1 = (int)(unsigned char)(w[k+1]);
               const int c2 = (int)(unsigned char)(w[k+2]);
               if (c==226 && c1==128 && c2==156) {
                  ++quo; 
                  //cerr << "Found opening quot. mark 1" << endl;
               }
               else if (c==227 && c1==128 && c2==140) {
                  ++quo; 
                  //cerr << "Found opening quot. mark 2" << endl;
               }
               else if (c==226 && c1==128 && c2==157) {
                  ++quc; 
                  //cerr << "Found closing quot. mark 1" << endl;
               }
               else if (c==227 && c1==128 && c2==141) {
                  ++quc; 
                  //cerr << "Found closing quot. mark 2" << endl;
               }
               k += 2;
            } // if (c>223 && c<240) 
         } // for k
      } // for i
   } // else
}
