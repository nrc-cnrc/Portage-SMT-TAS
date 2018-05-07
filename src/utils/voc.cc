/**
 * @author George Foster
 * @file voc.cc  Vocabulary associates an integer to every word.
 *
 *
 * COMMENTS:
 *
 * Map strings <-> unique indexes
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "voc.h"
#include <errors.h>
#include <iostream>
#include <iterator>

using namespace Portage;

void Voc::deleteWords()
{
   for (Uint i = 0; i < words.size(); ++i)
      delete[] words[i];
}

Voc::Voc() {
}

Voc::~Voc() {
   deleteWords();
}

void Voc::read(const string& filename) {
   iSafeMagicStream istr(filename.c_str());
   read(istr);
}

void Voc::read(istream& istr) {
   string line;
   while (getline(istr, line)) {add(line.c_str());}
}

void Voc::write(const string& filename, const char* delim) const {
   oSafeMagicStream ostr(filename);
   write(ostr, delim);
}

void Voc::write(ostream& os, const char* delim) const {
   ostream_iterator<const char *> outStr(os, delim);
   copy(words.begin(), words.end(), outStr);
}

static const char magic_string[] = "Portage Voc stream v1.0";

void Voc::writeStream(ostream& os) const {
   os << magic_string << nf_endl;
   os << size() << nf_endl;
   write(os);
   os << "End of " << magic_string << endl;
}

void Voc::readStream(istream& is, const char* stream_name) {
   clear();
   string line;

   // header
   if ( !getline(is, line) )
      error(ETFatal, "No input in stream %s; expected Voc magic line",
            stream_name);
   if ( line != magic_string )
      error(ETFatal, "Magic line does not match Voc magic line in %s: %s",
            stream_name, line.c_str());
   Uint count = 0;
   is >> count;
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file right after Voc magic line in %s",
            stream_name);
   if ( line != "" ) 
      error(ETFatal, "Unexpected content right after Voc magic line in %s: %s",
            stream_name, line.c_str());

   // Vocabulary itself
   Uint line_no(1);
   while ( count > 0 ) {
      ++line_no;
      if ( !getline(is, line) )
         error(ETFatal, "Unexpected end of file in %s, %u lines after "
               "Voc magic line; expected %u more words",
               stream_name, line_no, count);
      if ( line == "" )
         error(ETFatal, "Unexpected blank line in %s, %u lines after "
               "Voc magic line", stream_name, line_no);
      if ( add(line.c_str()) != line_no-2 )
         error(ETFatal, "Unexpected duplicate line (%s) in %s, %u lines after "
               "Voc magic line; got index %d, expected %d",
               line.c_str(), stream_name, line_no,
               index(line.c_str()), (line_no-2));
      --count;
   }

   // footer
   if ( !getline(is, line) )
      error(ETFatal, "Missing footer in Voc stream %s, %u lines after magic line",
            stream_name, line_no);
   if ( line != string("End of ") + magic_string )
      error(ETFatal, "Bad footer in Voc stream %s, %u lines after magic line: %s",
            stream_name, line_no, line.c_str());

}

Uint Voc::add(const char* word) {
   // Optimization: don't strdup unless we actually need to add word.  The
   // extra call to find means adding a new word is somewhat slower, though not
   // significantly.  But re-adding an existing word is much faster this way.
   // We often re-add the same words many times, so this implementation choice
   // makes the Voc class fastest overall.
   MapIter p = map.find(word);
   if ( p == map.end() ) {
      const char* w = strdup_new(word);
      pair<MapIter,bool> res = map.insert(make_pair(w, size()));
      assert(res.second);
      words.push_back(w);
      return res.first->second;
   } else {
      return p->second;
   }
}

void Voc::index(const vector<string>& src, vector<Uint>& dest) const
{
   // Operates on the basis of append mode
   dest.reserve(dest.size() + src.size());

   //transform(src.begin(), src.end(), back_insert_iterator< vector<Uint> >(dest), indexConverter(*this));
   //return;
   typedef vector<string>::const_iterator SRC_IT;
   for (SRC_IT it(src.begin()); it!=src.end(); ++it)
      dest.push_back(index(it->c_str()));
}

void Voc::clear() {
   deleteWords();
   words.clear();
   map.clear();
}

void Voc::swap(Voc& that) {
   words.swap(that.words);
   map.swap(that.map);
}

Voc::Voc(const Voc& that) {
   *this = that;
}

Voc& Voc::operator=(const Voc& that) {
   clear();
   words.resize(that.words.size(), NULL);
   for ( Uint i(0); i < words.size(); ++i ) {
      words[i] = strdup_new(that.words[i]);
      pair<MapIter,bool> res = map.insert(make_pair(words[i], i));
      FOR_ASSERT(res);
      assert(res.second);
   }
   return *this;
}

bool Voc::remap(Uint index, const char* newToken) {
   // Obviously an invalid index.
   if (index >= words.size()) return false;

   // If this new token is already part of the vocabulary and hasn't been
   // itself remapped, refuse to do the remapping.
   MapIter it = map.find(newToken);
   if (it != map.end() && 0 == strcmp(words[it->second], newToken) )
      return false;

   /*
   // Get rid of the old token
   map.erase(words[index]);
   delete[] words[index];

   // new token
   const char* w = strdup_new(newToken);
   pair<MapIter, bool> res = map.insert(make_pair(w, index));
   assert(res.second);
   words[index] = w;
   */

   // EJJ - we only change the output string, not the search string: this
   // behaviour may be weird, but gen_phrase_tables, the program for which
   // remap() exists, has to alternate between recognizing the old string
   // when converting to an index() and getting the new string when converting
   // back from an index.
   //delete [] words[index]; //memory leak: we still need the old string, for map, so we can't delete it now!
   words[index] = strdup_new(newToken);

   return true;
}

bool Voc::remap(const char* oldToken, const char* newToken) {
   // This old token is not part of the vocabulary.
   MapIter it = map.find(oldToken);
   if (it == map.end()) return false;

   return remap(it->second, newToken);
}

bool Voc::test() {

   Voc v;
   bool ok = true;

   const char* testwords[] = {
      "this", "is", "a", "somewhat", "redundant", "test", "of", "one", "small",
      "class", "not", "entirely", "superfluous", "itself", "however"
   };

   for (Uint iter = 0; ok && iter < 10; ++iter) {

      for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i) {
         if (v.add(testwords[i]) != v.size()-1) {
            error(ETWarn, "init add test failed");
            ok = false;
            break;
         }
      }
      if (v.size() != ARRAY_SIZE(testwords)) {
         error(ETWarn, "init size check failed");
         ok = false;
      }
      for (Uint i = 0; i < v.size(); ++i) {
         if (strcmp(v.word(i),testwords[i]) != 0) {
            error(ETWarn, "index contents check failed");
            ok = false;
         }
      }
      for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i) {
         if (v.index(testwords[i]) != i) {
            error(ETWarn, "find test failed");
            ok = false;
            break;
         }
      }
      for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i) {
         if (v.add(testwords[i]) != i) {
            error(ETWarn, "redundancy test failed");
            ok = false;
            break;
         }
      }
      v.write("-", ", "); cout << endl;
      v.clear();
   }

   return ok;
}

bool Portage::testCountingVoc()
{
   CountingVoc v;
   bool ok = true;

   const char* testwords[] = {
      "what", "is", "the", "price", "of", "experience", "?",
      "do", "men", "buy", "it", "for", "a", "song", ",", "or", "wisdom",
      "for", "a", "dance", "in", "the", "street", "?",
      "no", ",", "it", "is", "bought", "with", "the", "cost", "of", "all",
      "that", "man", "has", ",", "his", "house", ",", "his", "wife", ",",
      "his", "children"
   };

   for (Uint i = 0; i < ARRAY_SIZE(testwords); ++i)
      v.add(testwords[i]);

   Uint total = 0;
   for (Uint i = 0; i < v.size(); ++i) {
      total += v.freq(i);
      // cout << v.word(i) << " " << v.freq(i) << endl;
   }

   ok = (total == ARRAY_SIZE(testwords));

   return ok;
}

