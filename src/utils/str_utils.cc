/**
 * @author George Foster
 * @file str_utils.cc String utilities.
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

#include <stdio.h>
#include <sstream>
#include "str_utils.h"

using namespace Portage;

string& Portage::trim(string &str, const char *rmChars)
{
   if (str.length() == 0) return str;
   string::size_type first = str.find_first_not_of(rmChars);
   if (first == string::npos) first = 0;
   string::size_type last = str.find_last_not_of(rmChars);
   if (last == string::npos) last = str.length() - 1;
   str = str.substr(first, last - first + 1);
   return str;
}

char* Portage::trim(char* str, const char *rmChars)
{
   char* start = str + strspn(str, rmChars);
   Uint len = strlen(start);
   while ( len > 0 && strchr(rmChars, start[len-1]) != NULL )
      start[--len] = '\0';
   return start;
}

size_t Portage::getNextTok(const string& s, size_t pos, string& tok,
			   const string& seps, const string& backslashes,
			   const string& lquotes, const string& rquotes,
			   const string& punc, bool* was_quoted, bool quotes_break)
{
   enum {white, backslash, intok, quote, backslash_in_quote} state = white;
   size_t quote_loc = 0;	// init to keep compiler happy
   tok = "";

   if (was_quoted)
      *was_quoted = false;

   for (; pos < s.size(); ++pos) {
      switch (state) {
      case white:
	 if (backslashes.find(s[pos]) != string::npos)
	    state = backslash;
	 else if ((quote_loc = lquotes.find(s[pos])) != string::npos)
	    state = quote;
	 else if (seps.find(s[pos]) == string::npos) {
	    tok.push_back(s[pos]);
	    if (punc.find(s[pos]) != string::npos)
	       return pos+1;	// punc token
	    state = intok;
	 }
	 break;
      case backslash:
	 tok.push_back(s[pos]);
	 state = intok;
	 break;
      case intok:
	 if (backslashes.find(s[pos]) != string::npos)
	    state = backslash;
	 else if ((quote_loc = lquotes.find(s[pos])) != string::npos) {
	    state = quote;
	    if (quotes_break) return pos;
	 } else if (seps.find(s[pos]) != string::npos || 
		    punc.find(s[pos]) != string::npos)
	    return pos;		// token -> white or punc transition
	 else
	    tok.push_back(s[pos]);
	 break;
      case quote:
	 if (rquotes[quote_loc] == s[pos]) {
	    state = intok;
	    if (quotes_break) return pos+1;
	 } else if (backslashes.find(s[pos]) != string::npos)
	    state = backslash_in_quote;
	 else
	    tok.push_back(s[pos]);
	 break;
      case backslash_in_quote:
	 if (seps.find(s[pos]) == string::npos) {
	    tok.push_back(s[pos]);
	    state = quote;
	 }
	 break;
      }
      if (was_quoted && state == quote)
	 // (state == backslash || state == quote || state == backslash_in_quote))
	 *was_quoted = true;
   }
   return tok.size() || state == intok ? s.size() : string::npos;
}

bool Portage::conv(const string& str, Uint& val)
{
   char* end;
   const char* s = str.c_str();
   int v = strtol(s, &end, 10);
   val = (Uint)v;
   return v >= 0 && end != s && *end == 0;
}

/*
bool Portage::conv(const char* s, Uint& val)
{
   char* end;
   int v = strtol(s, &end, 10);
   val = (Uint)v;
   return v >= 0 && end != s && *end == 0;
}
*/

bool Portage::conv(const char* s, int& val)
{
   char* end;
   val = strtol(s, &end, 10);
   return end != s && *end == 0;
}

bool Portage::conv(const char* s, double& val)
{
   char* end;
   val = strtod(s, &end);
   return end != s && *end == 0;
}

bool Portage::conv(const char* s, float& val)
{
   char* end;
   val = (float)strtod(s, &end);
   return end != s && *end == 0;
}

bool Portage::conv(const char* s, string& val)
{
   val = s;
   return true;
}

bool Portage::conv(const string& s, string& val)
{
   val = s;
   return true;
}

bool Portage::conv(const char* s, char& val)
{
   if (s[0] != '\0' && s[1] == '\0') {
      val = s[0];
      return true;
   } else
      return false;
}

bool Portage::conv(const string& s, char& val)
{
   if (s.size() == 1) {
      val = s[0];
      return true;
   } else
      return false;
}

bool Portage::conv(const string& s, bool& val)
{
   val =  
      s == "y" || s == "yes" || s == "t" || s == "true" || s == "1" ||
      s == "Y" || s == "T";
   return true;
}



std::string& Portage::pack(Uint x, string& s, Uint base, Uint fill)
{
   Uint num_digits = 0;
   do {
      ++num_digits;
      s += (unsigned char)(x % base) + 1;
      x /= base;
   } while (x);

   // pad high digits with zeros
   for (; num_digits < fill; ++num_digits)
      s += (unsigned char) 1;
   
   return s;
}

Uint Portage::unpack(const string& s, Uint start, Uint num_chars, Uint base)
{
   Uint x = 0;
   Uint last = start + num_chars - 1;
   for (Uint i = 0; i < num_chars; ++i) {
      Uint d = (unsigned char) s[last-i];
      x = x * base + d-1;
   }
   return x;
}

string Portage::encodeRFC2396(const string& s)
{
   ostringstream ss;

   for (Uint i = 0; i < s.length(); ++i) {
      if (s[i] & (unsigned char)0x80 || s[i] == '"' || s[i] == '%' || s[i] == '>')
         ss << '%' << hex << (int)(unsigned char)s[i];
      else
	 ss << s[i];
   }
   return ss.str();
}

static Uint hexDig(char d)
{
   if (d >= 'a' && d <= 'f')
      return d - 'a' + 10;
   if (d >= 'A' && d <= 'F')
      return d - 'A' + 10;
   if (d >= '0' && d <= '9')
      return d - '0';
   else                         // undefined
      return 0;
}

string Portage::decodeRFC2396(const string& s)
{
   string d;
   for (Uint i = 0; i < s.length(); ++i)
      if (s[i] == '%' && i+2 < s.length()) {
         d += (char)(hexDig(s[i+1]) * 16 + hexDig(s[i+2]));
         i += 2;
      } else
         d += s[i];

   return d;
}

string& Portage::join(vector<string>::const_iterator beg,
                      vector<string>::const_iterator end, 
                      string& s, const string& sep)
{
   while (beg < end) {
      s += *beg;
      if (beg+1 != end)  s += sep;
      ++beg;
   }
   return s;
}

Uint Portage::split(char* s, char* tokens[], Uint max_tokens, const char* sep)
{
   char* strtok_state;
   Uint tok_count = 0;
   char* tok = strtok_r(s, sep, &strtok_state);
   while (tok != NULL && tok_count < max_tokens) {
      tokens[tok_count] = tok;
      ++tok_count;
      tok = strtok_r(NULL, sep, &strtok_state);
   }
   if ( tok != NULL ) ++tok_count;
   return tok_count;
}

char* Portage::strdup_new(const char* in) {
   if ( in ) {
      char* copy = new char[strlen(in)+1];
      return strcpy(copy, in);
   }
   return NULL;
}

