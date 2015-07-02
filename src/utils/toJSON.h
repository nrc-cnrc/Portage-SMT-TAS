#ifndef  __TO_JSON__H_
#define __TO_JSON__H_

#include <ostream>
#include <string>
#include <algorithm>
#include "str_utils.h"

/*
 */
namespace Portage {

/**
 * Simple proxy for data to be written to json format.
 */
template <class T>
struct _valueJSON {
   const T& value;

   _valueJSON(const T& value)
   : value(value)
   { }
};

/**
 * Simple proxy for data to be written to json format.
 */
template <class T, class E>
struct _valueExtraJSON {
   const T& value;
   const E& extra;

   _valueExtraJSON(const T& value, const E& extra)
   : value(value)
   , extra(extra)
   { }
};

/**
 * Simple proxy for key/value data to be written to json format.
 */
template <class T>
struct _keyValueJSON {
   const char* const key;
   const T& value;

   _keyValueJSON(const char * const key, const T& value)
   : key(key)
   , value(value)
   { }
};


/**
 * Wraps an arbitrary type to write it in json format.
 */
template <class T>
_valueJSON<T> to_JSON(const T& value) {
   return _valueJSON<T>(value);
}

/// Converts float to json format.
float to_JSON(const float& o);

/// Converts double to json format.
double to_JSON(const double& o);

/// Converts int to json format.
int to_JSON(const int& o);

/// Converts long to json format.
long to_JSON(const long& o);

/// Converts unsigned int to json format.
unsigned int to_JSON(const unsigned int& o);

/// Converts unsigned long to json format.
unsigned long to_JSON(const unsigned long& o);

/// Converts unsigned bool to json format.
const char* to_JSON(const bool& o);


/**
 * Wraps an arbitrary type to write it in json format.
 */
template <class T, class E>
_valueExtraJSON<T, E> to_JSON(const T& value, const E& extra) {
   return _valueExtraJSON<T, E>(value, extra);
}

/**
 * Wraps an arbitrary key/value pair to write it in json format.
 */
template <class T>
_keyValueJSON<T> to_JSON(const char * const key, const T& value) {
   return _keyValueJSON<T>(key, value);
}



/**
 * Helper function that, given an object key, return "key": .
 */
string keyJSON(const char* const key);





/**
 * For non-basic types, invoke toJSON on the object to write that object to
 * json format to the stream.
 */
template <class T>
inline ostream& operator<<(ostream& out, const _valueJSON<T>& o) {
   return o.value.toJSON(out);
}

/**
 * For non-basic types, invoke toJSON on the object to write that object to
 * json format to the stream.
 */
template <class T>
inline ostream& operator<<(ostream& out, const _valueJSON<T*>& o) {
   if (o.value == NULL)
      return out << "null";
   else
      return o.value->toJSON(out);
}

/**
 * For string, quote the string.
 */
template <>
inline ostream& operator<<(ostream& out, const _valueJSON<string>& o) {
   string c(o.value);
   replaceAll(c, string("\""), string("\\\""));
   return out << '"' << c << '"';
}

/**
 * For char buffer, quote the string.
 */
template <>
inline ostream& operator<<(ostream& out, const _valueJSON<const char *>& o) {
   // We need a temporary string to live at least as long as _valueJSON
   // returned by to_JSON since _valueJSON only hold a reference to the
   // temporary string.
   return out << to_JSON(string(o.value));
}



/*
template <>
inline ostream& operator<<(ostream& out, const _valueJSON<int>& o) {
   return out << o.value;
}

template <>
inline ostream& operator<<(ostream& out, const _valueJSON<unsigned int>& o) {
   return out << o.value;
}

template <>
inline ostream& operator<<(ostream& out, const _valueJSON<long>& o) {
   return out << o.value;
}

template <>
inline ostream& operator<<(ostream& out, const _valueJSON<unsigned long>& o) {
   return out << o.value;
}

template <>
inline ostream& operator<<(ostream& out, const _valueJSON<float>& o) {
   return out << o.value;
}

template <>
inline ostream& operator<<(ostream& out, const _valueJSON<double>& o) {
   return out << o.value;
}

template <>
inline ostream& operator<<(ostream& out, const _valueJSON<bool>& o) {
   return out << (o.value ? "true" : "false");
}
*/




/**
 * For vectors, invoke to_JSON on all items to write them in json format to the
 * stream.
 */
template <class T>
inline ostream& operator<<(ostream& out, const _valueJSON<vector<T> >& o) {
   typename vector<T>::const_iterator it = o.value.begin();
   typename vector<T>::const_iterator it_end = o.value.end();

   out << '[';
   if (it != it_end) {
      out << to_JSON(*it);
      ++it;
   }
   while (it != it_end) {
      out << ',' << to_JSON(*it);
      ++it;
   }

   out << ']';
   return out;
}


/**
 * For non-basic types, invoke toJSON on the object to write that object to
 * json format to the stream.
 */
template <class T, class E>
inline ostream& operator<<(ostream& out, const _valueExtraJSON<T, E>& o) {
   return o.value.toJSON(out, o.extra);
}

/**
 * For non-basic types, invoke toJSON on the object to write that object to
 * json format to the stream.
 */
template <class T, class E>
inline ostream& operator<<(ostream& out, const _valueExtraJSON<T*, E>& o) {
   if (o.value == NULL)
      return out << "null";
   else
      return o.value->toJSON(out, o.extra);
}

/**
 * For vectors, invoke to_JSON on all items to write them in json format to the
 * stream.
 */
template <class T, class E>
inline ostream& operator<<(ostream& out, const _valueExtraJSON<vector<T*>, E>& o) {
   typename vector<T*>::const_iterator it = o.value.begin();
   typename vector<T*>::const_iterator it_end = o.value.end();

   out << '[';
   if (it != it_end) {
      out << to_JSON(*it, o.extra);
      ++it;
   }
   while (it != it_end) {
      out << ',' << to_JSON(*it, o.extra);
      ++it;
   }

   out << ']';
   return out;
}



/**
 * Write a key/value to the stream, in json format.
 */
template <class T>
inline ostream& operator<<(ostream& out, const _keyValueJSON<T>& o) {
   return out << to_JSON(o.key) << ':' << to_JSON(o.value);
}

};  // namespace Portage


#endif  // __TO_JSON__H_
