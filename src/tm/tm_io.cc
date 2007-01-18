/**
 * @author George Foster
 * @file tm_io.cc  Implementation of tm_io utilities.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <str_utils.h>
#include "tm_io.h"


// Required so by doxygen
namespace Portage {
namespace TMIO {

const char* escapes = "";

string getTag(const string& markup)
{
   Uint i;
   for (i = 0; i < markup.size(); ++i)
      if (markup[i] == ' ' || markup[i] == '\r' || markup[i] == '\t' || markup[i] == '\n')
	 break;
   return markup.substr(0, i);
}

bool findEnd(const string& tag, const string& line, size_t& pos)
{
   size_t orig_pos = pos;
   string tok;
   bool was_quoted;
   do {
      pos = getNextTok(line, pos, tok, " \r\t\n", TMIO::escapes, "<", ">", "", &was_quoted, true);
      if (pos == string::npos) break;
      if (was_quoted) {
	 string etag = getTag(tok);
	 if (etag.size() && tag == etag.substr(1))
	    return true;
      }
   } while (pos < line.size());
   pos = orig_pos;
   return false;
}

vector<string>& getTokens(const string& line, vector<string>& tokens, Uint do_markup)
{
   if (do_markup == 1) {
      split(line, tokens);
      return tokens;
   }
   
   string tok;
   size_t pos = 0;
   bool was_quoted;

   do {
      pos = getNextTok(line, pos, tok, " \r\t\n", escapes, "<", ">", "", &was_quoted, true);

      if (pos == string::npos) break;
      if (was_quoted) {
	 if (do_markup == 0) {	// output tag; otherwise, delete markup
	    string tag = getTag(tok);
	    if (findEnd(tag, line, pos))
	       tokens.push_back("<" + tag + ">");
	 }
      } else
	 tokens.push_back(tok);
   
   } while (pos < line.size());

   return tokens;
}

string& joinTokens(const vector<string>& tokens, string& line, bool bracket)
{
   for (vector<string>::const_iterator p = tokens.begin(); p != tokens.end(); ++p) {
      if (bracket) line += "[";
      line += *p;
      if (bracket) line += "]";
      if (p != tokens.end()-1)
	 line += " ";
   }
   return line;
}

void test() {
   string lout;
   string in = "\
   so\\me tokenized<noun \"quoted nouninfo\">text</noun> ,	\
   in a good <withtrashtoken> format , with \\<escaped markup\\> \
   and multi\\ word\\ tokens\
   ";
   cerr << "[" << in << "]" << endl;
   string ref;
   if (escapes == "\\")
      ref = "[some] [tokenized] [<noun>] [,] [in] [a] [good] [format] [,] [with] [<escaped] [markup>] [and] [multi word tokens]";
   else
      ref = "";			// need to work this out - no time now! GF

   vector<string> tokens;
   string out = joinTokens(getTokens(in, tokens), lout, true);
   
   cerr << out << endl;
   if (escapes == "\\")
      cerr << (ref == out ? "looks ok" : "problem") << endl;
}

} // ends namespace TMIO
} // ends namespace Portage
