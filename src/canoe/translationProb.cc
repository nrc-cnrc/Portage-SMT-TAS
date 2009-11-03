/**
 * @author Samuel Larkin
 * @file translationProb.cc  Contains a class that finds and sums the
 * probability of a translation for all aligments in the lattice.
 *
 * $Id$
 *
 * Translation Probability Calculator
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
#include <translationProb.h>
#include <str_utils.h>
#include <file_utils.h>
#include <math.h>
#include <iostream>   // cerr


using namespace Portage;
using namespace std;


const translationProb::Prob translationProb::Parcouru::NOT_SEEN(-1.0f);


/// Definition for an iterator over a vector of DecoderStates.
typedef VDS::const_iterator dsIT;


/**
 * Verifies if sub is a suffix of sent.
 * @param[in] sent  sentence we are looking a suffix into
 * @param[in] sub   the suffix we are looking for.
 * @return  Returns true if sub is a suffix of sent.
 */
static bool containsSubString(const vector<Uint>& sent, const Phrase& sub)
{
   vector<Uint>::const_reverse_iterator s(sent.rbegin());
   Phrase::const_reverse_iterator t(sub.rbegin());
   unsigned int count(0);
   while (s!=sent.rend() && t!=sub.rend() && (*s) == (*t)) {
      ++s;
      ++t;
      ++count;
   }
   if (t == sub.rend()) assert(count == sub.size());
   return t == sub.rend();
}


double translationProb::recursion(vector<Uint> tokens, const DecoderState * const ds, Parcouru& vu, const SuffixLength& sl) const
{
   // We've not already seen this state
   const Prob dejaVu = vu.find(ds->id, sl);
   if (dejaVu != Parcouru::NOT_SEEN)
   {
      return dejaVu;
   }
   
   // END OF RECURSION
   // have we reached the acceptor state
   if (ds->back == 0) {
      // Entire sentence processed => valid path
      if (tokens.empty())
         return vu.add(ds->id, sl, 1.0f);
      // Still some words in the sentence => invalid path
      else
         return vu.add(ds->id, sl, 0.0f);
   } else {
      // we have exhausted the sentence's words without reaching an acceptor state => reject path
      if (tokens.empty())
         return vu.add(ds->id, sl, 0.0f);
   }
   
   
   Prob prob(0.0f);
   // RECOMBINED STATES
   for (dsIT it(ds->recomb.begin()); it != ds->recomb.end(); ++it)
      prob += recursion(tokens, *it, vu, sl);

   const Phrase& lastPhrase(ds->trans->lastPhrase->phrase);
   Length phraseLength(lastPhrase.size());
   Length sentenceLength(tokens.size());
   assert(phraseLength > 0);
   if (containsSubString(tokens, lastPhrase)) {
      assert(sentenceLength >= phraseLength);   // Can be shrinked
      tokens.resize(sentenceLength - phraseLength);   // Extracting prefix
      assert(tokens.size() < sentenceLength);   // Should have shrink
      const Prob arcwt = exp(ds->score - ds->back->score);
      prob += arcwt * recursion(tokens, ds->back, vu, sl+phraseLength);
   }
   
   
   return vu.add(ds->id, sl, prob);
}


void translationProb::find(const string& filename, const VDS& finalStates, const Prob& dMasse)
{
   // INPUT FILE
   iMagicStream input(filename);
   if (!input) {
      cerr << "Invalid file name: " << filename << endl;
      return;
   }

   // OUTPUT FILE
   const string outputFile(filename + ".subLattice");
   ofstream output(outputFile.c_str());
   if (!output) {
      cerr << "Unable to open output: " << outputFile << endl;
      return;
   }
   
   string hypothese;
   Prob probSubLattice(0.0f);
   while(!input.eof()) {
      getline(input, hypothese);
      Prob probHypothese(0.0f);
      if (hypothese.size() > 0) {
         vector<Uint> tokens;
         getTokens(hypothese, tokens);
         
         Parcouru vu;
         for (dsIT it(finalStates.begin()); it != finalStates.end(); ++it) {
            probHypothese += recursion(tokens, *it, vu);
         }
         
         assert(probHypothese <= dMasse);
         assert(probHypothese > 0.0f);
      
         output << probHypothese << "\t" << hypothese << endl;
      }
      
      probSubLattice += probHypothese;
   }
   assert(probSubLattice <= dMasse);
   fprintf(stderr, "subset masse: %32.32g\n", probSubLattice);
}


void translationProb::getTokens(const string& hypothese, vector<Uint>& tokens) const
{
   typedef vector<string> WORDS;
   typedef WORDS::const_iterator WIT;
   
   WORDS words;
   split(hypothese, words);
   assert(words.size() > 0);
   
   tokens.clear();
   tokens.reserve(words.size());
   for (WIT it(words.begin()); it<words.end(); ++it) {
      tokens.push_back(m_pdm.getUintWord(*it));
      assert(tokens.back() != PhraseDecoderModel::OutOfVocab);
   }
}

