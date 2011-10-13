/**
 * @author Eric Joanis and Samuel Larkin
 * @file join_details.h  Implementation details for efficient join()
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#ifndef JOIN_DETAILS_H
#define JOIN_DETAILS_H

#include <sstream>
#include <iostream>
#include <vector>
#include <string>

namespace Portage {

/**
 * Object for currying the parameters to join() until we know the intent of the
 * caller.  This allows for efficient writing to a stream without temporary
 * storage if that's the only end-use, while transparently building a resulting
 * string when needed.
 */
template <class IteratorType>
struct _Joiner {
private:
   string buffer;           ///< storage used in case the user calls c_str()
public:
   const IteratorType beg;  ///< begining of thing to join
   const IteratorType end;  ///< end of thing to join
   const string sep;        ///< separator to place between elements
   const Uint precision;    ///< stream precision to use for numerical elements

   /// Constructor
   _Joiner(IteratorType beg, IteratorType end, const string& sep, Uint precision)
      : beg(beg), end(end), sep(sep), precision(precision)
   {}

   /// Cast operator for implicit conversion to a string, which does the
   /// real joining work now that we know the caller wants an actual string.
   operator const string () const;

   /// Explicit cast request
   const string str() const {
      return *this;
   }

   /// Get a temporary const char* of the result of the join, which remains
   /// valid as long as the _Joiner itself is in scope.
   /// Yes, it is safe to use this in a printf() or error() statement.
   const char* c_str() {
      buffer = *this;
      return buffer.c_str();
   }
};

// forward declaration needed to declare the string cast operator outside
// the template class definition.
template <class IteratorType>
ostream& operator<<(ostream& os, const _Joiner<IteratorType>& joiner);

// general implementation, correct for all cases, but not optimised
// ostringstream is quite expensive, so this variant is only for when we don't
// already have a stream, and really need the stream operator <<.
template <class IteratorType>
_Joiner<IteratorType>::operator const string () const {
   ostringstream oss;
   oss << *this;
   return oss.str();
}

// optimised specializations for commonly needed cases.
// (Implementations in str_utils.cc)
template <> _Joiner<vector<string>::const_iterator>::operator const string () const;
template <> _Joiner<vector<string>::iterator>::operator const string () const;


// implementation only - documentation and declaration in str_utils.h
template <class IteratorType>
_Joiner<IteratorType>
join(IteratorType beg, IteratorType end, const string& sep=" ", Uint precision=8) {
   return _Joiner<IteratorType>(beg, end, sep, precision);
}

// implementation only - documentation and declaration in str_utils.h
template <class ContainerType>
_Joiner<typename ContainerType::const_iterator>
join(const ContainerType& c, const string& sep=" ", Uint precision=8) {
   return _Joiner<typename ContainerType::const_iterator>(c.begin(), c.end(), sep, precision);
}

/**
 * Output the join results to a stream efficiently, i.e., without any
 * unecessary temporary storage.
 */
template <class IteratorType>
ostream& operator<<(ostream& os, const _Joiner<IteratorType>& joiner) {
   int saved_precision = os.precision();
   os.precision(joiner.precision);
   IteratorType it = joiner.beg;
   if (it != joiner.end) {
      os << *it;
      ++it;
   }
   while (it != joiner.end) {
      os << joiner.sep << *it;
      ++it;
   }
   os.precision(saved_precision);
   return os;
}

// For outputing joined vectors of strings, profiling shows that it's faster
// to join them into a temporary by concatenation and calling << just once.
// I guess << has significant overhead!  :(
template <>
inline
ostream& operator<<(ostream& os, const _Joiner<vector<string>::iterator>& joiner) {
   os << string(joiner);
   return os;
}

template <>
inline
ostream& operator<<(ostream& os, const _Joiner<vector<string>::const_iterator>& joiner) {
   os << string(joiner);
   return os;
}

//@{
/// Operators provided so that join(...) behaves as much as possible as if
/// string was its actual return value.
template <class IteratorType>
const string operator+(const char* s, const _Joiner<IteratorType>& joiner) {
   return s + string(joiner);
}
template <class IteratorType>
const string operator+(const _Joiner<IteratorType>& joiner, const char* s) {
   return string(joiner) + s;
}
template <class IteratorType>
bool operator==(const _Joiner<IteratorType>& joiner, const char* s) {
   return string(joiner) == s;
}
template <class IteratorType>
bool operator==(const char* s, const _Joiner<IteratorType>& joiner) {
   return s == string(joiner);
}
template <class IteratorType>
bool operator==(const _Joiner<IteratorType>& joiner, const string& s) {
   return string(joiner) == s;
}
template <class IteratorType>
bool operator==(const string& s, const _Joiner<IteratorType>& joiner) {
   return s == string(joiner);
}
//@}

} // namespace Portage

#endif // JOIN_DETAILS_H
