/**
 * @author J Howard Johnson
 * @file nbesttranslation.h  This file produces the N-Best translations directly (using BGL algorithms).
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

#ifndef NBESTTRANSLATION_H
#define NBESTTRANSLATION_H

#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>
#include <boost/graph/graph_traits.hpp>
#include <boost/operators.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
   #include <boost/property_map/property_map.hpp>
#else
   #include <boost/property_map.hpp>
#endif
#include <file_utils.h>
#include "decoder.h"
#include "phrasedecoder_model.h"
#include "basicmodel.h"
#include "wordgraph.h"
#include "lattice_overlay_visitor.h"


//   How to use:
//   The calling environment needs to include this file and
//   include the object file from nbesttranslation.cc.  The
//   method of call is as follows.

//  #include "nbesttranslation.h"
//  lattice_overlay  theLatticeOverlay(
//                finalStates.begin(), finalStates.end(), model );
//  print_nbest( theLatticeOverlay, N );


namespace Portage {

typedef std::pair< DecoderState *, DecoderState * >  DecoderStatePair;
typedef vector< DecoderState * >::const_iterator  DecoderStateIterator;
typedef std::pair< DecoderStateIterator, DecoderStateIterator >
    DecoderStateIteratorPair;

//      a forward declaration
class lattice_overlay_out_edge_iterator;

// ============================= lattice_overlay =============================

/// Adaptor for Portage in memory lattices.
class lattice_overlay {

    DecoderStateIterator  initial_states_begin;
    DecoderStateIterator  initial_states_end;
    vector< DecoderState * >  expanded_initial_states;
    ///  model phrase which the states were created.
    PhraseDecoderModel &  model;
    DecoderState *  dummy_initial_state;

  public:

///      Vertex is a pointer to a DecoderState
    typedef DecoderState *  vertex_descriptor;
///      Edge is a pair of Vertices
    typedef DecoderStatePair  edge_descriptor;

//      Indicate that the graph is directed, supports the IncidenceGraph
//      concept and is without parallel edges
/// Indicates that the graph is directed.
    typedef boost::directed_tag  directed_category;
/// Indicates that the graph supports the IncidenceGraph concept.
    typedef boost::incidence_graph_tag  traversal_category;
/// Indicates that the graph is without parallel edges.
    typedef boost::disallow_parallel_edge_tag  edge_parallel_category;

///      IncidenceGraph requires an out_edge_iterator, degree_size_type
///      out_edges(v,g), source(e,g), target(e,g), and out_degree(v,g)
    typedef lattice_overlay_out_edge_iterator  out_edge_iterator;
    typedef int  degree_size_type;

//@{
///      IncidenceGraph requires that these have dummy declarations
    typedef void  adjacency_iterator;
    typedef void  in_edge_iterator;
    typedef void  vertex_iterator;
    typedef void  edge_iterator;
    typedef int  vertices_size_type;
    typedef int  edges_size_type;
//@}

/**
 * Constructor.
 * Lattice also has a set of initial_states. Provide them to the constructor.
 * @param initial_states_begin_parm  first initial state
 * @param initial_states_end_parm    last initial state
 * @param model_parm                 model from which the states were created.
 */
    lattice_overlay(
               DecoderStateIterator  initial_states_begin_parm,
               DecoderStateIterator  initial_states_end_parm,
               PhraseDecoderModel &  model_parm )
          : initial_states_begin( initial_states_begin_parm ),
            initial_states_end( initial_states_end_parm ),
            model( model_parm ) {
        dummy_initial_state = new DecoderState;
        Uint  max_id = 0;
        for ( DecoderStateIterator  vi = initial_states_begin;
              vi != initial_states_end;
              ++vi ) {
            expanded_initial_states.push_back( *vi );
            if ( max_id < ( * vi )-> id ) {
                max_id = ( * vi )-> id;
            }
            for ( DecoderStateIterator  rvi = ( *vi )-> recomb.begin();
                  rvi != ( *vi )-> recomb.end();
                  ++rvi ) {
                expanded_initial_states.push_back( *rvi );
                if ( max_id < ( * vi )-> id ) {
                    max_id = ( * vi )-> id;
                }
            }
        }
        initial_states_begin = expanded_initial_states.begin();
        initial_states_end   = expanded_initial_states.end();
        dummy_initial_state-> id = max_id + 1;
        dummy_initial_state-> trans = NULL;
        dummy_initial_state-> back = NULL;
        dummy_initial_state-> refCount = 0;
    }

    /// Destructor.
    ~lattice_overlay() {
        delete dummy_initial_state;
    }

//      prevent copy of lattice_overlay -- it is a bad thing
//      "if you write one of the three special functions (copy constructor,
//       copy assignment operator, or destructor), you will probably need to
//       write all three" (p. 154 C++ in a Nutshell)
  private:
    /// Deactivated copy constructor.
    lattice_overlay( lattice_overlay const & );
    /// Deactivated copy assignment operator
    lattice_overlay  operator=( lattice_overlay const & );
  public:

//@{
///      accessors for private data
    DecoderState *  get_initial_state() const {
        return  dummy_initial_state;
    }
    DecoderStateIterator  get_initial_states_begin() const {
        return  initial_states_begin;
    }
    DecoderStateIterator  get_initial_states_end() const {
        return  initial_states_end;
    }
    PhraseDecoderModel &  get_model() const {
        return  model;
    }
//@}

//@{
/// Verifies states is an initial state or an acceptor state.
    bool  is_initial_state( DecoderState * const  state ) const {
        return  state == dummy_initial_state;
    }
    bool  is_final_state( DecoderState * const  state ) const {
        return  state-> id == 0;
    }
//@}
};


// ==================== lattice_overlay_out_edge_iterator ====================

class lattice_overlay_out_edge_iterator
  :
    public boost::forward_iterator_helper
      <
        lattice_overlay_out_edge_iterator,
        DecoderState *
  > {

    DecoderState *  from_state;
    DecoderState *  to_state;
    DecoderStateIterator  state_iterator;
    enum state_type {
        use__to_state,
        use__iterator
    };
    state_type  state_finite_memory;

  public:

    /// Constructor.
    lattice_overlay_out_edge_iterator() { }

    /// Copy constructor.
    /// @param ei  edge iterator we want to copy.
    lattice_overlay_out_edge_iterator(
        lattice_overlay_out_edge_iterator const &  ei
      ) {
        from_state          = ei.from_state;
        to_state            = ei.to_state;
        state_iterator      = ei.state_iterator;
        state_finite_memory = ei.state_finite_memory;
    }

    /// Copy assignment operator.
    /// @param ei  edge iterator we want to copy.
    lattice_overlay_out_edge_iterator &  operator=(
        lattice_overlay_out_edge_iterator const &  ei
      ) {
        from_state          = ei.from_state;
        to_state            = ei.to_state;
        state_iterator      = ei.state_iterator;
        state_finite_memory = ei.state_finite_memory;
        return *this;
    }

    /**
     * Swaps two edge iterator.
     * @param ei_1  left-hand side operand.
     * @param ei_2  right-hand side operand.
     */
    void  swap(
        lattice_overlay_out_edge_iterator & ei_1,
        lattice_overlay_out_edge_iterator & ei_2
      ) {
        std::swap( ei_1.from_state, ei_2.from_state );
        std::swap( ei_1.to_state, ei_2.to_state );
        std::swap( ei_1.state_iterator, ei_2.state_iterator );
        std::swap( ei_1.state_finite_memory, ei_2.state_finite_memory );
    }

    bool  operator==(
        lattice_overlay_out_edge_iterator const &  ei
      ) const {
        return (    from_state          == ei.from_state
                 && to_state            == ei.to_state
                 && state_iterator      == ei.state_iterator
                 && state_finite_memory == ei.state_finite_memory );
    }

    DecoderStatePair  operator*() {
        switch( state_finite_memory ) {
        case use__to_state:
            return DecoderStatePair( from_state, to_state );
        default:
        case use__iterator:
            return DecoderStatePair( from_state, *state_iterator );
        }
    }

    lattice_overlay_out_edge_iterator &  operator++(
      ) {
        switch( state_finite_memory ) {
        case use__to_state:
            state_finite_memory = use__iterator;
            break;
        default:
        case use__iterator:
            ++state_iterator;
            break;
        }
        return *this;
    }

    lattice_overlay_out_edge_iterator(
        DecoderState * const &  state,
        lattice_overlay const &  g
      ) {
        from_state =  state;
        to_state =  state;
        if ( g.is_initial_state( state ) ) {
            state_finite_memory = use__iterator;
            state_iterator = g.get_initial_states_begin();
        } else if ( state-> back ) {
            state_finite_memory = use__to_state;
            to_state = state-> back;
            state_iterator = state-> back-> recomb.begin();
        } else {
            state_finite_memory = use__iterator;
            state_iterator = g.get_initial_states_end();
        }
    }

    /**
     * Constructor.
     * @param state
     * @param g
     * @param construct_end_iterator  acts as a flag to differentiate between both constructors.
     */
    lattice_overlay_out_edge_iterator(
        DecoderState * const &  state,
        lattice_overlay const &  g,
        int  construct_end_iterator
      ) {
        from_state =  state;
        to_state =  state;
        if ( g.is_initial_state( state ) ) {
            state_finite_memory = use__iterator;
            state_iterator = g.get_initial_states_end();
        } else if ( state-> back ) {
            state_finite_memory = use__iterator;
            to_state = state-> back;
            state_iterator = state-> back-> recomb.end();
        } else {
            state_finite_memory = use__iterator;
            state_iterator = g.get_initial_states_end();
        }
    }

};


// ============================ service functions ============================

/**
 * Helps create a pair of lattice_overlay_out_edge_iterator
 * @relates lattice_overlay_out_edge_iterator
 * @param v
 * @param g
 */
inline std::pair<
        lattice_overlay_out_edge_iterator,
        lattice_overlay_out_edge_iterator
      >
        out_edges(
          DecoderState * const &  v, lattice_overlay const &  g
  ) {
    return std::pair<
        lattice_overlay_out_edge_iterator,
        lattice_overlay_out_edge_iterator
      > (
        lattice_overlay_out_edge_iterator( v, g ),
        lattice_overlay_out_edge_iterator( v, g, 1 )
    );
}

inline  DecoderState *  source(
        DecoderStatePair &  e, lattice_overlay const &  g ) {
    return e.first;
}

inline  DecoderState *  target(
        DecoderStatePair &  e, lattice_overlay const &  g ) {
    return e.second;
}

inline  lattice_overlay::degree_size_type  out_degree(
        DecoderState *  v,
        lattice_overlay const &  g
      ) {
        if ( g.is_initial_state( v ) ) {
            return g.get_initial_states_end() - g.get_initial_states_begin();
        } else if ( v-> back == NULL ) {
            return 0;
        } else {
            return 1 + v-> back-> recomb.size();
        }
}


// ============================== my_weight_map_t ==============================

class my_weight_map_t {
  public:
    typedef  DecoderStatePair  key_type;
    typedef  double  value_type;
    typedef  value_type &  reference;
    typedef  boost::readable_property_map_tag  category;
    my_weight_map_t( lattice_overlay const &  g_parm ) : g( g_parm ) { }

    lattice_overlay const &  g;
};

inline double  get( my_weight_map_t  wm, DecoderStatePair  e ) {
    DecoderState *  v1( source( e, wm.g ) );
    if ( v1-> back ) {
        return  v1-> score - v1-> back-> score;
    } else {
        return  0.0;
    }
}


// ============================== my_phrase_map_t ==============================

class my_phrase_map_t {
  public:
    typedef  DecoderStatePair  key_type;
    typedef  std::string  value_type;
    typedef  value_type &  reference;
    typedef  boost::readable_property_map_tag  category;
    my_phrase_map_t( lattice_overlay const &  g_parm )
      : g( g_parm ) { }

    lattice_overlay const &  g;
};

inline std::string  get( my_phrase_map_t  pm, DecoderStatePair  e ) {
    DecoderState *  v1( source( e, pm.g ) );
    string  result;
    if ( v1-> trans ) {
        pm.g.get_model().getStringPhrase(
            result, v1-> trans-> lastPhrase-> phrase );
        return result;
    } else {
        return "";
    }
}


// ============================== my_ffvals_map_t ==============================

class my_ffvals_map_t {
  public:
    typedef  lattice_overlay::edge_descriptor  key_type;
    typedef  vector< double >  value_type;
    typedef  value_type &  reference;
    typedef  boost::readable_property_map_tag  category;
    my_ffvals_map_t( lattice_overlay const &  g_parm )
      : g( g_parm ) { }

    lattice_overlay const &  g;
};

inline vector< double >  get( my_ffvals_map_t  fm, DecoderStatePair  e ) {
    DecoderState *  v1( source( e, fm.g ) );
    vector< double >  ffvals;
    if ( v1-> trans ) {
        ( ( BasicModel & )
            fm.g.get_model() ).getFeatureFunctionVals( ffvals, *v1-> trans );
    }
    return ffvals;
}


/**
 * Prints the NBest List for the lattice.
 * @param g          the lattice itself.
 * @param n          number of best hypotheses (n=100 => 100 best list).
 * @param print      a functor to format the content of the NBest List
 *                   and print it.
 * @param backwards  
 */
void  print_nbest( lattice_overlay const &  g,
          const int  n, NbestPrinter &  print, bool  backwards );

/**
 * Prints the entire lattice.
 * @param g      the lattice itself.
 * @param file   output stream to print the lattice.
 * @param print  a functor to format the content of the lattice's nodes.
 */
void print_lattice( lattice_overlay const & g,
                    ostream& file,
                    PrintFunc & print);

/**
 * Prints the entire lattice.
 * @param g         the lattice itself.
 * @param filename  a file to print the lattice.
 * @param print     a functor to format the content of the lattice's nodes.
 */
inline
void print_lattice( lattice_overlay const & g,
                    const std::string& filename,
                    PrintFunc & print)
{
   oSafeMagicStream file( filename );
   print_lattice(g, file, print);
}

} // Portage

#endif // NBESTTRANSLATION_H
