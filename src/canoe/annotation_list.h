/**
 * @author Eric Joanis
 * @file annotation_list.h
 *
 * This class is designed to hold a number of annotations of various types, associated
 * with each phrase pair, for use by various features.  The specifics of each
 * annotation are to be hidden in the client feature's implementation.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#ifndef _ANNOTATION_LIST_H_
#define _ANNOTATION_LIST_H_

#include "canoe_general.h"
#include "voc.h"

namespace Portage {

class TScore;
class PhraseTableEntry;
class PhraseInfo;
class newSrcSentInfo;
class VectorPSrcSent;
class MarkedTranslation;

class AnnotationList;

static bool debug_annotation_list = true;

/**
 * This is the base class for all annotation types.
 * Each subclass must:
 *  - Have a "static const string name" field initialized to its name; make
 *    sure your name is unique across all PhrasePairAnnotation types.
 *  - Implement clone().
 *  - Implement get(list) or getOrCreate(list, is_new) (Recommendation: Cut and
 *    paste the one-liner from TestSringAnnotation::get(list) (bottom of this
 *    file) or TestPrivateAnnotation::getOrCreate(list, is_new) (in
 *    tests/test_annotation_list.h), changing the class name, for a quick
 *    correct implementation.)
 *
 * When you want access to your annotation, get it by calling
 *    MyAnnotation* a = MyAnnotation::get(phrase_info.annotations);
 * or:
 *    bool is_new;
 *    MyAnnotation* a = MyAnnotation::getOrCreate(phrase_info.annotations, &is_new);
 *    if (*is_new)
 *       do whatever you need to do the first time the annotation is created
 *    else
 *       do whatever you need to do when the annotation already existed before
 *
 * Samples to look at to create your own annotations:
 *  - TestStringAnnotation (at the end of this file): this is the simplest form
 *    of an annotation loaded with the phrase table in a named field.
 *  - AlignmentAnnotation (in alignment_annotation.h): loaded from the phrase
 *    table, but with more flesh in the code, since this one is a real
 *    annotation.
 *
 *  - BiLMAnnotation (in bilm_annotation.h): this is the most complex
 *    annotation, because it needs initialization before the TM loads,
 *    calculation while the TM loads, and has an effect on how the BiLM model
 *    is loaded itself afterwards.  This is the example to look at if you need
 *    such complex behaviour for yours.
 *
 *  - TestPrivateAnnotation (in tests/test_annotation_list.h): this is the
 *    simplest form of a private annotation; it can live in your own class file
 *    and gives you complete encapsulation.  This is a good example to follow
 *    if you don't need initialization while the phrase table is loading.
 *  - SparsePill (in sparsemodel.cc): a real example of a private annotation.
 *
 * Other annotation types that exist:
 *  - UnknownAnnotation class (below): intended to handle named fields in the
 *    phrase table that are not known yet.  You should not follow this one's
 *    example.
 */
class PhrasePairAnnotation {
public:
   /// Get the type ID for a given annotation type, registering it in the AnnotationList registry if necessary
   static Uint getTypeID(const string& name);

   /// Factory method to create an annotation from its name and value
   static PhrasePairAnnotation* create(const char* key, const char* value);

   /**
    * Update the value of the annotation with the new value.  The semantics of
    * update can be defined by each annotation: update might tally, append, replace,
    * or whatever your annotation needs to do when it seems the same phrase pair
    * multiple times while loading phrase tables.
    * Only relevant for annotations that come from named fields in phrase tables.
    */
   virtual void updateValue(const char* value);

   /**
    * Display the value of an annotation, for debugging purposes (gets output
    * in canoe's -v 3 output, with each decoder state)
    * Should produce one line matching the following pattern:
    *    "\tdescription           human-readable-value\n"
    * i.e., a tab, a description padded with spaces to fit in exactly 22
    * characters, and the value of the annotation for human inspection.
    * This is completely optional!  Only worth implementing if you ever need
    * to look at -v 3 output from canoe.
    */
   virtual void display(ostream& out) const {};

   /**
    * Print the annotation to its representation in the text phrase table
    * format, by calling write_helper().
    *
    * Only override this method if your annotation exists as a field in phrase
    * tables.
    */
   virtual void write(ostream& phrase_table_file) const {};

   /**
    * Helper for write() that prints out the results in the correct format:
    *  - one space
    *  - the annotation name
    *  - "="
    *  - the annotation's text representation
    * E.g., AlignmentAnnotation would output something like " a=0_1".
    *
    * @param phrase_table_file  output file
    * @param name               the name of the annotation to write
    * @param value              the value of the annotation to write
    */
   static void write_helper(ostream& phrase_table_file, const string& name, const char* value);
   static void write_helper(ostream& phrase_table_file, const string& name, const string& value);

   /// Clone this annotation; should make a deep copy of your annotation
   virtual PhrasePairAnnotation* clone() const = 0;

   /// Virtual destructor required for polymorphic classes
   virtual ~PhrasePairAnnotation() {};

   /**
    * Get a annotation if it exists, or NULL otherwise
    * Suggestion: implement a get(list) method in your annotation class which class
    * which calls this method.
    * @param list    the annotation list from which to get an annotation of type AnnotationType.
    */
   template <class AnnotationType> static AnnotationType* get(const string& name, const AnnotationList& list);

   /**
    * If an annotation of AnnotationType does not exist, create it; return the annotation
    * Suggestion: implement a getOrCreate(list) method in your annotation class which
    * calls this method and does further appropriate initialization when
    * *is_new is true.
    * @param list    the annotation list in which to get or create an annotation of type AnnotationType.
    * @param is_new  if non-NULL, will be set to indicate whether the annotation existed before.
    */
   template <class AnnotationType> static AnnotationType* getOrCreate(const string& name, const AnnotationList& list, bool* is_new = NULL);

}; // class PhrasePairAnnotation

class PhrasePairAnnotator {
public:
   /// abstract class requires a virtual destructor
   virtual ~PhrasePairAnnotator() {}

   /// Return the name of the annotation this annotator creates
   virtual const string& getName() const = 0;

   /// If your annotator needs the source sentences somehow for its
   /// initialization, override this virtual method
   virtual void addSourceSentences(const VectorPSrcSent& sentences) {}

   /// If your annotator needs to do something with marked phrases, override
   /// this method too
   virtual void annotateMarkedPhrase(PhraseInfo* newPI, const MarkedTranslation* mark, const newSrcSentInfo* info) {}

   /// If your annotator needs to do something with pass-through translations,
   /// override this method
   virtual void annotateNoTransPhrase(PhraseInfo* newPI, const Range &range, const char *word) {}

   /**
    * Main factory function to create an annotation of your type, called when
    * loading a phrase pair from a text phrase table
    * Should create your annotation type and add it to tscore->annotations as appropriate
    * The default implementation will call the 4-argument annotate, so it's
    * enough to implement only that one.
    */
   virtual void annotate(TScore* tscore, const PhraseTableEntry& entry);

   /**
    * Factory function to create an annotation of your type, called when
    * loading a phrase pair from a TPPT
    * Should create your annotation type and add it to tscore->annotations as appropriate
    * @param tscore the TScore object has most phrase information, including
    *               previously initialized annotations such as count and alignment.
    * @param src_tokens array with the source phrase in const char* format for
    *                   each source word
    * @param src_word_count number of words in source phrase src_tokens
    * @param tgtPhrase target phrase in the standard vector of word ID format.
    */
   virtual void annotate(TScore* tscore, const char* const src_tokens[],
         Uint src_word_count, const VectorPhrase& tgtPhrase) = 0;
};

/**
 * The UnknownAnnotation is intended for name fields in the phrase table's
 * third column that are not of a known annotation type.
 * Not used yet, but they will be needed once we automatically initialize
 * annotations based on their presence in the phrase table.
 */
class UnknownAnnotation : public PhrasePairAnnotation {
   string name;
   string value;
public:
   UnknownAnnotation(const string& name, const char* value) : name(name), value(value) {}
   virtual void updateValue(const char* value);
   virtual PhrasePairAnnotation* clone() const { return new UnknownAnnotation(name, value.c_str()); }
   static UnknownAnnotation* get(const char* name, const AnnotationList& list);
   const string& getValue() const { return value; }
   virtual void display(ostream& out) const;
   virtual void write(ostream& phrase_table_file) const;
};

/**
 * Container for annotations - you should not usually use this class directly, only
 * via the various annotation types.
 */
class AnnotationList {
   /// The actual annotations list.  Mutable because we want features to
   /// received const PhraseInfo& and nonetheless be able to associate and
   /// modify their own annotation for that phrase pair.
   mutable vector<PhrasePairAnnotation*> annotations;
   static Voc registry;

public:
   PhrasePairAnnotation* getAnnotation(Uint type_id) const;

   /// setAnnotation() adds annotation of type type_id to *this.  Const for the
   /// same reason annotations is mutable.
   void setAnnotation(Uint type_id, PhrasePairAnnotation* annotation) const;

   /// Register a new annotation type, by its name.
   static Uint addAnnotationType(const char* name) {
      return registry.add(name);
   }

   /// Initialize an annotation from its key=value representation
   void initAnnotation(const char* key, const char* value);

   /// operator() is defined so that the AnnotationList can be used as the
   /// NameFieldHandler accepted by TMEntry::parseThird().
   bool operator()(const char* name, const char* value);

   /// Empty all the annotations in this list
   void clear();

   /// Display the contents of the list for human inspection
   void display(ostream& out) const;

   /// Write all the annotations in text phrase table format
   void write(ostream& phrase_table_file) const;

   /// Make a deep copy of another annotation list into *this, cloning each annotation
   AnnotationList& operator=(const AnnotationList& list);
   /// Copy constructor does a deep copy, cloning each annotation
   explicit AnnotationList(const AnnotationList& list);

   /// Default constructor has to be declared since another constructor was declared
   AnnotationList() {}
   /// Destructor
   ~AnnotationList() { clear(); }
}; // class AnnotationList

template <class AnnotationType> AnnotationType* PhrasePairAnnotation::get(const string& name, const AnnotationList& list)
{
   static const Uint type_id = getTypeID(name);
   PhrasePairAnnotation* annotation = list.getAnnotation(type_id);
   if (debug_annotation_list)
      assert(static_cast<AnnotationType*>(annotation) == dynamic_cast<AnnotationType*>(annotation));
   return static_cast<AnnotationType*>(annotation);
}

template <class AnnotationType> AnnotationType* PhrasePairAnnotation::getOrCreate(const string& name, const AnnotationList& list, bool* is_new)
{
   static const Uint type_id = getTypeID(name);
   PhrasePairAnnotation* existing_annotation = list.getAnnotation(type_id);
   if (existing_annotation) {
      if (is_new) *is_new = false;
      if (debug_annotation_list)
         assert(static_cast<AnnotationType*>(existing_annotation) == dynamic_cast<AnnotationType*>(existing_annotation));
      return static_cast<AnnotationType*>(existing_annotation);
   } else {
      if (is_new) *is_new = true;
      AnnotationType* new_annotation = new AnnotationType;
      list.setAnnotation(type_id, new_annotation);
      return new_annotation;
   }
}

/**
 * TestStringAnnotation class is used for unit testing - standard annotation init from string
 * It would be nicer to hide this in the test suite itself, but we need it
 * here to be able to support this annotation type in PhrasePairAnnotation::create() and
 * AnnotationList::initAnnotation().
 * This class illustrates how to create your own string based annotation, for fields
 * stored in the phrase table, when you don't want to just use the generic
 * StringAnnotation.
 */
class TestStringAnnotation : public PhrasePairAnnotation {
   string value;
public:
   static const string name;
   TestStringAnnotation(const char* s) : value(s) {}
   virtual void updateValue(const char* _value) { value += ";"; value += _value; }
   virtual PhrasePairAnnotation* clone() const { return new TestStringAnnotation(value.c_str()); }
   const string& getValue() const { return value; }
   static TestStringAnnotation* get(const AnnotationList& list) { return PhrasePairAnnotation::get<TestStringAnnotation>(name, list); }
};

/// This is the generic holder for annotations
} // namespace Portage

#endif
