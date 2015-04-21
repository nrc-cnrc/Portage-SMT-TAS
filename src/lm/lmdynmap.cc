/**
 * @author George Foster
 * @file lmdynmap.cc   Wrap a language model whose vocabulary is a many-to-one
 * mapping from words in the global vocabulary.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "lmdynmap.h"

using namespace Portage;

const char* LMDynMap::header = "DynMap;";
const char LMDynMap::separator = ';';


// If you add new mappings, please describe them here and in the constructor doc.
static char help_the_poor_user[] = "\
Syntax for LMDynMap: DynMap;MAP;LMDESC, where:\n\
<MAP> specifies the vocabulary mapping, one of:\n\
   - ident Map chars to themselves (for debugging)\n\
   - lower[-LOC] Map characters to lowercase using a CaseMapStrings\n\
     object created with locale <LOC>, eg lower-fr_CA.iso88591. If no <LOC> is\n\
     supplied, lower-en_US.UTF-8 is assumed.\n\
   - simpleNumber Substitute @ for digits in words that only contains digits\n\
     and punctuation.\n\
   - prefixNumber Substitute @ for digits which are prefix of a word.\n\
   - wordClasses-CLASSES Map a word to a corresponding word class;\n\
     <CLASSES> names a text file mapping word to class # as from mkcls.\n\
<LMDESC> is a normal LM filename.\n\
\n\
Examples:\n\
   DynMap;ident;file.lm.gz\n\
   DynMap;lower;file.lm.gz\n\
   DynMap;lower-fr_CA.iso88591;file.lm.gz\n\
   DynMap;simpleNumber;file.lm.gz\n\
   DynMap;prefixNumber;file.lm.gz\n\
   DynMap;wordClasses-en.100.classes;file.lm.gz\n\
";

LMDynMap::Creator::Creator(
      const string& lm_physical_filename, Uint naming_limit_order)
   : PLM::Creator(lm_physical_filename.substr(strlen(header)),
                  naming_limit_order)
{
   assert(lm_physical_filename.substr(0,strlen(header)) == header);
}

bool LMDynMap::Creator::checkFileExists(vector<string>* list)
{
   // Removing the map type
   const string::size_type p = lm_physical_filename.find(separator);
   if (p == string::npos || p+1 == lm_physical_filename.size())
      error(ETFatal, "LMDynMap name not in the form: maptype;name\n%s", 
               help_the_poor_user);
   const string embedded_lm = lm_physical_filename.substr(p+1);
   const string map_type = lm_physical_filename.substr(0, p);

   bool ok = true;

   if (isPrefix("wordClasses", map_type)) {      // word classes mapping
      const Uint keylen = strlen("wordClasses");
      string classesFile;
      if (map_type.size() > keylen + 1) {
         classesFile = map_type.substr(keylen+1);
         if (list) list->push_back(classesFile);
         if (!check_if_exists(classesFile)) {
            ok = false;
            error(ETWarn, "Can't access class file %s in DynMap LM %s",
                  classesFile.c_str(), lm_physical_filename.c_str());
         }
      }
   }

   return PLM::checkFileExists(embedded_lm, list) && ok;
}

Uint64 LMDynMap::Creator::totalMemmapSize()
{
   const string::size_type p = lm_physical_filename.find(separator);
   if (p == string::npos || p+1 == lm_physical_filename.size())
      error(ETFatal, "LMDynMap name not in the form: maptype;name\n%s", 
            help_the_poor_user);
   const string embedded_lm = lm_physical_filename.substr(p+1);
   return PLM::totalMemmapSize(embedded_lm);
}

PLM* LMDynMap::Creator::Create(VocabFilter* vocab,
                            OOVHandling oov_handling,
                            float oov_unigram_prob,
                            bool limit_vocab,
                            Uint limit_order,
                            ostream *const os_filtered,
                            bool quiet)
{
   return new LMDynMap(lm_physical_filename, vocab, oov_handling,
                       oov_unigram_prob, limit_vocab, limit_order,
                       os_filtered);
}

LMDynMap::LMDynMap(const string& name, VocabFilter* vocab,
                   OOVHandling oov_handling, float oov_unigram_prob,
                   bool limit_vocab, Uint limit_order,
                   ostream *const os_filtered) :
   PLM(vocab, oov_handling, oov_unigram_prob),
   limit_vocab(limit_vocab),
   m(NULL),
   local_voc(NULL),
   mapping(NULL)
{
   time_t start = time(NULL);
   const string::size_type p = name.find(separator);
   if (p == string::npos || p+1 == name.size())
      error(ETFatal, "LMDynMap name not in the form: maptype;name\n%s", 
            help_the_poor_user);

   // Initialize specified mapping & construct local vocab 

   const string map_type = name.substr(0, p);
   const string nm = name.substr(p+1);
   if (map_type == "ident") {
      mapping = new IdentMap();
   } else if (isPrefix("lower", map_type)) {    // lowercase mapping
      string loc("en_US.UTF-8");
      const Uint keylen = strlen("lower");
      if (map_type.size() > keylen + 1) 
         loc = map_type.substr(keylen+1);
      else if (map_type.size() != keylen)
         error(ETFatal, "syntax error in LMDynMap map type spec: " + map_type);
      mapping = new LowerCaseMap(loc.c_str());
   } else if (isSuffix("Number", map_type.c_str())) {
      mapping = new Number(map_type);
   } else if (isPrefix("wordClasses", map_type)) {      // word classes mapping
      const Uint keylen = strlen("wordClasses");
      string classesFile;
      if (map_type.size() > keylen + 1)
         classesFile = map_type.substr(keylen+1);
      else
         error(ETFatal, "syntax error in LMDynMap map type spec: " + map_type);
      mapping = new WordClassesMap(classesFile, limit_vocab ? vocab : NULL);
   } else
      error(ETFatal, "unknown LMDynMap map type: %s\n%s",
            map_type.c_str(), help_the_poor_user);
   assert(mapping);

   local_voc = new VocabFilter(*vocab, *mapping, index_map);
   cerr << "LMDynMap: Mapping initialization took " << (time(NULL) - start) << "s." << endl;

   // Construct the wrapped model

   m = PLM::Create(nm, local_voc, oov_handling, oov_unigram_prob,
                   limit_vocab, limit_order, os_filtered);

   // Clean up the voc

   local_voc->freePerSentenceData();
}

LMDynMap::~LMDynMap()
{
   if (local_voc != NULL)
      delete local_voc;
   if (m != NULL)
      delete m;
   if (mapping != NULL)
      delete mapping;
}

void LMDynMap::resyncLocalVoc()
{
   const Uint b = index_map.size();
   if (b < vocab->size()) index_map.resize(vocab->size());
   string r,s;
   for (Uint i = b; i < vocab->size(); ++i) {
      r = vocab->word(i);
      s = (*mapping)(r);
      index_map[i] = local_voc->add(s.c_str());
   }
}

float LMDynMap::wordProb(Uint word, const Uint context[], Uint context_length)
{
   Uint local_context[context_length];
   for (Uint i = 0; i < context_length; ++i)
      local_context[i] = localIndex(context[i]);
   return m->wordProb(localIndex(word), local_context, context_length);
}

float LMDynMap::cachedWordProb(Uint word, const Uint context[],
                               Uint context_length)
{
   // We override cachedWordProb in this class so that we can cache queries in
   // the mapped space, which should mean a smaller cache.
   Uint local_context[context_length];
   for (Uint i = 0; i < context_length; ++i)
      local_context[i] = localIndex(context[i]);
   return m->cachedWordProb(localIndex(word), local_context, context_length);
}

Uint LMDynMap::minContextSize(const Uint context[], Uint context_length)
{
   Uint local_context[context_length];
   for (Uint i = 0; i < context_length; ++i)
      local_context[i] = localIndex(context[i]);
   return m->minContextSize(local_context, context_length);
}

void LMDynMap::newSrcSent(const vector<string>& src_sent,
                          Uint external_src_sent_id)
{
   m->newSrcSent(src_sent, external_src_sent_id);
}

string LMDynMap::fix_relative_path(const string& path, string file)
{
   assert(isPrefix(header, file));
   
   // check for map type
   const string::size_type p = file.find(LMDynMap::separator, strlen(header));
   if (p == string::npos || p+1 == file.size())
      error(ETFatal, "LMDynMap name not in the form: maptype;name\n%s", 
               help_the_poor_user);

   if (!file.empty() && file[p+1] != '/')
      file.insert(p+1, path + "/");

   // If we have a word-class map, apply the relative path modification to that too
   if (isPrefix("wordClasses", file.substr(strlen(header)))) {
      Uint classFilePos = strlen(header)+strlen("wordClasses")+1;
      assert(classFilePos < file.size());
      if (file[classFilePos] != '/')
         file.insert(classFilePos, path + "/");
   }

   return file;
}

const string LMDynMap::WordClassesMap::UNK_Symbol(PLM::UNK_Symbol);
const string LMDynMap::WordClassesMap::SentStart(PLM::SentStart);
const string LMDynMap::WordClassesMap::SentEnd(PLM::SentEnd);

LMDynMap::WordClassesMap::WordClassesMap(const string& classesFile, Voc *vocab)
: classesFile(classesFile)
{
   word_classes.read(classesFile, vocab, true);
   class_str.reserve(word_classes.getHighestClassId()+1);
   char buf[24];
   for (Uint i = 0; i <= word_classes.getHighestClassId(); ++i) {
      sprintf(buf, "%d", i);
      class_str.push_back(buf);
   }
}

const string& LMDynMap::WordClassesMap::operator()(string& in) {
   if (in == SentStart || in == SentEnd || in == UNK_Symbol)
      return in;
   return getClassString(word_classes.classOf(in.c_str()));
}

