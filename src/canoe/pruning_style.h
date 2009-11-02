/**
 * @author Samuel Larkin
 * @file pruning_style.h
 *
 * $Id$
 *
 * Calculate the pruning value for filtering phrase tables.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef __PRUNING_STYLE_H__
#define __PRUNING_STYLE_H__

#include "portage_defs.h"
#include <string>

namespace Portage {
using namespace std;

class pruningStyle {
public:
   virtual ~pruningStyle() {}
   virtual Uint operator()(Uint number_of_word) const = 0;
   virtual string description() const = 0;

   static pruningStyle* create(const string& type, Uint limit);
};


class fixPruning : public pruningStyle {
private:
   const Uint L;
public:
   fixPruning(Uint L) : L(L) {}
   virtual Uint operator()(Uint) const;
   virtual string description() const;
};


class linearPruning : public pruningStyle {
private:
   const Uint constant;
public:
   linearPruning(Uint constant) : constant(constant) {}
   virtual Uint operator()(Uint number_of_word) const;
   virtual string description() const;
};

};  //  ends namespace Portage

#endif  // __PRUNING_STYLE_H__
