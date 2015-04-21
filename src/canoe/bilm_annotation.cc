/**
 * @author Eric Joanis
 * @file bilm_annotation.cc
 *
 * Implements the annotation used to store the biphrase for BiLMs.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "bilm_annotation.h"
#include "alignment_annotation.h"
#include "phrasetable.h"
#include "word_align_io.h"
#include "alignment_freqs.h"
#include "basicmodel.h"
#include "new_src_sent_info.h"
#include "compact_phrase.h"
#include "lm.h"

//===================================================================
// Regular BiLM annotation/annotator
//===================================================================

const string BiLMAnnotation::name = "bilm";
VocabFilter* BiLMAnnotation::biPhraseVocab = NULL;

VocabFilter* BiLMAnnotation::init(const VocabFilter& tgtVocab)
{
   // We need to create the biPhraseVocab with the same number of source
   // sentences as the main vocabulary object, for filtering the BiLM as we
   // load it.
   if (!biPhraseVocab) {
      biPhraseVocab = new VocabFilter(tgtVocab.getNumSourceSents());
   } else {
      if (biPhraseVocab->getNumSourceSents() != tgtVocab.getNumSourceSents())
         error(ETFatal, "Called BiLMAnnotation::init() twice with different numbers of source sentences, I can't work with that!");
   }

   assert(biPhraseVocab);
   return biPhraseVocab;
}

void BiLMAnnotation::display(ostream& out) const
{
   if (!bi_phrase.empty())
      out << "\tbi phrase             "
          << phrase2string(bi_phrase, *biPhraseVocab)
          << endl;
}

BiLMAnnotation::Annotator::Annotator(const VocabFilter& tgtVocab)
   : type_id(AnnotationList::addAnnotationType(BiLMAnnotation::name.c_str()))
   , tgtVocab(tgtVocab)
{
   BiLMAnnotation::init(tgtVocab);
}

BiLMAnnotation::Annotator::~Annotator()
{
   //cerr << "BiLMAnnotation::Annotator::~Annotator() for " << this << endl;
   *(const_cast<Uint*>(&type_id)) = 0xdea110cd;
}

void BiLMAnnotation::Annotator::addSourceSentences(const VectorPSrcSent& sents)
{
   //cerr << "Called BiLMAnnotation::Annotator::addSourceSentences" << endl;
   // Add BiLM words for each potential no-trans option, since BiLM features are used.
   for (Uint s(0); s<sents.size(); ++s) {
      const vector<string>& src_sent = sents[s]->src_sent;
      for (Uint i(0); i<src_sent.size(); ++i)
         BiLMAnnotation::biPhraseVocab->addWord((src_sent[i] + BiLMWriter::sep + src_sent[i]).c_str(), s);

      // Add marked phrases to the vocab too, by creating bogus annotations
      // now, that we'll discard right away, but whose creation has the desired
      // side effects
      for (Uint i(0); i < sents[s]->marks.size(); ++i) {
         PhraseInfo bogus_pi;
         annotateMarkedPhrase(&bogus_pi, &(sents[s]->marks[i]), sents[s].get());
      }
   }
}

void BiLMAnnotation::Annotator::annotateMarkedPhrase(PhraseInfo* newPI, const MarkedTranslation* mark, const newSrcSentInfo* info)
{
   VectorPhrase bi_phrase;

   for ( vector<string>::const_iterator jt = mark->markString.begin();
         jt != mark->markString.end(); jt++) {
      bi_phrase.push_back(
         BiLMAnnotation::biPhraseVocab->add(
            (jt->c_str() +
             BiLMWriter::sep +
             join(info->src_sent.begin()+mark->src_words.start, info->src_sent.begin()+mark->src_words.end, BiLMWriter::sep)
            ).c_str()));
   }

   newPI->annotations.setAnnotation(type_id, new BiLMAnnotation(bi_phrase));
}

void BiLMAnnotation::Annotator::annotateNoTransPhrase(PhraseInfo* newPI, const Range &range, const char *word)
{
   VectorPhrase bi_phrase(1, BiLMAnnotation::biPhraseVocab->add((word + BiLMWriter::sep + word).c_str()));
   newPI->annotations.setAnnotation(type_id, new BiLMAnnotation(bi_phrase));
}

void BiLMAnnotation::Annotator::annotate_helper(
      VectorPhrase& bi_phrase, VocabFilter& biVoc,
      TScore* tscore, const PhraseTableEntry& entry)
{
   AlignmentAnnotation* a_ann = AlignmentAnnotation::get(tscore->annotations);
   const Uint tgt_size = entry.tgtPhrase.size();
   bi_phrase.resize(entry.tgtPhrase.size());
   if (a_ann) {
      // construct the bi_phrase from the phrase pair and its alignment
      vector<vector<Uint> > reversed_sets;

      AlignmentFreqs<float> alignment_freqs;
      parseAndTallyAlignments(alignment_freqs, a_ann->getAlignmentVoc(), a_ann->getAlignment());
      assert(!alignment_freqs.empty());
      const char* top_alignment_string = a_ann->getAlignmentVoc().word(alignment_freqs.max()->first);
      string reversed_al;
      GreenWriter::reverse_alignment(reversed_al, top_alignment_string,
            entry.src_word_count, tgt_size, '_');
      GreenReader('_').operator()(reversed_al, reversed_sets);

      assert(reversed_sets.size() >= tgt_size);
      string bi_word;
      for (Uint i = 0; i < tgt_size; ++i) {
         bi_word = tgtWord(entry.tgtPhrase[i]);
         if (reversed_sets[i].empty()) bi_word += BiLMWriter::sep;
         for (Uint j = 0; j < reversed_sets[i].size(); ++j) {
            bi_word += BiLMWriter::sep;
            if (reversed_sets[i][j] >= entry.src_word_count)
               bi_word += "NULL";
            else
               bi_word += srcWord(entry.src_tokens[reversed_sets[i][j]]);
         }
         bi_phrase[i] = biVoc.add(biToken(bi_word));
      }
   } else {
      // when the alignment information is missing, just assume all
      // target words are aligned to all source words - this is a fine
      // default for the most common case, the single-word phrase pair
      // that came from the TTable.
      string sep_src_words;
      for (Uint i = 0; i < entry.src_word_count; ++i)
         sep_src_words += BiLMWriter::sep + srcWord(entry.src_tokens[i]);
      for (Uint i = 0; i < entry.tgtPhrase.size(); ++i)
         bi_phrase[i] = biVoc.add(biToken(tgtWord(entry.tgtPhrase[i]) + sep_src_words));
   }
}

void BiLMAnnotation::Annotator::annotate(TScore* tscore, const PhraseTableEntry& entry)
{
   BiLMAnnotation* bilm_ann = BiLMAnnotation::get(tscore->annotations);
   if (bilm_ann) {
      static bool warning_printed = false;
      if (!warning_printed) {
         error(ETWarn, "When using -bilm-file with multiple phrase tables, the first phrase table where a phrase pair is encountered determines the alignment used to construct the bi-phrase.  Ignoring the second instance found.  (Printing this message only once.)");
         warning_printed = true;
      }
   } else {
      VectorPhrase bi_phrase;
      annotate_helper(bi_phrase, *BiLMAnnotation::biPhraseVocab, tscore, entry);
      tscore->annotations.setAnnotation(type_id, new BiLMAnnotation(bi_phrase));
      if (entry.limitPhrases)
         BiLMAnnotation::biPhraseVocab->addPerSentenceVocab(bi_phrase, &entry.tgtTable->input_sent_set);
   }

}



//===================================================================
// Coarse BiLM annotation/annotator
//===================================================================

static const bool debug_coarse_bilm = false;

const string CoarseBiLMAnnotation::name_prefix = "CoarseBiLM_";
char CoarseBiLMAnnotation::name_suffix = '0';

void CoarseBiLMAnnotation::display(ostream& out) const
{
   if (!bi_phrase.empty())
      out << "\tcoarse bi phrase      "
          << phrase2string(bi_phrase, annotator->coarseBiPhraseVocab)
          << endl;
}

const string& CoarseBiLMAnnotation::Annotator::class2string(Uint classID)
{
   static const string UNK_Symbol = PLM::UNK_Symbol;
   if (classID < class_str.size())
      return class_str[classID];
   else
      return UNK_Symbol;
}

CoarseBiLMAnnotation::Annotator::Annotator(const VocabFilter& tgtVocab, const vector<string>& clsFiles)
   : BiLMAnnotation::Annotator(tgtVocab)
   , name(name_prefix + name_suffix++)
   , type_id(AnnotationList::addAnnotationType(name.c_str()))
   , useSrcClasses(false), useTgtClasses(false), useTgtSrcClasses(false)
   , coarseBiPhraseVocab(tgtVocab.getNumSourceSents())
{
   if (debug_coarse_bilm) cerr << "CoarseBiLMAnnotation::Annotator::Annotator()" << endl;
   Uint max_num_classes = 0;

   // Load all the class files specified
   for (Uint i = 0; i < clsFiles.size(); ++i) {
      vector<string> keyvalue;
      split(clsFiles[i], keyvalue, "=", 2);
      assert(keyvalue.size() == 2);
      const string class_file = keyvalue[1];
      if (!check_if_exists(class_file))
         error(ETFatal, "Can't read class file " + class_file + " in coarse BiLM");
      const string key = keyvalue[0];
      if (key == "cls(src)") {
         useSrcClasses = true;
         srcClasses.read(class_file);
         max_num_classes = max(max_num_classes, srcClasses.getHighestClassId()+1);
      } else if (key == "cls(tgt)") {
         useTgtClasses = true;
         tgtClasses.read(class_file);
         max_num_classes = max(max_num_classes, tgtClasses.getHighestClassId()+1);
      } else if (key == "cls(tgt/src)") {
         useTgtSrcClasses = true;
         tgtSrcClasses.read(class_file);
         max_num_classes = max(max_num_classes, tgtSrcClasses.getHighestClassId()+1);
      } else {
         error(ETFatal, "Don't know how to interpret class file specification " + clsFiles[i] + " in coarse BiLM");
      }
   }

   // Init the class_str vector with a string for each possible class id over
   // any of the three possible class maps.
   class_str.reserve(max_num_classes);
   char buf[24];
   for (Uint i = 0; i < max_num_classes; ++i) {
      sprintf(buf, "%d", i);
      class_str.push_back(buf);
   }
}

void CoarseBiLMAnnotation::Annotator::addSourceSentences(const VectorPSrcSent& sents)
{
   if (debug_coarse_bilm) cerr << "CoarseBiLMAnnotation::Annotator::addSourceSentences()" << endl;
   static string UNK_Symbol = PLM::UNK_Symbol;
   for (Uint s(0); s<sents.size(); ++s) {
      const vector<string>& src_sent = sents[s]->src_sent;
      for (Uint i(0); i<src_sent.size(); ++i) {
         const char* word = src_sent[i].c_str();
         coarseBiPhraseVocab.addWord(biToken(tgtWord(word) + BiLMWriter::sep + srcWord(word)), s);
      }
      for (Uint i(0); i < sents[s]->marks.size(); ++i) {
         PhraseInfo bogus_pi;
         annotateMarkedPhrase(&bogus_pi, &(sents[s]->marks[i]), sents[s].get());
      }
   }
   if (debug_coarse_bilm) cerr << "biVoc " << &coarseBiPhraseVocab << " .size() = " << coarseBiPhraseVocab.size() << endl;
}

void CoarseBiLMAnnotation::Annotator::annotateMarkedPhrase(PhraseInfo* newPI, const MarkedTranslation* mark, const newSrcSentInfo* info)
{
   if (debug_coarse_bilm) cerr << "CoarseBiLMAnnotation::Annotator::annotateMarkedPhrase()" << endl;
   VectorPhrase bi_phrase;

   string sep_src_words;
   for (Uint i = mark->src_words.start; i < mark->src_words.end; ++i)
      sep_src_words += BiLMWriter::sep + srcWord(info->src_sent[i].c_str());
   for ( vector<string>::const_iterator jt = mark->markString.begin();
         jt != mark->markString.end(); jt++)
      bi_phrase.push_back(coarseBiPhraseVocab.add(biToken(tgtWord(jt->c_str()) + sep_src_words)));

   newPI->annotations.setAnnotation(type_id, new CoarseBiLMAnnotation(this, bi_phrase));
}

void CoarseBiLMAnnotation::Annotator::annotateNoTransPhrase(PhraseInfo* newPI, const Range &range, const char *word)
{
   if (debug_coarse_bilm) cerr << "CoarseBiLMAnnotation::Annotator::annotateNoTransPhrase()" << endl;
   VectorPhrase bi_phrase(1, coarseBiPhraseVocab.add(biToken(tgtWord(word) + BiLMWriter::sep + srcWord(word))));
   newPI->annotations.setAnnotation(type_id, new CoarseBiLMAnnotation(this, bi_phrase));
}

void CoarseBiLMAnnotation::Annotator::annotate(TScore* tscore, const PhraseTableEntry& entry)
{
   //if (debug_coarse_bilm) cerr << "CoarseBiLMAnnotation::Annotator::annotate()" << endl;
   CoarseBiLMAnnotation* bilm_ann = get(tscore->annotations);
   if (bilm_ann) {
      static bool warning_printed = false;
      if (!warning_printed) {
         error(ETWarn, "When using a coarse BiLM with multiple phrase tables, the first phrase table where a phrase pair is encountered determines the alignment used to construct the bi-phrase.  Ignoring the second instance found.  (Printing this message only once.)");
         warning_printed = true;
      }
   } else {
      VectorPhrase bi_phrase;
      annotate_helper(bi_phrase, coarseBiPhraseVocab, tscore, entry);
      tscore->annotations.setAnnotation(type_id, new CoarseBiLMAnnotation(this, bi_phrase));
      if (entry.limitPhrases)
         coarseBiPhraseVocab.addPerSentenceVocab(bi_phrase, &entry.tgtTable->input_sent_set);
   }
}

CoarseBiLMAnnotation* CoarseBiLMAnnotation::Annotator::get(const AnnotationList& list) {
   return static_cast<CoarseBiLMAnnotation*>(list.getAnnotation(type_id));
}
