/**
 * @author Colin Cherry
 * @file shift_reducer.cc  This file contains the implementation of the
 * ShiftReducer object, used primarily for Hierarchical Lexical Distortion.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include "shift_reducer.h"
#include "config_io.h"
#include "distortionmodel.h"

Uint ShiftReducer::nonITGCount=0;

Uint ShiftReducer::incompleteStackCnt=0;

bool ShiftReducer::allowNonITG=true;

ShiftReducer::ShiftReducer(const Range& r, ShiftReducer* parent)
   :top(r)
   ,tail(parent)
   ,hashCache(0)
{
   assert(parent!=NULL);

   // Bound processing non-deterministic with 0-width phrases
   //assert(top.start!=top.end);
   // EJJ: this assertion is too strict, because it prevents us from using
   // [sentlen,sentlen) as an artificial phrase to represent the jump to the
   // end of the sentence.  Hence replace this assertion with the one inside
   // the if statement below, which catches strictly non-deterministic
   // situations.

   // Figure out new bounds
   if(r.end<=parent->start()) {
      leftBound = parent->leftBound;
      rightBound = parent->start();
      // EJJ make sure the non-deterministic case never occurs.
      assert(!(r.start>=parent->end()));
   } else if(r.start>=parent->end()) {
      leftBound = parent->end();
      rightBound = parent->rightBound;
   } else {
      assert(false); // Should be one of the above cases
   }
   reduce();
}

ShiftReducer::ShiftReducer(Uint sentSize)
   :top(Range(0,0))
   ,tail(NULL)
   ,leftBound(0)
   ,rightBound(sentSize)
{}

string ShiftReducer::toString() const {
   ostringstream s;
   const ShiftReducer* prev = this;
   while(prev!=NULL) {
      if(prev!=this) s << " ";
      s << "|" << prev->lBound() << " "
        << prev->top.toString() << " "
        << prev->rBound() << "|";
      prev = prev->tail;
   }
   return s.str();
}


Uint ShiftReducer::computeRecombHash()
{
   if(hashCache!=0) {
      hashCache = start()+17*end();
      if(tail!=NULL) {
         hashCache*=17;
         hashCache+=tail->computeRecombHash();
      }
   }
   return hashCache;
}

bool ShiftReducer::isRecombinable(ShiftReducer* p1, ShiftReducer* p2)
{
   // Boring cases involving NULL
   if(p1==NULL && p2==NULL) return true;
   if(p1==p2) return true; // If they're the same object, we can stop
   if(p1==NULL && p2!=NULL) return false;
   if(p1!=NULL && p2==NULL) return false;

   // Actually have two parsers
   if(p1->top == p2->top)
      return isRecombinable(p1->tail, p2->tail);
   else
      return false;
}

void ShiftReducer::reduce()
{
   // Greedily reduce if possible
   Uint imax = top.end;
   Uint imin = top.start;
   Uint size = top.end - top.start;
   ShiftReducer* prev = tail;
   Uint idepth = 1;
   while(prev != NULL && (allowNonITG || idepth==1) ){
      imax = max(prev->end(), imax);
      imin = min(prev->start(), imin);
      size += (prev->end() - prev->start());
      idepth++;
      if(size == (imax - imin)) {
         // Reduce
         top.start = imin;
         top.end = imax;
         tail = prev->tail;
         leftBound = prev->leftBound;
         rightBound = prev->rightBound;
         if(idepth>2) nonITGCount++;
         idepth = 1;
      }
      prev = prev->tail;
   }
}

bool ShiftReducer::usingSR(const CanoeConfig& c)
{
   if(c.distLimitITG) return true;
   
   const vector<string>& distortionModel(c.featureGroup("d")->args);
   for(Uint i=0; i<distortionModel.size(); i++)
   {
      string name, arg;
      DistortionModel::splitNameAndArg(distortionModel[i],name,arg);
      if(name.compare("back-hlex")==0) return true;      
   }

   return false;
}
