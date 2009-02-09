/**
 * $Id$
 * @author Eric Joanis
 * @file word_classes.cc Implementation of class to store word classes
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "portage_defs.h"
#include "word_classes.h"
#include "errors.h"
#include "str_utils.h"
#include "file_utils.h"

using namespace Portage;

const Uint WordClasses::NoClass = Uint(-1);

void WordClasses::add_helper(const char* word, Uint class_id) {
   char* w = block_storage.alloc_array_no_ctor(strlen(word)+1);
   strcpy(w, word);
   //error(ETWarn, "w=%p", w);

   pair<ClassMap::const_iterator, bool> res =
      class_map.insert(make_pair(w, class_id));
   assert(res.second);

   if ( class_id > highest_class_id )
      highest_class_id = class_id;
}

WordClasses::WordClasses(const WordClasses& wc)
   : highest_class_id(0)
{
   for ( ClassMap::const_iterator it(wc.class_map.begin()),
                                  end(wc.class_map.end());
         it != end; ++it )
      add_helper(it->first, it->second);
}

void WordClasses::clear() {
   class_map.clear();
   block_storage.clear();
}

Uint WordClasses::size() const {
   return class_map.size();
}

bool WordClasses::add(const char* word, Uint class_id) {
   ClassMap::const_iterator p = class_map.find(word);
   if ( p == class_map.end() ) {
      add_helper(word, class_id);
   } else if ( p->second != class_id ) {
      return false;
   }
   return true;
}

void WordClasses::read(const string& class_file) {
   iSafeMagicStream in(class_file);
   string line;
   vector<string> toks;
   Uint line_no(0);
   while ( getline(in, line) ) {
      ++line_no;
      splitZ(line, toks, "\t");
      if ( toks.size() != 2 )
         error(ETFatal, "Error in class file %s line %d: "
               "expected exactly one tab character, found %d",
               class_file.c_str(), line_no, toks.size()-1);
      Uint class_id;
      if ( !conv(toks[1], class_id ) )
         error(ETFatal, "Error in class file %d line %d: "
               "%s can't be converted to a number",
               class_file.c_str(), line_no, toks[1].c_str());
      if ( ! add(toks[0].c_str(), class_id) )
         error(ETFatal, "Error in class file %s line %d: "
               "%s occurs twice with different class ids.", 
               class_file.c_str(), line_no, toks[0].c_str());
   }
} // read()

static const char magic_string[] = "Portage WordClasses stream v1.0";

void WordClasses::writeStream(ostream& os) const {
   os << magic_string << endl;
   os << class_map.size() << endl;
   for ( ClassMap::const_iterator it(class_map.begin()), end(class_map.end());
         it != end; ++it )
      os << it->first << "\t" << it->second << nf_endl;
   os << "End of " << magic_string << endl;
}

void WordClasses::readStream(istream& is, const char* stream_name) {
   clear();
   string line;

   // header
   if ( !getline(is, line) )
      error(ETFatal, "No input in stream %s; expected WordClasses magic line",
            stream_name);
   if ( line != magic_string )
      error(ETFatal, "Magic line does not match WordClasses magic line in %s: %s",
            stream_name, line.c_str());
   Uint count = 0;
   is >> count;
   if ( !getline(is, line) )
      error(ETFatal, "Unexpected end of file right after WordClasses magic line in %s",
            stream_name);
   if ( line != "" ) 
      error(ETFatal, "Unexpected content right after WordClasses magic line in %s: %s",
            stream_name, line.c_str());

   // word classes themselves
   vector<string> toks;
   Uint line_no(1);
   while ( count > 0 ) {
      ++line_no;
      if ( !getline(is, line) )
         error(ETFatal, "Unexpected end of file in %s, %u lines after "
               "WordClasses magic line; expected %u more words",
               stream_name, line_no, count);
      splitZ(line, toks, "\t");
      if ( toks.size() != 2 )
         error(ETFatal, "Error in WordClasses stream %s, %u lines after magic line: "
               "expected exactly one tab character, found %d",
               stream_name, line_no, toks.size()-1);
      Uint class_id;
      if ( !conv(toks[1], class_id ) )
         error(ETFatal, "Error in WordClasses stream %s, %u lines after magic line: "
               "%s can't be converted to a number",
               stream_name, line_no, toks[1].c_str());
      if ( ! add(toks[0].c_str(), class_id) )
         error(ETFatal, "Error in WordClasses stream %s, %u lines after magic line: "
               "%s occurs twice with different class ids.", 
               stream_name, line_no, toks[0].c_str());
      --count;
   }

   // footer
   if ( !getline(is, line) )
      error(ETFatal, "Missing footer in WordClasses stream %s, %u lines after magic line",
            stream_name, line_no);
   if ( line != string("End of ") + magic_string )
      error(ETFatal, "Bad footer in WordClasses stream %s, %u lines after magic line: %s",
            stream_name, line_no, line.c_str());
}


