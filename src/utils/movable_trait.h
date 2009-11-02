/**
 * @author Eric Joanis
 * @file movable_trait.h  definition of movable_trait
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef MOVABLE_TRAIT_H
#define MOVABLE_TRAIT_H

#include "portage_defs.h"
#include <utility>

namespace Portage {

/**
 * Class trait that indicates whether a class/type can safely be moved
 * (memmove), copied (memcpy) and otherwise manipulated without calling its
 * copy constructor, assignment operator or destructor.  To be safe-movable,
 * the class must also be such that it can be safely deleted without calling
 * the destructor.  The regular constructor can still be mandatory to create
 * new instances, but all other operations must be correctly doable by simply
 * copying the memory allocated to an object bit-by-bit. 
 *
 * This trait can be used to specialize algorithms in order to optimize 
 * operations when constructors and destructors are optional.  For example,
 * an array T[n] can be moved/copied/saved to disk/etc very efficiently, using
 * block operations such as memmove() or std::ostream::write() and
 * std::istream::read() when T is safe-movable.  See binio.h for sample uses.
 *
 * The default is false: unless declared safe-movable, a class's constructor
 * and desctructor are assumed to be mandatory.  Specialize this template for
 * your class to declare it safe-movable.
 */
template <class T>
struct movable_trait {
   static const bool safe = false;
};

/**
 * Macro to shorten the syntax required to declare a class safe-movable, when
 * no special logic is required.
 * Use this after your class definition.
 */
#define SAFE_MOVABLE(T) template <> struct movable_trait<T> \
   { static const bool safe = true; };

/**
 * Primitive types are safe-movable.
 *
 * Note that we don't use the types defined in portage_defs.h here, because the
 * choice of int vs long vs long long in some types can cause template
 * specialization conflicts.  Instead, we declare all the base C++ types
 * safe-movable, and as a result anything defined using these will also be
 * safe-movable.
 */
SAFE_MOVABLE(bool);
SAFE_MOVABLE(char);
SAFE_MOVABLE(unsigned char);
SAFE_MOVABLE(short);
SAFE_MOVABLE(unsigned short);
SAFE_MOVABLE(int);
SAFE_MOVABLE(unsigned int);
SAFE_MOVABLE(long);
SAFE_MOVABLE(unsigned long);
SAFE_MOVABLE(long long);
SAFE_MOVABLE(unsigned long long);
SAFE_MOVABLE(float);
SAFE_MOVABLE(double);
SAFE_MOVABLE(long double);

/**
 * A std::pair is safe-movable if its two embeded types are safe-movable
 */
template <class T1, class T2>
struct movable_trait<std::pair<T1, T2> > {
   static const bool safe =
      movable_trait<T1>::safe && movable_trait<T2>::safe;
};

/**
 * Convert a boolean value into distinct classes, usable for function
 * overloading.
 */
template <unsigned int v>
struct ConstIntType {
   enum { value = v };
};

/**
 * movable_trait_t<T>::safe_t has the same semantics as movable_trait<T>::safe,
 * but converts the bool into distinct classes ConstIntType<true> and
 * ConstIntType<false>, making it easier to use in overloaded functions.
 */
template <class T>
struct movable_trait_t {
   typedef ConstIntType<movable_trait<T>::safe> safe_t;
};

} // Portage

#endif // MOVABLE_TRAIT_H
