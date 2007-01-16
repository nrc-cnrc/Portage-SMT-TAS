/**
 * @author George Foster
 * @file str_utils.h String utilities.
 * 
 * 
 * COMMENTS: 
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "errors.h"

namespace Portage {

/**
 * Removes beginning and trailing whitespace from the given string.
 * Whitespace is considered to include all characters in rmChars.
 * @param str           The string to trim.
 * @param rmChars       Characters considered whitespace, to be trimmed.
 * @return returns str trimmed
 */
string& trim(string &str, const char *rmChars = " \t");

/**
 * Same as trim, but over a char*.  Returns a char* to the first non-trimmed
 * character, and overwrites the end of str with "\0"s when trimming happens.
 * The pointer returned always points to an offset within str.
 * @param str           The string to trim.
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

/**
 * Convert string to Uint.
 *
 * @param s string to convert to Uint
 * @param val returned Uint value of s
 * @return true iff conv successful.
 */
extern bool conv(const string& s, Uint& val);

// For some reason this pair of signatures causes ambiguity and won't compile.
//extern bool conv(const char* s, Uint& val);
//inline bool conv(const string& s, Uint& val) { return conv(s.c_str(), val); }

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

/**
 * Join a vector of strings into a single string.
 *
 * @param beg begin iterator
 * @param end end iterator
 * @param s   result of the join operation
 * @param sep separator to put between each word
 * @return the result of the join operation (ref to s)
 */
string& join(vector<string>::const_iterator beg,
             vector<string>::const_iterator end, 
             string& s, const string& sep=" ");

/**
 * Join a vector of strings into a single string.
 *
 * @param v   whole vector to join
 * @param s   result of the join operation
 * @param sep separator to put between each word
 * @return the result of the join operation (ref to s)
 */
inline string& join(const vector<string>& v, string& s, const string& sep=" ")
{
   return join(v.begin(), v.end(), s, sep);
}

/**
 * Joins a sequence of arbitrary type into a string.
 *
 * Join a vector of T into a single string. Call this using T explicitly, eg
 * for a vector x<double>, call: join<double>(x.begin(), x.end()).
 *
 * @param beg begin iterator
 * @param end end iterator
 * @param sep separator to put between each word
 * @param precision stream precision
 * @return the result of the join operation
 */
template <class T> 
string join(typename vector<T>::const_iterator beg, 
            typename vector<T>::const_iterator end, 
            const string& sep=" ", Uint precision = 8)
{
   ostringstream ss;
   ss << setprecision(precision);

   while (beg < end) {
      ss << *beg;
      if (beg+1 != end) ss << sep;
      ++beg;
   }
   return ss.str();
}

/**
 * Join a vector of arbitrary type into a string.  See join<T> above for
 * details.
 *
 * @param v   whole vector to join
 * @param sep separator to put between each word
 * @param precision stream precision
 * @return the result of the join operation
 */
template <class T>
inline string join(const vector<T>& v, const string& sep=" ",
                   Uint precision = 8)
{
   return join<T>(v.begin(), v.end(), sep, precision);
}


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
extern Uint unpack(const string& s, Uint start, Uint num_chars, Uint base);

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
 * There's also a non-template version of this function.
 * @param s input string
 * @param dest output iterator for resulting tokens: for instance, the begin()
 * position for a string container known to be big enough (risky), or an insert
 * iterator positioned at the end of a string container (safer).  
 * @param sep the set of characters that are considered to be whitespace.  
 * @param max_toks the maximum number of tokens to extract; if > 0, then 
 * only the 1st max_toks-1 delimited tokens will be extracted, and the 
 * remainder of the string will be the last token.
 * @return the number of tokens found
 */
template <class StringOutputIter> 
size_t split(const string& s, StringOutputIter dest, const string& sep=" \t\n",
             Uint max_toks=0)
{
   size_t n = 0;
   size_t pos = 0, endpos = 0;
   while (pos < s.size() && (max_toks == 0 || n+1 < max_toks)) {
      pos = s.find_first_not_of(sep, pos);
      endpos = s.find_first_of(sep, pos);
      if (endpos > pos) {
	 *dest++ = s.substr(pos, endpos - pos);
	 ++n;
      }
      pos = endpos;
   }
   if (n < max_toks && pos < s.size()) {
      pos = s.find_first_not_of(sep, pos);
      if (pos < s.size()) {
	 *dest++ = s.substr(pos, s.size()-pos);
	 ++n;
      }
   }
   return n;
}

/**
 * Split an input string into a sequence of whitespace-delimited tokens. The
 * splitZ() version clears the output vector first.
 *
 * @see size_t split(const string& s, StringOutputIter dest, 
 *                   const string& sep, Uint max_toks)
 * @param s        input string
 * @param dest     vector that tokens get appended to
 * @param sep the  set of characters that are considered to be whitespace
 * @param max_toks the maximum number of tokens to extract; if > 0, then
 * only the 1st max_toks-1 delimited tokens will be extracted, and the
 * remainder of the string will be the last token.
 * @return the number of tokens found
 */
inline size_t
split(const string& s, vector<string>& dest, const string& sep = " \t\n", 
      Uint max_toks=0) {
   return split(s, inserter(dest, dest.end()), sep, max_toks);
}

/**
 * Same as split(const string& s, vector<string>& dest, const string& sep, Uint
 * max_toks), but clears the output vector first.
 */
inline size_t
splitZ(const string& s, vector<string>& dest, const string& sep = " \t\n", 
       Uint max_toks=0) {
   dest.clear();
   return split(s, inserter(dest, dest.end()), sep, max_toks);
}

/**
 * @brief Split with conversion.
 * @param s    string to convert
 * @param dest destination vector which holds the conversion of s
 * @param sep  character on which to split s
 * @return Return true on success.
 */
template<class T>
bool split(const string &s, vector<T> &dest, const string& sep = " \t\n")
{
   vector<string> notConverted;
   split(s, notConverted, sep);
   bool success = true;
   for (vector<string>::const_iterator it = notConverted.begin(); it < notConverted.end();
	it++) {
      dest.push_back(T());
      success = success & conv(*it, dest.back());
   } // for
   return success;
} // split

/// Same as split(const string& s, vector<T> &dest, const string& sep),
/// but clears the output vector first.
template<class T>
inline bool splitZ(const string &s, vector<T> &dest, const string& sep = " \t\n")
{
   dest.clear();
   return split(s, dest, sep);
}

/// Same as split(const string& s, vector<T> &dest, const string& sep),
/// but die on error.
template<class T>
void splitCheck(const string &s, vector<T> &dest, const string& sep = " \t\n")
{
   vector<string> notConverted;
   split(s, notConverted, sep);
   for (vector<string>::const_iterator it = notConverted.begin(); it < notConverted.end(); it++)
      dest.push_back(conv<T>(*it));
}

/// Same as split(const string& s, vector<T> &dest, const string& sep),
/// but clears the output vector first and die on error.
template<class T>
inline void splitCheckZ(const string &s, vector<T> &dest, const string& sep = " \t\n")
{
   dest.clear();
   return splitCheck(s, dest, sep);
}

/**
 * Fast split of a char*, in place, destructive.
 *
 * Much less elegant or general than the templatized split() functions above,
 * this version is optimized for raw speed, but works on much simpler and more
 * restrictive premises.
 * Returns the number of tokens found if <= max_tokens, max_tokens+1 if there
 * were too many, in which case tokens has only the max_tokens first ones.
 *
 * @param s          the input string - will be modified, and will be the
 *                   memory buffer for the resulting tokens, and therefore must
 *                   not be freed until the tokens are no longer needed
 * @param tokens     destination token list
 * @param max_tokens the size of the tokens array
 * @param sep        the set of characters that are considered to be whitespace
 * @return Returns the number of tokens found
 */
Uint split(char* s, char* tokens[], Uint max_tokens, const char* sep = " \t\n");

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
 * Verifies that the file has .Z, .z or .gz extension.
 * @param filename filename to check if it's a gzip file
 * @return Returns true if filename contains .Z, .z or .gz
 */
inline bool
isZipFile(const string& filename)
{
   const size_t dot = filename.rfind(".");
   return dot != string::npos
             && (filename.substr(dot) == ".gz"
                 || filename.substr(dot) == ".z"
                 || filename.substr(dot) == ".Z");
}

/**
 * Removes the .Z, .z or .gz extension from filename
 * @param filename file name to remove gzip extension
 * @return Returns filename without the gzip extension
 */
inline string
removeZipExtension(const string& filename)
{
   const size_t dot = filename.rfind(".");
   if (dot != string::npos
       && (filename.substr(dot) == ".gz"
           || filename.substr(dot) == ".z"
           || filename.substr(dot) == ".Z"))
   {
      return filename.substr(0, dot);
   }
   else
   {
      return filename;
   }
}

/**
 * Adds an extension to a file name and takes care of gzip files.
 * filename + toBeAdded
 * filename%(.gz|.z|.Z) + toBeAdded + .gz
 * @param filename  file name to extended
 * @param toBeAdded extension to add
 * @return Returns the extended file name
 */
inline string
addExtension(const string& filename, const string& toBeAdded)
{
   if (isZipFile(filename))
      return removeZipExtension(filename) + toBeAdded + ".gz";
   else
      return filename + toBeAdded;
}

/**
 * Strips the file name from the path
 * @param path  path from which to extract the filename
 * @return Return the file name
 */
inline string
extractFilename(const string& path)
{
   size_t pos = path.rfind("/");
   if (pos == string::npos)
      return path;
   else
      return path.substr(pos+1);
}

/**
 * Do not delete point in boost::shared_ptr. 
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

} // ends Portage namespace
#endif /*STR_UTILS_H*/
