/**
 * @author Eric Joanis
 * @file annotation_list.cc
 *
 * Implementation for AnnotationList and friends.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "annotation_list.h"
#include "alignment_annotation.h"
#include "count_annotation.h"
#include "phrasetable.h"


Voc AnnotationList::registry;

// PhrasePairAnnotation ===============================================

Uint PhrasePairAnnotation::getTypeID(const string& name)
{
   return AnnotationList::addAnnotationType(name.c_str());
}

PhrasePairAnnotation* PhrasePairAnnotation::create(const char* key, const char* value)
{
   if (key == AlignmentAnnotation::name)
      return new AlignmentAnnotation(value);
   else if (key == CountAnnotation::name)
      return new CountAnnotation(value);
   else if (key == TestStringAnnotation::name)
      return new TestStringAnnotation(value);
   else
      return new UnknownAnnotation(key, value);
}

void PhrasePairAnnotation::updateValue(const char* value)
{
   // not required for synthetic annotators, so we don't make it pure virtual,
   // but then we don't allow letting this method be called, either.
   assert(false && "your annotator needs to implement updateValue()");
}

void PhrasePairAnnotation::write_helper(ostream& phrase_table_file, const string& name, const char* value)
{
   phrase_table_file << " " << name << "=" << value;
}

void PhrasePairAnnotation::write_helper(ostream& phrase_table_file, const string& name, const string& value)
{
   phrase_table_file << " " << name << "=" << value;
}

// PhrasePairAnnotator ========================================

void PhrasePairAnnotator::annotate(TScore* tscore, const PhraseTableEntry& entry) {
   annotate(tscore, &(entry.src_tokens[0]), entry.src_word_count, entry.tgtPhrase);
}

// UnknownAnnotation ========================================

void UnknownAnnotation::updateValue(const char* _value) {
   static bool warning_printed = false;
   if (!warning_printed) {
      error(ETWarn, "Unknown annotation type %s found in multiple phrase tables; combination semantics are not defined, keeping the last value seen.  Printing this message only once for all occurrences and all unknown annotation types.", name.c_str());
      warning_printed = true;
   }

   value = _value;
}

UnknownAnnotation* UnknownAnnotation::get(const char* name, const AnnotationList& list)
{
   // name is not static, so we have to call getTypeID() each time.
   const Uint type_id = getTypeID(name);
   PhrasePairAnnotation* ann = list.getAnnotation(type_id);
   if (ann) {
      UnknownAnnotation* u_ann = dynamic_cast<UnknownAnnotation*>(ann);
      if (!u_ann)
         error(ETFatal, "Trying to access annotation of type %s via UnknownAnnotation when it is not actually unknown.  "
               "Use its proper Annotation class to access it instead!", name);
      return u_ann;
   } else {
      return NULL;
   }
}

void UnknownAnnotation::display(ostream& out) const
{
   out << "\tUnknown annotation    " << name << "=" << value << endl;
}

void UnknownAnnotation::write(ostream& phrase_table_file) const
{
   write_helper(phrase_table_file, name, value);
}

// AnnotationList ===============================================

void AnnotationList::initAnnotation(const char* key, const char* value)
{
   Uint type_id = registry.add(key);
   if (annotations.size() <= type_id)
      annotations.resize(registry.size(), NULL);
   if (annotations[type_id])
      annotations[type_id]->updateValue(value);
   else
      annotations[type_id] = PhrasePairAnnotation::create(key, value);
}

bool AnnotationList::operator()(const char* name, const char* value)
{
   if (*name == '\0' || *value == '\0') {
      error(ETWarn, "Empty name or value is now allowed in phrase table named fields.");
      return false;
   } else {
      initAnnotation(name, value);
      return true;
   }
}

PhrasePairAnnotation* AnnotationList::getAnnotation(Uint type_id) const
{
   if (annotations.size() > type_id)
      return annotations[type_id];
   else
      return NULL;
}

void AnnotationList::setAnnotation(Uint type_id, PhrasePairAnnotation* annotation) const
{
   if (annotations.size() <= type_id)
      annotations.resize(registry.size(), NULL);
   if (annotations[type_id])
      delete annotations[type_id];
   annotations[type_id] = annotation;
}

void AnnotationList::clear()
{
   for (Uint i = 0; i < annotations.size(); ++i) {
      delete annotations[i];
      annotations[i] = NULL;
   }
}

void AnnotationList::display(ostream& out) const
{
   for (Uint i = 0; i < annotations.size(); ++i)
      if (annotations[i])
         annotations[i]->display(out);
}

void AnnotationList::write(ostream& phrase_table_file) const
{
   for (Uint i = 0; i < annotations.size(); ++i)
      if (annotations[i])
         annotations[i]->write(phrase_table_file);
}

AnnotationList& AnnotationList::operator=(const AnnotationList& list)
{
   if (!annotations.empty()) clear();
   const Uint b_size = list.annotations.size();
   if (annotations.size() < b_size) {
      assert(b_size <= registry.size());
      annotations.resize(registry.size(), NULL);
   }
   for (Uint i = 0; i < b_size; ++i) {
      if (list.annotations[i])
         annotations[i] = list.annotations[i]->clone();
   }
   return *this;
}

AnnotationList::AnnotationList(const AnnotationList& list)
{
   *this = list;
}

// TestStringAnnotation ===============================================

const string TestStringAnnotation::name = "stringannotation";
