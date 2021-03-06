#ifndef __ug_ttrack_position_h
#define __ug_ttrack_position_h

#include <cassert>
#include "tpt_typedefs.h"

// A token position in a Ttrack, with a LESS functor for comparing token
// positions in whatever sorting order the underlying token type implies.  
//
// (c) 2007-2010 Ulrich Germann. All rights reserved.
// Licensed to NRC under special agreement.

namespace ugdiss
{
  namespace ttrack
  {
    /** Represents a position in a corpus (sentence Id + offset from beginning
     *  of sentence) */
    class
    Position
    {
    public:
      id_type sid;
      ushort  offset;
      Position();
      Position(id_type _sid, ushort _off);
      template<typename TTRACK_TYPE> class LESS; // probably abandoned
    }; // end of deklaration of Position
    
#if 1 
    template<typename TTRACK_TYPE>
    class 
    Position::
    LESS
    {
      TTRACK_TYPE const* c;
    public:
      typedef typename TTRACK_TYPE::Token Token;
      
      LESS(TTRACK_TYPE const* crp) : c(crp) {};
      
      bool operator()(Position const& A, Position const& B) const
      {
        Token const* bosA = c->sntStart(A.sid);
        Token const* eosA = c->sntEnd(A.sid);
        
        Token const* bosB = c->sntStart(B.sid);
        Token const* eosB = c->sntEnd(B.sid);
        
        Token const* a = c->getToken(A); assert(a);
        Token const* b = c->getToken(B); assert(b);
#if 0
        Token const* z = a; 
        cout << "A: " << z->id();
        for (z = next(z); z; z = next(z))
          cout << "-" << z->id(); 
        cout << endl;
        
        z = b; 
        cout << "B: " << z->id();
        for (z = next(z); z; z = next(z))
          cout << "-" << z->id(); 
        cout << endl;
#endif
        while (*a == *b)
          {
            a = next(a);
            b = next(b);
            if (a < bosA || a >= eosA) 
              return (b >= bosB && b < eosB);
            if (b < bosB || b >= eosB) 
                return false;
          }
        int x = a->cmp(*b);
        // cout << " " << (x < 0) << endl;
        assert (x != 0);
        return x < 0;
      }
    }; // end of definition of LESS
#endif
  } // end of namespace ttrack
} // end of namespace ugdiss
#endif
  
