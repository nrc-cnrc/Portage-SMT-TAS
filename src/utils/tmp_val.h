/**
 * @author Eric Joanis
 * @file tmp_val.h Temporarily change the value of a variable.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef __TMP_VAL_H__
#define __TMP_VAL_H__

namespace Portage {

   /**
    * Temporarily change the value of a variable.  Inspired by a) the behaviour
    * of the "local" keyword in Perl and b) how often we need to temporarily
    * change a variable's value and restore it later.
    *
    * Typical usage:
    * // BEFORE
    * {
    *    tmp_val<T> tmp(some_var, tmp_value_for_some_var);
    *    // do stuff that requires that some_var == tmp_value_for_some_var
    * }
    * // some_var is back to the value it had on line "BEFORE"
    *
    * Caveat: can't (yet) handle the more general case where a value comes from
    * a getter and is reset via a setter, such as an ostream's precision().
    */
   template <class T> class tmp_val {
      T saved_value;
      T& t;
    public:
      /**
       * Constructor: Temporarily change t to val, restoring it to its original
       * value when this object is destroyed (e.g., by falling out of scope).
       * @param t   variable whose value to temporarily modify
       * @param val temporary value for t
       */
      tmp_val(T& t, T val) : saved_value(t), t(t) { t = val; }

      /**
       * Destructor: reset t to its original value
       */
      ~tmp_val() { t = saved_value; }

      /**
       * Get the original value of t (without changing t)
       */
      T originalValue() const { return saved_value; }
   };

}

#endif // __TMP_VAL_H__
