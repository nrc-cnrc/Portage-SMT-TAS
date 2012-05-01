/**
 * @author Colin Cherry
 * @file shift_reducer.h  This file contains the declaration of
 * ShiftReducer, which allows the tracking of contiguous translation
 * blocks, even if those blocks were not used as phrases.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#ifndef SHIFT_REDUCER_H
#define SHIFT_REDUCER_H

#include "canoe_general.h"

namespace Portage
{
   class CanoeConfig;
   
   /// Placeholder for shift-reduce parser
   class ShiftReducer
   {
   public:
      
      Uint start() const {return top.start;}
      Uint end() const {return top.end;}

      Uint lBound() const {return leftBound;}
      Uint rBound() const {return rightBound;}

      bool isOneElement() {return tail==NULL;}
      
      /// Add a source phrase to the parser
      /// Immediately reduce as much as possible
      ShiftReducer(const Range& r, ShiftReducer* parent);

      /// Start a new parser for a sentence of given length
      ShiftReducer(Uint sentenceLength);

      /// Print the stack for debugging
      string toString() const;

      /// Hash function - identical parser states will have
      /// identical hash values
      ///
      /// Any feature that relies on the parser should call this
      /// inside its computeRecombHash() 
      Uint computeRecombHash();

      /// Check parser equality
      /// 
      /// Any feature that relies on the parser should call this
      /// inside its isRecombinable()
      static bool isRecombinable(ShiftReducer* p1, ShiftReducer* p2);

      /// Check to see if we need the parser
      ///
      /// One-stop-shop to list all features/whatever that could
      /// possible need the extra overhead of shift-reduce parsing
      static bool usingSR(const CanoeConfig& c);

      /// Return a count of non-ITG reductions seen during reduce()
      static Uint getNonITGCount() {return nonITGCount;}

      /// Allow only ITG reductions
      static void allowOnlyITG() {allowNonITG=false;}

      /// Counter to track non-empty final stacks
      static Uint incompleteStackCnt;

   private:
      /// The top of the shift-reduce stack
      Range top;

      /// The rest of the shift-reduce stack
      /// tail==null means we've hit bottom
      ShiftReducer* tail;

      /// Cache for hash values
      Uint hashCache;

      /// Left (inclusive) and right (exclusive) bounds to maintain
      /// 2-reducible adjacency
      Uint leftBound;
      Uint rightBound;

      /// Reduce the current top as much as possible
      void reduce();

      /// Count the number of non-ITG reductions
      static Uint nonITGCount;

      /// Are we allowed to perform non-ITG reductions?
      static bool allowNonITG;
   };
}

#endif
