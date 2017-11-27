#include "wordClassMapper.h"
#include "file_utils.h"

using namespace Portage;


bool IWordClassesMapper::isMemoryMapped(const string& fname) {
   string magicNumber;
   iSafeMagicStream is(fname);
   if (!getline(is, magicNumber))
      error(ETFatal, "Empty classfile %s", fname.c_str());

   return magicNumber == MMMap::version2;
}


IWordClassesMapper* Portage::getWord2ClassesMapper(const string& fname, const string& unknown) {
   IWordClassesMapper* mapper(NULL);
   if (IWordClassesMapper::isMemoryMapped(fname)) {
      mapper = new WordClassesMemoryMappedMapper(fname, unknown);
   }
   else {
      mapper = new WordClassesTextMapper(fname, unknown);
   }
   assert(mapper != NULL);

   return mapper;
}


WordClassesTextMapper::WordClassesTextMapper(const string& fname, const string& unknown)
: IWordClassesMapper(unknown)
{
   //cerr << "Loading text classes " << fname << endl;
   iSafeMagicStream is(fname);
   string line;
   const char* sep = " \t";
   while (getline(is, line)) {
      const Uint len = strlen(line.c_str());
      char work[len+1];
      strcpy(work, line.c_str());
      assert(work[len] == '\0');

      char* strtok_state;
      const char* word = strtok_r(work, sep, &strtok_state);
      if (word == NULL)
         error(ETFatal, "expected 'word\ttag' entries in <%s>", fname.c_str());
      const char* tag = strtok_r(NULL, sep, &strtok_state);
      if (tag == NULL)
         error(ETFatal, "expected 'word\ttag' entries in <%s>", fname.c_str());

      word2class[word] = tag;
   }
}


const string& WordClassesTextMapper::operator()(const string& word) const {
   Word2class::const_iterator it = word2class.find(word);
   if (it == word2class.end()) return unknown;
   return it->second;
}



WordClassesMemoryMappedMapper::WordClassesMemoryMappedMapper(const string& fname, const string& unknown)
: IWordClassesMapper(unknown)
, word2class(fname)
{
}


const string& WordClassesMemoryMappedMapper::operator()(const string& word) const {
   MMMap::const_iterator it = word2class.find(word.c_str());
   if (it == word2class.end())
      return unknown;

   className = it.getValue();
   return className;
}


