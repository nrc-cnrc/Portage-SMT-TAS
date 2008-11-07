/**
 * @author Samuel Larkin
 * @file vocab_filter.cc  Implements Word <-> index mapping, with filtering
 *                        operations for LMs.
 *
 * COMMENTS:
 *
 * This is pretty specific to LMs, so maybe it should be part of the LM module.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include <vocab_filter.h>


using namespace Portage;


// Yes a prime number, just for fun :D
Uint VocabFilter::maxSourceSentence4filtering = 2011;


VocabFilter::VocabFilter(Uint number_source_sents) : 
   per_sentence_vocab(number_source_sents > 0 ? 
		      new MultiVoc(*this, min(number_source_sents, maxSourceSentence4filtering)) : 
		      NULL)
{
   if (per_sentence_vocab) {
      cerr << "LM filtering with per sentence vocab(" << number_source_sents << ")" << endl;
      assert(number_source_sents>0);
   } else {
      assert(number_source_sents==0);
   }
}

VocabFilter::~VocabFilter()
{
   if (per_sentence_vocab) { delete per_sentence_vocab; per_sentence_vocab = NULL; }
}

void VocabFilter::addWord(const string& word, Uint sent_no)
{
   assert(per_sentence_vocab);
   sent_no %= maxSourceSentence4filtering;
   const Uint index = add(word.c_str());
   if (per_sentence_vocab)
      per_sentence_vocab->add(index, sent_no);
}


void VocabFilter::addSentence(const vector<string> sentence, Uint sent_no)
{
   typedef vector<string>::const_iterator Word;
   for (Word w(sentence.begin()); w!=sentence.end(); ++w) {
      addWord(*w, sent_no);
   }
}


void VocabFilter::addSentences(const vector<vector<string> > sentences)
{
   for (Uint s(0); s<sentences.size(); ++s) {
      addSentence(sentences[s], s);
   }
}

void VocabFilter::addSpecialSymbol(const char* symbol)
{
   const Uint symbol_index = add(symbol);
   if (per_sentence_vocab) {
      boost::dynamic_bitset<> all_source_sentences(per_sentence_vocab->get_num_vocs());
      all_source_sentences.set();
      per_sentence_vocab->add(symbol_index, all_source_sentences);
   }
}

bool VocabFilter::keepLMentry(const Uint phrase[/*order*/], Uint tok_count, Uint order)
{
   for (Uint w(0); w<tok_count; ++w) {
      if ( phrase[w] == size() ) {
         // OOV found - skip this line
         ++discarding.oov[order-1];
         return false;
      }
   }

   // If per_sentence_vocab is specified, check that their intersection
   // is non empty.
   if ( per_sentence_vocab && order > 1 ) {
      boost::dynamic_bitset<> intersection(per_sentence_vocab->get_vocs(phrase[0]));
      for ( Uint i = 1; i < tok_count; ++i )
         intersection &= per_sentence_vocab->get_vocs(phrase[i]);
      if ( intersection.none() ) {
         ++discarding.perSentenceVocab[order-1];
         return false;
      }
   }

   return true;
}


void VocabFilter::addPerSentenceVocab(vector<Uint>& tgtPhrase, const boost::dynamic_bitset<>* const input_sent_set)
{
   if (input_sent_set == NULL) return;

   // In limitPhrases mode, we need to add for each word in tgtPhrase to
   // the per_sentence_vocab for each sentence the source phrase
   // occurs in.
   if (per_sentence_vocab) {
      for ( Uint i = 0; i < tgtPhrase.size(); ++i ) {
         per_sentence_vocab->add(tgtPhrase[i], *input_sent_set);
      }
   }
}


Uint VocabFilter::getNumSourceSents() const
{
   return (per_sentence_vocab ? per_sentence_vocab->get_num_vocs() : 0);
}


void VocabFilter::freePerSentenceData()
{
   if (per_sentence_vocab) {
      delete per_sentence_vocab;
      per_sentence_vocab = NULL;
   }
}

