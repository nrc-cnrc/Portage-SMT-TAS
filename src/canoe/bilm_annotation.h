/**
 * @author Eric Joanis
 * @file bilm_annotation.h
 *
 * Defines the annotation used to store the biphrase for BiLMs.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#ifndef _BILM_ANNOTATION_H_
#define _BILM_ANNOTATION_H_

#include "annotation_list.h"
#include "vocab_filter.h"
#include "word_classes.h"
#include "mm_map.h"

namespace Portage {


class BasicModelGenerator;


class BiLMAnnotation : public PhrasePairAnnotation {
   /// vocabulary of bi-phrase elements
   static VocabFilter* biPhraseVocab;

   /// Initilialize the annotation vocabulary for filtering
   /// Can only be called by BiLMModel.
   /// @param tgtVocab  The global vocab for decoding (e.g., bmg->get_voc())
   static VocabFilter* init(const VocabFilter& tgtVocab);

   friend class BiLMModel;

protected:
   /// BiPhrase for the phrase pair this annotation is attached to
   Phrase bi_phrase;

   /// constructor for Annotator to call
   BiLMAnnotation(const VectorPhrase& bi_phrase) : bi_phrase(bi_phrase) {}

public:
   /// Name of this annotation type
   static const string name;

   /// Clone self.
   virtual PhrasePairAnnotation* clone() const { return new BiLMAnnotation(*this); }

   /// Display self for debugging
   virtual void display(ostream& out) const;

   /// Find the bilm annotation in AnnotationList list
   static BiLMAnnotation* get(const AnnotationList& list) { return PhrasePairAnnotation::get<BiLMAnnotation>(name, list); }

   /// Access the biphrase itself
   const Phrase& getBiPhrase() const { return bi_phrase; }

   /// Annotator class called by the PhraseTable while loading the phrase table.
   class Annotator : public PhrasePairAnnotator, private NonCopyable {
      const Uint type_id;
   protected:
      /// vocab from the phrase table
      const VocabFilter& tgtVocab;
      /// convert Uint wordID to its string representation as a target-language word
      virtual const char* tgtWord(Uint wordID) { return tgtVocab.word(wordID); }
      /// convert a raw char* word to its string representation as a source-language word for this BiLM
      virtual const char* srcWord(const char* rawWord) { return rawWord; }
      /// Convert a char* biToken to its string rep (applying classes of bitokens if necessary)
      virtual const char* biToken(const string& rawBiToken) { return rawBiToken.c_str(); }
      /// Do the real work for annotate()
      void annotate_helper(VectorPhrase& bi_phrase, VocabFilter& biVoc,
            TScore* tscore, //const PhraseTableEntry& entry);
            const char* const src_tokens[], Uint src_word_count, const VectorPhrase& tgtPhrase);
   public:
      explicit Annotator(const VocabFilter& tgtVocab);
      virtual ~Annotator();
      virtual const string& getName() const { return BiLMAnnotation::name; }
      virtual void addSourceSentences(const VectorPSrcSent& sentences);
      virtual void annotateMarkedPhrase(PhraseInfo* newPI, const MarkedTranslation* mark, const newSrcSentInfo* info);
      virtual void annotateNoTransPhrase(PhraseInfo* newPI, const Range &range, const char *word);
      virtual void annotate(TScore* tscore, const PhraseTableEntry& entry);
      /// The four-argument annotate() is for annotating after having read the
      /// phrase table, e.g., phrase pairs that come from a TPPT
      virtual void annotate(TScore* tscore, const char* const src_tokens[],
            Uint src_word_count, const VectorPhrase& tgtPhrase);

      virtual BiLMAnnotation* get(const AnnotationList& list) { return BiLMAnnotation::get(list); }
      virtual VocabFilter* getVoc() { /*cerr << "BILM::getVoc" << endl;*/ return BiLMAnnotation::biPhraseVocab; }
   };
}; // class BiLMAnnotation




class CoarseBiLMAnnotation : public BiLMAnnotation {
   static const string name_prefix;
   static char name_suffix; // Suffix so we can create multiple distinct Coarse BiLM annotators.

protected:
   struct IWordClassesMapping {
      virtual ~IWordClassesMapping() {}
      virtual const char* operator()(const char* word) const { assert(false); }
   };

   struct WordClassesTextMapper : public IWordClassesMapping, private NonCopyable {
      WordClasses word2class;   ///<
      vector<string>& class_str; ///< strings for class numbers

      WordClassesTextMapper(const string& filename, vector<string>& class_str);
      virtual const char* operator()(const char* word) const;
   };

   struct WordClassesMemoryMappedMapper : public IWordClassesMapping, private NonCopyable {
      MMMap word2class;   ///<  Memory Mapped map  word -> class.

      WordClassesMemoryMappedMapper(const string& filename);
      virtual const char* operator()(const char* word) const;
   };

public:
   virtual PhrasePairAnnotation* clone() const { return new CoarseBiLMAnnotation(*this); }
   virtual void display(ostream& out) const;
   class Annotator : public BiLMAnnotation::Annotator {
      const string name;
      const Uint type_id;

      IWordClassesMapping* srcClasses; ///< classes of source tokens
      IWordClassesMapping* tgtClasses; ///< classes of target tokens
      IWordClassesMapping* tgtSrcClasses; ///< classes of tgt/src bitokens (of words or of classes)

      VocabFilter coarseBiPhraseVocab;
      vector<string> class_str; ///< strings for class numbers

      IWordClassesMapping* getWordClassesMapper(const string& filename);

      /// convert Uint wordID to its string representation as a target-language
      /// word, after having applied srcClasses mapping.
      virtual const char* tgtWord(Uint wordID) {
         const char* const word = tgtVocab.word(wordID);
         return tgtClasses != NULL ? (*tgtClasses)(word) : word;
      }
      const char* tgtWord(const char* rawWord) {
         return tgtClasses != NULL ? (*tgtClasses)(rawWord) : rawWord;
      }
      /// convert a raw char* word to its string representation as a source-language
      /// word, after having applied tgtClasses mapping.
      virtual const char* srcWord(const char* rawWord) {
         return srcClasses != NULL ? (*srcClasses)(rawWord) : rawWord;
      }
      /// Convert a char* bitoken to its string rep (applying classes of bitokens if necessary)
      virtual const char* biToken(const string& rawBiToken) {
         return tgtSrcClasses != NULL ? (*tgtSrcClasses)(rawBiToken.c_str()) : rawBiToken.c_str();
      }

      friend class CoarseBiLMAnnotation;
   public:
      explicit Annotator(const VocabFilter& tgtVocab, const vector<string>& clsFiles);
      virtual ~Annotator();

      virtual const string& getName() const { return name; }
      virtual void addSourceSentences(const VectorPSrcSent& sentences);
      virtual void annotateMarkedPhrase(PhraseInfo* newPI, const MarkedTranslation* mark, const newSrcSentInfo* info);
      virtual void annotateNoTransPhrase(PhraseInfo* newPI, const Range &range, const char *word);
      virtual void annotate(TScore* tscore, const PhraseTableEntry& entry);
      virtual void annotate(TScore* tscore, const char* const src_tokens[],
            Uint src_word_count, const VectorPhrase& tgtPhrase);

      virtual CoarseBiLMAnnotation* get(const AnnotationList& list);
      virtual VocabFilter* getVoc() { /*cerr << "COARSE BILM::getVoc" << endl;*/ return &coarseBiPhraseVocab; }

      static bool isMemoryMapped(const string& filename);
   };
private:
   Annotator* annotator; ///< the annotator that was used to create this annotation
   CoarseBiLMAnnotation(Annotator* annotator, const VectorPhrase& bi_phrase) : BiLMAnnotation(bi_phrase), annotator(annotator) {}
}; // class CoarseBiLMAnnotation


} // namespace Portage

#endif // _BILM_ANNOTATION_H_
