/**
 * @author J Howard Johnson
 * @file nbesttranslation.cc  This file produces the N-Best translations
 * directly (using BGL algorithms).
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004-2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004-2005, Her Majesty in Right of Canada
 */

#include "nbesttranslation.h"
#include "decoder.h"
#include "phrasedecoder_model.h"
#include "basicmodel.h"
#include <vector>
#include <deque>
#include <queue>
#include <iostream>
#include <fstream>

using namespace Portage;
using namespace std;

/// Needs description
typedef pair< Uint, pair< DecoderState *, double > >  state_weight_t;


// ============================ service functions ============================
/**
 * Easy way to create state_weight_t.
 * @param state  the state
 * @param uid    state's id
 * @param prob   state's probability
 * @return Returns
 */
inline state_weight_t make_state_weight_t(DecoderState* state, Uint uid, double prob)
{
   return make_pair(uid, make_pair(state, prob));
}
/**
 * Easy DecoderState extraction from state_weight_t.
 * @param sw  State weight we want to extract the DecoderState.
 * @return Returns the DecoderState contained in astate_weight_t.
 */
inline DecoderState*  getState(const state_weight_t& sw) {
   return sw.second.first;
}
/**
 * Easy Probability extraction from state_weight_t.
 * @param sw  State weight we want to extract the Probability.
 * @return Returns the DecoderState contained in astate_weight_t.
 */
inline double getProb(const state_weight_t& sw) {
   return sw.second.second;
}


namespace Portage {
/// Callable entity to order states in the priority_queue in decreasing order of scores.
class state_weight_t_compare {
public:
    /**
     * Equivalent to the less operator for state_weight_t.
     * @param p_c__1  left-hand side operand
     * @param p_c__2  right-hand side operand
     * @return Returns true p_c__1 has the best probability score
     */
    bool  operator() ( const state_weight_t&  p_c__1, const state_weight_t&  p_c__2 ) {
        return   getProb(p_c__1) + getState(p_c__1)-> score
               < getProb(p_c__2) + getState(p_c__2)-> score;
    }
};
} // ends namespace Portage


/// Needs description
typedef map< state_weight_t, state_weight_t >  Paths;


/**
 * Once a translation has been found, this function transforms a set of paths
 * composing the translation into a string and escapes the quotes and slashes.
 * @param print
 * @param backwards
 * @param p_c
 * @param pi
 * @param g          the lattice itself.
 */
static void print_path(
    NbestPrinter &  print,
    bool  backwards,
    state_weight_t  p_c,
    Paths &  pi,
    lattice_overlay const &  g
  ) {
    typedef deque< DecoderState * > Sequence;
    typedef Sequence::const_iterator SequenceIterator;

    p_c = pi[ p_c ];
    DecoderState *        p_1 = getState(p_c);
    DecoderState * const  src = g.get_initial_state();
    Sequence  state_sequence;
    while ( p_1 != src ) {
       if ( backwards ) {
          state_sequence.push_front( p_1 );
       } else {
          state_sequence.push_back( p_1 );
       }
       p_c = pi[ p_c ];
       p_1 = getState(p_c);
    }
    for ( SequenceIterator vi = state_sequence.begin();
          vi != state_sequence.end();
          ++vi ) {
       print( *vi );
    }
    print.sentenceEnd();
}

void Portage::print_nbest( lattice_overlay const &  g,
                  const int n, NbestPrinter & print, bool  backwards
  ) {
    static Uint Uid_maker(0);
    map< DecoderState *, int >  r;
    Paths pi;
    priority_queue<
        state_weight_t,
        vector< state_weight_t >,
        state_weight_t_compare
    >  S;
    my_weight_map_t  w( g );
    DecoderState * const  src = g.get_initial_state();
    DecoderState *        sink(NULL);
    S.push( make_state_weight_t( src, ++Uid_maker, (double) 0.0 ) );
    while ( ! S.empty() ) {
        state_weight_t  p_c = S.top();
        S.pop();
        DecoderState *  p = getState(p_c);
        const double    c = getProb(p_c);
        const int     r_p = ++r[ p ];
        if ( g.is_final_state( p ) ) {
            sink = p;
            print_path( print, backwards, p_c, pi, g );
            if ( r_p == n ) {
                break;
            }
        }
        if ( r_p <= n ) {
            lattice_overlay_out_edge_iterator  ei, ei_end;
            for ( tie( ei, ei_end ) = out_edges( p, g ); ei != ei_end; ++ei ) {
                DecoderStatePair  e = *ei;
                const double  c_prime = c + get( w, e );
                state_weight_t  n_e__c_prime =
                    make_state_weight_t( e.second, ++Uid_maker, c_prime );
                pi[ n_e__c_prime ] = p_c;
                S.push( n_e__c_prime );
            }
        }
    }
    while ( r[ sink ] < n ) {
        ++r[ sink ];
        print.sentenceEnd();
    }
}


void Portage::print_lattice( lattice_overlay const & g,
                    ostream& file,
                    PrintFunc & print)
{
   cerr << "Entering print_lattice" << endl;
   DecoderState* src = g.get_initial_state();

   deque<DecoderStatePair> myStack;
   myStack.push_back(make_pair(src, src));

   file << "FINAL" << endl;
   for (DecoderStateIterator it = g.get_initial_states_begin();
         it != g.get_initial_states_end();
         ++it)
   {
      file << '(' << (*it)->id << " (" << "FINAL" << " \"\" " << 1 << "))" << endl;
      for (DecoderStateIterator it2 = (*it)->recomb.begin();
            it2 != (*it)->recomb.end();
            ++it2)
      {
         file << '(' << (*it2)->id << " (" << "FINAL" << " \"\" " << 1 << "))" << endl;
      }
   }

   while (!myStack.empty()) {
      DecoderStatePair pair = myStack.front();
      myStack.pop_front();
      DecoderState* p = pair.second;

      if (pair.first->back != 0) {
         const double score = exp(pair.first->score - pair.first->back->score);
         file << '(' << pair.second->id << " (" << pair.first->id << " \"" << print(pair.first) << "\" " << score << "))" << endl;
      }

      lattice_overlay_out_edge_iterator  ei, ei_end;
      for ( tie( ei, ei_end ) = out_edges( p, g ); ei != ei_end; ++ei ) {
         DecoderStatePair  e = *ei;
         DecoderState* s = e.second;
         myStack.push_back(make_pair(p, s));
      }
   }
}
