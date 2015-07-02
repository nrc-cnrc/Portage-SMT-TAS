/**
 * @author George Foster
 * @file str_utils.h String utilities.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef STR_UTILS_H
#define STR_UTILS_H

#include "errors.h"
#include "join_details.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Portage {

/**
 * Removes beginning and trailing whitespace from the given string.
 * Whitespace is considered to include all characters in rmChars.
 * @param str           The string to trim - gets modified.
 * @param rmChars       Characters considered whitespace, to be trimmed.
 * @return returns str trimmed
 */
string& trim(string &str, const char *rmChars = " \t");

/**
 * Same as trim, but over a char*.  Returns a char* to the first non-trimmed
 * character, and overwrites the end of str with "\0"s when trimming happens.
 * The pointer returned always points to an offset within str.
 * @param str           The string to trim - gets modified.
 * @param rmChars       Characters considered whitespace, to be trimmed.
 * @return returns str trimmed
 */
char* trim(char* str, const char *rmChars = " \t");

/**
 * Find the longest common prefix in two strings.
 *
 * @param s1 strings to search
 * @param s2 strings to search
 * @return length of longest common prefix
 */
inline size_t longestPrefix(const char* s1, const char* s2)
{
   const char* s = s1;
   while (*s1 == *s2 && *s1)
      ++s1, ++s2;
   return s1 - s;
}

/**
 * Determine whether s1 is a prefix (not necessarily proper) of s2.
 *
 * @param s1 prefix string to search
 * @param s2 string to search
 * @return results of test
 */
inline bool isPrefix(const char* s1, const char* s2)
{
   while (*s1)
      if (*s1++ != *s2++)
         return false;
   return true;
}
inline bool isPrefix(const char* s1, const string& s2)
{
   return isPrefix(s1, s2.c_str());
}

/**
 * Determine whether s1 is a suffix (not necessarily proper) of s2.
 * @param s1 suffix string to search for
 * @param s2 string to search in
 * @return true iff s1 is a suffix of s2
 */
inline bool isSuffix(const string& s1, const string& s2)
{
   return s1.size() <= s2.size() &&
          0 == s2.compare(s2.size()-s1.size(), s1.size(), s1);
}

/**
 * Return the last character of a string (0 if string is empty).
 *
 * @param str string in which to get the last caracter
 * @return Returns the last caracter of str or 0
 */
inline char last(const string& str)
{
   return str.length() ? str[str.length()-1] : 0;
}

/**
 * Remove the last character of a string, in place.
 *
 * @param str string that we want to remove its last charater.
 * @return returns the choped string
 */
inline string& chop(string& str)
{
   if (str.length())
      str.erase(str.length()-1);
   return str;
}

/**
 * Does a string consist solely of lowercase chars? BEWARE: this uses C
 * character classification, so it's at the mercy of the locale!
 * @param str string to classify
 * @param start_char starting char for this classification
 */
inline bool isLower(const string& str, Uint start_char = 0)
{
   for (Uint i = start_char; i < str.size(); ++i)
      if (!islower(str[i]))
         return false;
   return true;
}

/**
 * Is a string capitalized: does it consist of an uppercase character followed
 * solely by lowercase characters? BEWARE: this uses C character
 * classification, so it's at the mercy of the locale!
 * @param str string to classify
 */
inline bool isCapitalized(const string& str)
{
   return str.size() > 0 && isupper(str[0]) && isLower(str, 1);
}

/**
 * Convert a string to lowercase. BEWARE: this uses C character
 * classification, so it's at the mercy of the locale!
 * @param in string to convert
 * @param out place to put conversion (may be same as in)
 * @return out
 */
inline string& toLower(const string& in, string& out)
{
   for (Uint i = 0; i < in.size(); ++i) {
      int ch = tolower(in[i]);
      if (i < out.size()) out[i] = ch;
      else out += ch;
   }
   out.erase(in.size());
   return out;
}

/// Return the name for basic types as a string.
/// This template can be specialized for any type you want to support.
/// @return The template parameter type name as a string.
template<class T> string typeName() {return "unknown-type";}
/// Same as typeName()
template<> inline string typeName<bool>() {return "bool";}
/// Same as typeName()
template<> inline string typeName<char>() {return "char";}
/// Same as typeName()
template<> inline string typeName<double>() {return "double";}
/// Same as typeName()
template<> inline string typeName<float>() {return "float";}
/// Same as typeName()
template<> inline string typeName<int>() {return "int";}
/// Same as typeName()
template<> inline string typeName<string>() {return "string";}
/// Same as typeName()
template<> inline string typeName<Uint>() {return "Uint";}
/// Same as typeName()
template<> inline string typeName<Int64>() {return "Int64";}
/// Same as typeName()
template<> inline string typeName<Uint64>() {return "Uint64";}

/**
 * Convert string to Uint.
 *
 * @param s string to convert to Uint
 * @param val returned Uint value of s
 * @return true iff conv successful.
 */
extern bool conv(const char* s, Uint& val);
/// Same as conv(const char* s, Uint& val);
inline bool conv(const string& s, Uint& val) { return conv(s.c_str(), val); }

/**
 * Convert string to int.
 *
 * @param s string to convert to int
 * @param val returned int value of s
 * @return true iff conv successful.
 */
extern bool conv(const char* s, int& val);
/// Same as conv(const char* s, int& val).
inline bool conv(const string& s, int& val) { return conv(s.c_str(), val); }

/**
 * Convert string to Uint64.
 *
 * @param s string to convert to Uint64
 * @param val returned Uint64 value of s
 * @return true iff conv successful.
 */
extern bool conv(const char* s, Uint64& val);
/// Same as conv(const char* s, Uint64& val);
inline bool conv(const string& s, Uint64& val) { return conv(s.c_str(), val); }

/**
 * Convert string to Int64.
 *
 * @param s string to convert to Int64
 * @param val returned Int64 value of s
 * @return true iff conv successful.
 */
extern bool conv(const char* s, Int64& val);
/// Same as conv(const char* s, Int64& val).
inline bool conv(const string& s, Int64& val) { return conv(s.c_str(), val); }

/**
 * Convert string to double.
 *
 * @param s string to convert to double
 * @param val returned double value of s
 * @return true iff conv successful.
 */
extern bool conv(const char* s, double& val);
/// Same as conv(const char* s, double& val).
inline bool conv(const string& s, double& val) { return conv(s.c_str(), val); }

/**
 * Convert string to float.
 *
 * @param s string to convert to float
 * @param val returned float value of s
 * @return true iff conv successful.
 */
extern bool conv(const char* s, float& val);
/// Same as conv(const char* s, float& val).
inline bool conv(const string& s, float& val) { return conv(s.c_str(), val); }

/**
 * Convert string to string (just copy).
 *
 * @param s   buffer to convert to a string
 * @param val returned string value of s
 * @return true.
 */
extern bool conv(const char* s, string& val);
/// Same as conv(const char* s, string& val).
extern bool conv(const string& s, string& val);

/**
 * Convert string to char
 *
 * @param s string to convert to char
 * @param val returned char value of s
 * @return true if s is a single char, otherwise false
 */
extern bool conv(const char* s, char& val);
/// Same as conv(const char* s, char& val).
extern bool conv(const string& s, char& val);

/**
 * Convert string to boolean
 *
 * @param s string to convert
 * @param val set to true iff s is "y", "Y", "yes", "t", "T", "true", or "1"
 * @return true
 */
extern bool conv(const string& s, bool& val);
/// Same as conv(const string& s, bool& val).
inline bool conv(const char* s, bool& val) { return conv(string(s), val); }

/**
 * string conversion to a arbitrary type.
 *
 * Conversion with error checking. Call this with an explicit template param,
 * for example:
 *    float good_float = conv<float>(possible_float_string);
 *
 * @param s string to convert to arbitrary type
 * @return conversion of s
 */
template <class T> T conv(const string& s)
{
   T val;
   if (!conv(s, val))
      error(ETFatal, "can't convert <%s> to %s", s.c_str(), typeName<T>().c_str());
   return val;
}
/// Same as conv(const string& s).
template <class T> T conv(const char* s)
{
   T val;
   if (!conv(s, val))
      error(ETFatal, "can't convert <%s> to %s", s, typeName<T>().c_str());
   return val;
}

template <class T> bool convT(const char* s, T& val)
{
   return conv(s, val);
}
// The const string& overload of convT must not exist:
// Makes compilation of convT<T>  ambiguous
//template <class T> bool convT(const string& s, T& val);


template <class T> bool convCheck(const char* s, T& val)
{
   if (!conv(s, val))
      error(ETFatal, "can't convert <%s> to %s", s, typeName<T>().c_str());
   return true;
}
// The const string& overload of convCheck must not exist:
// Makes compilation of convCheck<T>  ambiguous
//template <class T> bool convCheck(const string& s, T& val);

/// Convert any type to a string via an ostringstream
/// This is slow but convenient - avoid in tight loops
template <class T>
string toString(const T& val)
{
   ostringstream oss;
   oss << val;
   return oss.str();
}

/**
 * Join a vector of strings into a single string, appending the results to s.
 *
 * @param beg begin iterator
 * @param end end iterator
 * @param s   result of the join operation
 * @param sep separator to put between each word
 * @return the result of the join operation (ref to s)
 */
string& join_append(vector<string>::const_iterator beg,
                    vector<string>::const_iterator end,
                    string& s, const string& sep=" ");

/**
 * Join a vector of strings into a single string, appending the results to s.
 *
 * @param v   whole vector to join
 * @param s   result of the join operation
 * @param sep separator to put between each word
 * @return the result of the join operation (ref to s)
 */
inline string& join_append(const vector<string>& v, string& s, const string& sep=" ")
{
   return join_append(v.begin(), v.end(), s, sep);
}

/**
 * Joins a sequence of arbitrary type into a string.
 *
 * The return value can be used as if it was a string:
 * string s = join(...);
 * But it is optimized to avoid temporaries when output to a stream:
 * cerr << join(...) << endl;   (efficient: no temporary copy)
 * It is safe to call c_str() on the result in a printf() or error() statement:
 * error(ETWarn, "... %s ...", join(...).c_str());
 *
 * @param beg begin iterator
 * @param end end iterator
 * @param sep separator to put between each word
 * @param precision stream precision
 * @return the result of the join operation, usable as if it was a string
 */
template <class IteratorType>
_Joiner<IteratorType>
join(IteratorType beg, IteratorType end, const string& sep=" ", Uint precision=8);

/**
 * Join a vector/container of arbitrary type into a string.  See
 * join<IteratorType> above for details and safe/optimized uses.
 *
 * @param c    whole vector/container to join (c.begin() and c.end() must both
 *             return ContainerType::const_iterator.)
 * @param sep  separator to put between each word
 * @param precision stream precision
 * @return the result of the join operation, usable as if it was a string
 */
template <class ContainerType>
_Joiner<typename ContainerType::const_iterator>
join(const ContainerType& c, const string& sep=" ", Uint precision=8);

/**
 * Pack an integer into a string.  Codes it as a base-b number and writing
 * one digit per character of the string, low digit first. To avoid 0
 * characters, 1 is added to each digit before writing to the string.
 * @param x     integer to code
 * @param s     string to append code to
 * @param base  in 2..255 (NB, not 256 due to 0-digit avoidance)
 * @param fill  minimum number of digits to write - number will be padded with
 *              0's up to this amount.
 * @return s
 */
extern string& pack(Uint x, string& s, Uint base, Uint fill = 0);

/**
 * Unpack an integer coded by pack.
 *
 * @param s coded string
 * @param start start position in s
 * @param num_chars number of characters to unpack
 * @param base used for packing
 * @return packed integer
 */
extern Uint unpack(const char* s, Uint start, Uint num_chars, Uint base);

/**
 * Code a character string in RFC2396 (bytewise hex codes)
 *
 * @param s string to encode
 * @return return coded version
 */
extern string encodeRFC2396(const string& s);

/**
 * Decode a character string in RFC2396 (bytewise hex codes)
 *
 * @param s string to decode
 * @return return decoded version
 */
extern string decodeRFC2396(const string& s);


/**
 * Split an input string into a sequence of whitespace-delimited tokens.
 * The * splitZ() version clears the output vector first.
 *
 * @see Uint split(const char* s, vector<T>& dest, Converter converter, const char* sep, Uint max_toks)
 * @param s          input string
 * @param dest       vector that tokens get appended to
 * @param converter  a mapper from a string to desire T type (Applied to every token found)
 *                   definition: Converter(const char* src, T dest)
 * @param sep        the set of characters that are considered to be whitespace
 * @param max_toks   the maximum number of tokens to extract; if > 0, then
 *                   only the 1st max_toks-1 delimited tokens will be
 *                   extracted, and the remainder of the string will be the
 *                   last token; 0 means extract everything.
 * @return the number of tokens found
 */
template<class T, class Converter>
Uint split(const char* s, T* dest, Converter converter, const char* sep = " \t\n", Uint max_toks = 0)
{
   Uint init_size(0);
   // Make a copy to work on
   const Uint len = strlen(s);
   char work[len+1];
   strcpy(work, s);
   assert(work[len] == '\0');

   char* strtok_state;
   const char* tok = strtok_r(work, sep, &strtok_state);
   while (tok != NULL && (!max_toks || init_size < max_toks)) {
      conv(tok, dest[init_size++]);
      tok = strtok_r(NULL, sep, &strtok_state);
   }
   // Last token will contain the remainder of the string
   if (max_toks && tok != NULL && init_size < max_toks) {
      conv(&s[tok-work], dest[init_size++]);
   }

   return init_size;
}



/**
 * Split an input string into a sequence of whitespace-delimited tokens.
 * The * splitZ() version clears the output vector first.
 *
 * @see Uint split(const char* s, vector<T>& dest, Converter converter, const char* sep, Uint max_toks)
 * @param s          input string
 * @param dest       vector that tokens get appended to
 * @param converter  a mapper from a string to desire T type (Applied to every token found)
 *                   definition: Converter(const char* src, T dest)
 * @param sep        the set of characters that are considered to be whitespace
 * @param max_toks   the maximum number of tokens to extract; if > 0, then
 *                   only the 1st max_toks-1 delimited tokens will be
 *                   extracted, and the remainder of the string will be the
 *                   last token; 0 means extract everything.
 * @return the number of tokens found
 */
template<class T, class Converter>
Uint split(const char* s, vector<T>& dest, Converter converter, const char* sep = " \t\n", Uint max_toks = 0)
{
   const Uint init_size(dest.size());
   // Make a copy to work on
   const Uint len = strlen(s);
   // This arbitrary limit is close to default stack size; issue fatal error instead of seg fault.
   if (len > 1000000)
      error(ETFatal, "Trying to split a line that is longer 1 million characters - there's probably something wrong with your input.\n");
   char work[len+1];
   strcpy(work, s);
   assert(work[len] == '\0');

   char* strtok_state;
   const char* tok = strtok_r(work, sep, &strtok_state);
   while (tok != NULL && (!max_toks || dest.size()-init_size+1 < max_toks)) {
      dest.push_back(T());
      if (!converter(tok, dest.back()))
         error(ETFatal, "Unable to convert %s into %s", tok, typeName<T>().c_str());
      tok = strtok_r(NULL, sep, &strtok_state);
   }
   // Last token will contain the remainder of the string
   if (max_toks && tok != NULL && dest.size()-init_size < max_toks) {
      dest.push_back(T());
      converter(&s[tok-work], dest.back());
   }

   return dest.size() - init_size;
}

/**
 * Split an input string into a sequence of whitespace-delimited tokens.
 * The * splitZ() version clears the output vector first.
 *
 * @see Uint split(const char* s, vector<T>& dest, Converter converter, const char* sep, Uint max_toks)
 * @param s          input string
 * @param dest       vector that tokens get appended to
 * @param sep        the set of characters that are considered to be whitespace
 * @param max_toks   the maximum number of tokens to extract; 0 means
 *                   extract everything
 * @return the number of tokens found
 */
template<class T>
Uint split(const char* s, vector<T>& dest, const char* sep = " \t\n", Uint max_toks = 0)
{
   return split(s, dest, convT<T>, sep, max_toks);
}

template<class T>
vector<T> split(const char* s, const char* sep = " \t\n", Uint max_toks = 0)
{
   vector<T> dest;
   split(s, dest, convT<T>, sep, max_toks);
   return dest;
}

/**
 * Split an input string into a sequence of whitespace-delimited tokens.
 * The * splitZ() version clears the output vector first.
 *
 * @see Uint split(const char* s, vector<T>& dest, Converter converter, const char* sep, Uint max_toks)
 * @param s          input string
 * @param dest       vector that tokens get appended to
 * @param sep        the set of characters that are considered to be whitespace
 * @param max_toks   the maximum number of tokens to extract; 0 means
 *                   extract everything
 * @return the number of tokens found
 */
template<class T>
Uint split(const string& s, vector<T>& dest, const char* sep = " \t\n", Uint max_toks = 0)
{
   return split(s.c_str(), dest, convT<T>, sep, max_toks);
}

template<class T>
vector<T> split(const string& s, const char* sep = " \t\n", Uint max_toks = 0)
{
   vector<T> dest;
   split(s.c_str(), dest, convT<T>, sep, max_toks);
   return dest;
}

/// Same as split(const string& s, vector<T> &dest, const string& sep),
/// but clears the output vector first.
template<class T>
Uint splitZ(const string &s, vector<T> &dest, const char* sep = " \t\n", Uint max_toks = 0)
{
   dest.clear();
   return split(s.c_str(), dest, sep, max_toks);
}

/// Same as split(const string& s, vector<T> &dest, const string& sep),
/// but die on error.
template<class T>
Uint splitCheck(const string &s, vector<T> &dest, const char* sep = " \t\n", Uint max_toks = 0)
{
   return split(s.c_str(), dest, convCheck<T>, sep, max_toks);
}

/// Same as split(const string& s, vector<T> &dest, const string& sep),
/// but clears the output vector first and die on error.
template<class T>
Uint splitCheckZ(const string &s, vector<T> &dest, const char* sep = " \t\n", Uint max_toks = 0)
{
   dest.clear();
   return splitCheck(s, dest, sep, max_toks);
}

/**
 * Fast split of a char*, in place, destructive.
 *
 * Much less elegant or general than the templatized split() functions above,
 * this version is optimized for raw speed, but works on much simpler and more
 * restrictive premises.
 * Returns the number of tokens found.
 *
 * If there are max_tokens or more tokens in s, extracts the first max_tokens-1
 * tokens, puts the rest of the string in tokens[max_tokens-1], and returns
 * max_tokens.  If you expect exactly n tokens, pass max_tokens=n+1 and check
 * that the return value is n.  If you pass max_tokens=n, then tokens[n-1] will
 * not have trailing separator characters trimmed out and you won't be able to
 * confirm your input was valid.
 *
 * @param s          the input string - will be modified, and will be the
 *                   memory buffer for the resulting tokens, and therefore must
 *                   not be freed until the tokens are no longer needed
 * @param tokens     destination token list.  must have size >= max_tokens
 * @param max_tokens the size of the tokens array
 * @param sep        the set of characters that are considered to be whitespace
 * @return Returns the number of tokens found
 */
Uint destructive_split(char* s, char* tokens[], Uint max_tokens, const char* sep = " \t\n");

/// Same as argument destructive_split(), but without a maximum number of tokens
/// tokens is cleared before adding the tokens from s to it.
Uint destructive_splitZ(char* s, vector<char*>& tokens, const char* sep = " \t\n");

/**
 * Gets the next token.
 *
 * Get the next non-whitespace token, beginning at a specified position in an
 * input string. Quotes may be used to include whitespace (or other special
 * characters) in a token.
 *
 * @param s input string
 * @param pos the position in s to start looking for the token
 * @param tok the place to put the retrieved token (not necessarily a substring
 * of the input, due to removed quotes; also may be empty due to "")
 * @param seps the set of characters that are considered to be whitespace
 * @param backslashes the set of characters that act as backslash operators:
 * each backslash quotes the immediately following character, making it part of
 * a token; also, backslashes within quoted strings additionally cause any
 * following whitespace to be erased (this allows for multi-line quoted strings)
 * @param lquotes the set of characters that begin quotations: all characters
 * between a left and right quote are part of a token
 * @param rquotes the set of characters that end quotations: the ith character
 * in rquotes matches the ith in lquotes.
 * @param punc the set of characters that form tokens on their own, even if
 * embedded in non-whitespace character sequences.
 * @param was_quoted if not NULL, contents are set to true if any part of the
 * returned token was quoted (with quotes, not backslashes)
 * @param quotes_break quotes break tokens
 * @return the position to start looking for the next token (possibly s.size())
 * if a valid token was found this time; string::npos if not (the latter case
 * corresponds to whitespace at the end of the string or to a quote immediately
 * before the end of the string).
 */
extern size_t getNextTok(const string& s, size_t pos, string& tok,
                         const string& seps = " \t\n",
                         const string& backslashes = "\\",
                         const string& lquotes = "\"'",
                         const string& rquotes = "\"'",
                         const string& punc = "",
                         bool* was_quoted = NULL,
                         bool quotes_break = false);

/**
 * Split an input string into a sequence of whitespace-delimited and possibly
 * quoted tokens.
 * @param s input string
 * @param dest output iterator for resulting tokens: for instance, the begin()
 * position for a string container known to be big enough (risky), or an insert
 * iterator positioned at the end of a string container (safer).
 * @param seps         see getNextTok()
 * @param backslashes  see getNextTok()
 * @param lquotes      see getNextTok()
 * @param rquotes      see getNextTok()
 * @param punc         see getNextTok()
 * @param retain_quotes leave (normalized) quotes around quoted strings
 * @param omit_quoted_tokens
 */
template <class StringOutputIter>
void splitQuoted(const string& s, StringOutputIter dest,
                 const string& seps = " \t\n", const string& backslashes = "\\",
                 const string& lquotes = "\"'", const string& rquotes = "\"'",
                 const string& punc = "", bool retain_quotes = false,
                 bool omit_quoted_tokens = false)
{
   string tok;
   size_t pos = 0;
   bool was_quoted;
   do {
      pos = getNextTok(s, pos, tok, seps, backslashes, lquotes, rquotes, punc,
                       &was_quoted);
      if (pos == string::npos) break;
      else {
         if (was_quoted) {
            if (!omit_quoted_tokens)
               *dest++ = retain_quotes ? lquotes[0] + tok + rquotes[0] : tok;
         } else
            *dest++ = tok;
      }
   } while (pos < s.size());
}

/**
 * Split an input string into a sequence of whitespace-delimited and possibly
 * quoted tokens.
 * @param s input string
 * @param dest vector that tokens get appended to
 * @param seps         see getNextTok()
 * @param backslashes  see getNextTok()
 * @param lquotes      see getNextTok()
 * @param rquotes      see getNextTok()
 * @param punc         see getNextTok()
 * @param retain_quotes leave (normalized) quotes around quoted strings
 * @param omit_quoted_tokens should we omit quoted tokens?
 */
inline void
splitQuoted(const string& s, vector<string>& dest,
            const string& seps = " \t\n", const string& backslashes = "\\",
            const string& lquotes = "\"'", const string& rquotes = "\"'",
            const string& punc = "", bool retain_quotes = false,
            bool omit_quoted_tokens = false)
{
   splitQuoted(s, inserter(dest, dest.end()), seps, backslashes, lquotes,
               rquotes, punc, retain_quotes, omit_quoted_tokens);
}

/**
 * Split an input string into a sequence of tokens delimited by the (possibly
 * multi-character) string in sep.
 * Unlike the other split* functions, the delimiter is not just any character
 * in sep, but the string sep used as a whole.
 * @param s input string
 * @param dest vector that tokens get appended to
 * @param sep token separator
 */
inline void
splitString(const string& s, vector<string>& dest, const string& sep = " ") {
   size_t ppos = 0, pos = 0, sep_len = sep.length();
   while (pos != string::npos) {
      pos = s.find(sep, ppos);
      dest.push_back(s.substr(ppos, pos-ppos));
      ppos = pos + sep_len;
   }
}

/**
 * Like splitString(), but clears the output vector first.
 * @param s input string
 * @param dest vector that tokens get appended to (cleared by splitStringZ)
 * @param sep token separator
 */
inline void
splitStringZ(const string& s, vector<string>& dest, const string& sep = " ") {
   dest.clear();
   splitString(s, dest, sep);
}

/**
 * Do not delete pointer in boost::shared_ptr.
 *
 * Utility class for using boost::shared_ptr: the null deleter says don't
 * delete anything when the object is released.
 * This doesn't really belong here, but should be in a central location, and
 * here seems like the easiest place to put it.
 */
struct NullDeleter
{
    /// Makes the class a callable entity.
    void operator()(void const *) const {}
};

/**
 * C++ compliant strdup, using new[] instead of malloc.
 * @param in char string to copy
 * @return NULL if in is NULL, else a freshly new[]'d copy; delete with
 *         delete[] when no longer needed
 */
char* strdup_new(const char* in);

/**
 */
inline void replaceAll(string& str, const string& f, const string& t) {
   if (f.empty()) return;
   size_t pos = 0;
   const size_t fl = f.size();
   const size_t tl = t.size();
   while ((pos = str.find(f, pos)) != string::npos) {
      str.replace(pos, fl, t);
      pos += tl;
   }
}

} // ends Portage namespace
#endif /*STR_UTILS_H*/
