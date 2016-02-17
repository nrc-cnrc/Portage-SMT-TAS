/**
 * @author Samuel Larkin
 * @file mapper.h
 * @brief String mappers.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include "mapper.h"

using namespace Portage;


namespace Portage {

WordClassesMapper::WordClassesMapper(const string& classesFile, const string& unknown, Voc *vocab)
: IWordClassesMapper(unknown)
{
   word_classes.read(classesFile, vocab, true);

   class_str.reserve(word_classes.getHighestClassId() + 1);

   char buf[24];
   for (Uint i = 0; i <= word_classes.getHighestClassId(); ++i) {
      sprintf(buf, "%d", i);
      class_str.push_back(buf);
   }
}

WordClassesMapper::~WordClassesMapper()
{}

const string& WordClassesMapper::operator()(const string& in) {
   return getClassString(word_classes.classOf(in.c_str()));
}



WordClassesMapper_MemoryMappedMap::WordClassesMapper_MemoryMappedMap(const string& classesFile, const string& unknown)
: IWordClassesMapper(unknown)
{
   className.reserve(100);
   map.open(classesFile);
}

WordClassesMapper_MemoryMappedMap::~WordClassesMapper_MemoryMappedMap()
{}

const string& WordClassesMapper_MemoryMappedMap::operator()(const string& in) {
   MMMap::const_iterator it = map.find(in.c_str());
   if (it == map.end())
      return unknown;

   className = it.getValue();
   return className;
}



IWordClassesMapper* loadClasses(const string& fname, const string& unknown, Voc* vocab) {
   string magicNumber;
   iSafeMagicStream is(fname);
   if (!getline(is, magicNumber))
      error(ETFatal, "Empty classfile %s", fname.c_str());

   IWordClassesMapper* mapper(NULL);
   if (magicNumber == MMMap::version2)
      mapper = new WordClassesMapper_MemoryMappedMap(fname, unknown);
   else
      mapper = new WordClassesMapper(fname, unknown, vocab);
   assert(mapper != NULL);

   return mapper;
}

};
