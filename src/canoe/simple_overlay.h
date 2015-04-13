/**
 * @author Colin Cherry
 * @file simple_overlay.h  View a search graph as a lattice
 *
 * Canoe Decoder
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#ifndef SIMPLEOVERLAY_H
#define SIMPLEOVERLAY_H

#include <map>

#include "decoder.h"
#include "phrasedecoder_model.h"
#include "wordgraph.h"
#include <boost/operators.hpp>

namespace Portage {

   class LatticeEdges;
   class SimpleOutEdgeIterator;
   typedef vector<DecoderState *>::const_iterator  DecoderStateIterator;
   typedef vector<DecoderState *>::const_reverse_iterator RDecoderStateIterator;

   /**
    * An edge in the lattice.
    * Abstracts underlying decoder states into three pointers:
    * 
    * From: The recombined original state ("prime state")
    * To:   The original back-pointer
    * Info: The un-recombined state, contains all information about the edge
    *       from #from to #to.
    * Note that we use parent->child terminology throughout this code, in analog
    * to the java HyperGraph structure from which most of it is adapted.
    * From is the parent, To is the child.
    * Thus, FINAL is the root, and all true DecoderStates are its descendants
    */
   class LatticeEdge {
   private:
      DecoderState* m_from;
      DecoderState* m_to;
      DecoderState* m_info;

      static double m_min_score;
      
   public:
      /**
       * Constructor
       * @param from Prime state
       * @param to   Back-pointer
       * @param info Non-prime state that originally pointed to #to
       *             NULL if #from is the dummy root
       */
      LatticeEdge(DecoderState* from, DecoderState* to, DecoderState* info)
      {
         m_from = from;
         m_to = to;
         m_info = info;
      }
      
      // Accessors
      DecoderState* from() const { return m_from; }
      DecoderState* to() const {return m_to; }
      DecoderState* info() const {return m_info; }

      /**
       * @return The log probability, or cost of this edge
       */
      double score() const;

      /**
       * @return True if #from is the dummy root
       */
      bool isInitial() const {return m_info==0;}

      /**
       * @return Node id, or FINAL for dummy root
       */
      std::string fromName() const;

      /**
       * @return Unique edge identifier, not necessarily sequential
       */
      Uint id() const;

      /**
       * Set the minscore value
       */
      static void setMinScore(double d) {m_min_score = d;}
   };

   /**
    * The lattice overlay. Can answer questions about the lattice in general,
    * enumerate its nodes and provide edge lists for each node.
    */
   class SimpleOverlay {
      DecoderStateIterator initial_states_begin;
      DecoderStateIterator initial_states_end;
      PhraseDecoderModel& model;
      DecoderState* dummy_initial_state;
      vector<DecoderState*>* states_inside_order;
      bool lattice_log_prob;
      
   public:
      /**
       * Constructor
       * @param initial_states_begin_parm Begin iterator for list of initial states
       * @param initial_states_end_parm   End iterator for list of initial
       * @param model_parm                Model that produced the lattice, not currently used
       */
      SimpleOverlay(
               DecoderStateIterator  initial_states_begin_parm,
               DecoderStateIterator  initial_states_end_parm,
               PhraseDecoderModel &  model_parm,
               bool latticeLogProb)
          : initial_states_begin( initial_states_begin_parm ),
            initial_states_end( initial_states_end_parm ),
            model( model_parm ),
            lattice_log_prob(latticeLogProb)
      {
        dummy_initial_state = new DecoderState;
        Uint max_id = 0;
        for ( DecoderStateIterator  vi = initial_states_begin;
              vi != initial_states_end;
              ++vi )
        {
           if ( max_id < (*vi)-> id ) {
              max_id = (*vi)-> id;
           }
           // max_id needs to be over both initial states and their
           // equivalents
           for( DecoderStateIterator ri = (*vi)->recomb.begin();
                ri != (*vi)->recomb.end(); ++ri)
           {
              if ( max_id < (* ri)-> id ) {
                 max_id = (*ri)-> id;
              }
           }
        }
        dummy_initial_state-> id = max_id+1;
        dummy_initial_state-> trans = NULL;
        dummy_initial_state-> back = NULL;
        dummy_initial_state-> refCount = 0;
        states_inside_order = NULL;
      }

      /**
       * @return Naive estimate of number of states based on state ids
       */
      Uint numStates() const {return dummy_initial_state->id+1;}
      
      /**
       * Destructor
       */
      ~SimpleOverlay() {
         delete dummy_initial_state;
         if(states_inside_order!=NULL) delete states_inside_order;
      }

   private:
      /// Deactivated copy constructor.
      SimpleOverlay( SimpleOverlay const & );
      /// Deactivated copy assignment operator
      SimpleOverlay  operator=( SimpleOverlay const & );

      //
      // Utility
      //
  
      // Helper for various print functions
      void insertEscapes(std::string &str, const char *charsToEscape = "\"\\", char escapeChar = '\\');
      // Helper to build the inside-order list of nodes lazily
      void buildInsideVector();
      // Generic function to print only edges that satisfy a boolean predicate (pred)
      template<typename Pred>
      Uint print_lattice( ostream& file, PrintFunc & print, Pred pred);

      //
      // Viterbi inside-outside, good for pruning, n-best list construction
      //
      
      /**
       * Viterbi inside. Find best score up to each node
       * @return Map from node id to log inside score
       */
      map<Uint,double> inside();

      /**
       * Viterbi outside. Find best completion score following this node
       * @param inside Output from the inside algorithm
       * @return Map from node id to log outside score
       */
      map<Uint,double> outside(const map<Uint,double>& inside);
      
      /**
       * Viterbi inside-outside. For each edge, find the best scoring path
       * that includes that edge. Calls inside and outside.
       * @return Map from edge id to log inside-outside score
       */
      map<Uint,double> insideOutside();

   public:

      // Accessors
      DecoderState *  get_initial_state() const { return  dummy_initial_state; }
      DecoderStateIterator get_initial_states_begin() const {return initial_states_begin;}
      DecoderStateIterator get_initial_states_end() const {return initial_states_end;}
      PhraseDecoderModel &  get_model() const { return  model; }

      /**
       * Is the input state the root?
       * @param state State to be tested
       * @return true if its the root
       */
      bool  is_initial_state( DecoderState * const  state ) const {
         return  state == dummy_initial_state;
      }

      /**
       * Is the input state "final"?
       * @param state State to be tested
       * @return true if it is node 0, which should be the only node with no back-pointers
       */
      bool  is_final_state( DecoderState * const  state ) const {
         return  state-> id == 0;
      }

      /**
       * Get the edges from an input state
       * @param v State to use as from
       * @return Iterable of outgoing edges
       */
      LatticeEdges getEdges(DecoderState * const & v);

      /**
       * @return Begin iterator to enumerate the nodes of the lattice in inside order
       */
      DecoderStateIterator statesInOrderBegin();

      /**
       * @return End iterator to enumerate the nodes of the lattice in inside order
       */
      DecoderStateIterator statesInOrderEnd();

      /**
       * @return Begin iterator to enumerate the nodes of the lattice in outside order
       */
      RDecoderStateIterator statesOutOrderBegin();

      /**
       * @return End iterator to enumerate the nodes of the lattice in outside order
       */
      RDecoderStateIterator statesOutOrderEnd();

      /**
       * Print the lattice to a file
       * @param  file  Output stream to print to
       * @param  print PrintFunc specifies what edge attributes to print
       * @return Number of edges printed
       */
      Uint print_lattice( ostream& file, PrintFunc & print);

      /**
       * Print a pruned lattice to a stream
       * @param file      Output stream
       * @param print     PrintFunc object specifies what edge attributes to print
       * @param density   Prune the lattice down to #density x #tgtlen edges
       * @param tgtlen    Calculate density using this number of tokens,
       *                  sane choices are len of the Vertbi translation or source len
       * @param verbosity Higher numbers = more debugging info
       * @return          Number of edges printed
       */
      Uint print_pruned_lattice(ostream& file, PrintFunc & print, double density, Uint tgtlen, Uint verbosity);

      /**
       * @return The length of the source sentence for this lattice
       */
      Uint sourceLen();
   };

   /**
    * Here is where the magic happens. This OutEdgeIterator is where we hide all
    * the ugliness of state recombination.
    *
    * Iterator structure copied from lattice_overlay, but the interaction with the
    * underlying search graph is completely different, and results in much more compact
    * lattices
    */
   class SimpleOutEdgeIterator
      : public boost::forward_iterator_helper<SimpleOutEdgeIterator,DecoderState*>
   {
      DecoderState *  from_state;
      DecoderStateIterator  state_iterator;
      enum state_type {
         use__state,
         use__iterator
      };
      state_type  state_finite_memory;
      bool is_initial;

   public:

      // Empty constructor
      SimpleOutEdgeIterator(){}

      /// Copy constructor.
      /// @param ei  edge iterator we want to copy.
      SimpleOutEdgeIterator(SimpleOutEdgeIterator const &  ei);

      /// Copy assignment operator.
      /// @param ei  edge iterator we want to copy.
      SimpleOutEdgeIterator &  operator=(SimpleOutEdgeIterator const &  ei);

      /**
       * Swaps two edge iterator.
       * @param ei_1  left-hand side operand.
       * @param ei_2  right-hand side operand.
       */
      void  swap(SimpleOutEdgeIterator & ei_1, SimpleOutEdgeIterator & ei_2);

      bool  operator==(SimpleOutEdgeIterator const &  ei) const;
      
      LatticeEdge  operator*();

      SimpleOutEdgeIterator &  operator++();

      /**
       * Construct a Begin iterator
       * @param state State from which to build each edge
       * @param g     The overlay representing the entire lattice
       */
      SimpleOutEdgeIterator(DecoderState * const &  state, SimpleOverlay const &  g);

      /**
       * Construct an End iterator.
       * @param state State from which to build each edge
       * @param g     The overlay representing the entire lattice
       * @param construct_end_iterator  Acts as a flag to differentiate between both constructors.
       */
      SimpleOutEdgeIterator(DecoderState * const &  state, SimpleOverlay const &  g, int  construct_end_iterator);
   };

   /**
    * Hey look! It's a begin iterator and an end iterator living together, but it's not a container
    * I miss Java's Iterable
    * This is how you should interact with SimpleOutEdgeIterator, rather than calling the Iterator
    * constructor directly. See print_lattice or buildInsideVector for examples
    */
   class LatticeEdges {
      SimpleOutEdgeIterator m_begin;
      SimpleOutEdgeIterator m_end;
   public:
      LatticeEdges(DecoderState * const & state, SimpleOverlay const & g) :
         m_begin(state,g), m_end(state,g,1) {}
      
      SimpleOutEdgeIterator begin() {return m_begin;}
      const SimpleOutEdgeIterator& end() {return m_end;}
   };
}   
#endif // SIMPLEOVERLAY_H
