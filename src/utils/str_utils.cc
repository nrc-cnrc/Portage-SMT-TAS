/**
 * @author George Foster
 * @file str_utils.cc String utilities.
 * 
 * 
 * COMMENTS: 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <stdio.h>
#include <sstream>
#include "str_utils.h"
#include <cstdlib>
#include <limits>
#include <cerrno>

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

bool Portage::conv(const char* s, Uint& val)
{
   char* end;
   errno = 0;
   // v must be a 64 bit int so we can detect over-/underflow correctly.
   // We do not use strtoul because it silently accepts negative numbers and
   // casts them to positive ones.
   // On 64 bit machines, strtol would work, but not on 32 bit machines.
   const Int64 v = strtoll(s, &end, 10);
   val = (Uint)v;
   static const Int64 min(0), max(numeric_limits<Uint>::max());
   return errno == 0 && v >= min && v <= max && end != s && *end == 0;
}

bool Portage::conv(const char* s, int& val)
{
   char* end;
   errno = 0;
   // v must be long int, the return type of strtol, not int nor Int64!!!  
   // On a 32 bit machine, long int is the same as int, but on a 64 bit machine
   // long int is the same as Int64, so strtol itself has a different
   // implementation on the two architectures.  The implementation here is
   // designed to correctly parse s into a 32-bit int on either architecture,
   // and to reliably detect over/underflow on both architectures.
   const long int v = strtol(s, &end, 10);
   val = (int)v;
   static const long int min(numeric_limits<int>::min()),
                         max(numeric_limits<int>::max());
   return errno == 0 && v >= min && v <= max && end != s && *end == 0;
}

bool Portage::conv(const char* s, Uint64& val)
{
   char* end;
   errno = 0;

   while (isspace(*s)) ++s;
   bool negative = (*s == '-');
   val = strtoull(s, &end, 10);
   return errno == 0 && !negative && end != s && *end == 0;

   /*
   // Using strroll(), as done here, will not work for values in (LLONG_MAX ..
   // ULLONG_MAX].
   // Using strtoull will not work either, because it silently accepts negative
   // input as correct.
   Int64 v = strtoll(s, &end, 10);
   val = (Uint64)v;
   return errno == 0 && v >= 0 && end != s && *end == 0;
   */
}

bool Portage::conv(const char* s, Int64& val)
{
   char* end;
   errno = 0;
   val = strtoll(s, &end, 10);
   return errno == 0 && end != s && *end == 0;
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

Uint Portage::unpack(const char* s, Uint start, Uint num_chars, Uint base)
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

string& Portage::join_append(vector<string>::const_iterator beg,
                             vector<string>::const_iterator end, 
                             string& s, const string& sep)
{
   if (s.capacity() == 0) {
      // This optimization can help with a fresh string, but can be harmful if
      // done repeatedly in a tight loop over the same string buffer s
      // Profiling shows this optimization is worthwhile.
      Uint capacity_needed(0);
      Uint sep_len = sep.size();
      for ( vector<string>::const_iterator it = beg; it != end; ++it )
         capacity_needed += it->size() + sep_len;
      if ( beg < end ) capacity_needed -= sep_len;
      s.reserve(s.size() + capacity_needed);
   }

   if (beg != end) {
      s += *beg;
      ++beg;
   }
   while (beg != end) {
      s += sep;
      s += *beg;
      ++beg;
   }
   return s;
}

// optimized specializations for join() results going to an actual string
template <>
_Joiner<vector<string>::const_iterator>::operator const string () const {
   string s;
   join_append(beg, end, s, sep);
   return s;
}
template <>
_Joiner<vector<string>::iterator>::operator const string () const {
   string s;
   join_append(beg, end, s, sep);
   return s;
}

Uint Portage::destructive_split(char* s, char* tokens[], Uint max_tokens, const char* sep)
{
   char* strtok_state;
   Uint tok_count = 0;
   char* tok = strtok_r(s, sep, &strtok_state);
   while (tok != NULL && tok_count + 2 < max_tokens) {
      tokens[tok_count++] = tok;
      tok = strtok_r(NULL, sep, &strtok_state);
   }
   if ( tok != NULL ) {
      tokens[tok_count++] = tok;
      assert(tok_count <= max_tokens);
   }
   if ( strtok_state != NULL ) {
      // This bit of code is ugly, and relies on internals of strtok_r, but
      // it is well tested via unit testing, so it is in fact reliable.
      // We do this ugly stuff to return in tokens[max_tokens-1] the rest of
      // s starting at the beginning of the max_tokens'th token.  If we use
      // strtok_r again, a null is actually inserted at the end of the
      // max_tokens'th token, which is not what we want here.

      // skip past any characters from sep at the beginnig of strtok_state
      while ( *strtok_state != '\0' && strchr(sep, *strtok_state) )
         ++strtok_state;
      if ( *strtok_state != '\0' ) {
         tokens[tok_count++] = strtok_state;
         assert(tok_count <= max_tokens);
      }
   }
   return tok_count;
}

char* Portage::strdup_new(const char* in) {
   if ( in ) {
      char* copy = new char[strlen(in)+1];
      return strcpy(copy, in);
   }
   return NULL;
}

