/**
 * @author Colin Cherry
 * @file simple_overlay.cc  This file contains the implementation of the
 * SimpleOverlay object, a refined and compact lattice overlay for the
 * search graph output by Portage
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <stack>
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>

#include "canoe_general.h"
#include "simple_overlay.h"

double LatticeEdge::m_min_score=SMALL_MIN_EDGE;

double LatticeEdge::score() const {
   if(m_info!=0) {
      double toRet = m_info->score - m_info->back->score;
      if(toRet>m_min_score) return toRet;
      else return m_min_score;
   }
   else {
      return 0.0;
   }
}

std::string LatticeEdge::fromName() const {
   if( this->isInitial() )
      return "FINAL";
   else 
      return lexical_cast<std::string>(this->from()->id);
}

Uint LatticeEdge::id() const {
   if(m_info!=0)
      return m_info->id;
   else
      return m_from->id+m_to->id; // Not compact, but unique
}

/// Copy constructor.
/// @param ei  edge iterator we want to copy.
SimpleOutEdgeIterator::
SimpleOutEdgeIterator(SimpleOutEdgeIterator const &  ei)
{
   from_state          = ei.from_state;
   state_iterator      = ei.state_iterator;
   state_finite_memory = ei.state_finite_memory;
   is_initial          = ei.is_initial;
}

/// Copy assignment operator.
/// @param ei  edge iterator we want to copy.
SimpleOutEdgeIterator &  
SimpleOutEdgeIterator::operator=(SimpleOutEdgeIterator const &  ei)
{
   from_state          = ei.from_state;
   state_iterator      = ei.state_iterator;
   state_finite_memory = ei.state_finite_memory;
   is_initial          = ei.is_initial;
   return *this;
}

/**
 * Swaps two edge iterator.
 * @param ei_1  left-hand side operand.
 * @param ei_2  right-hand side operand.
 */
void  SimpleOutEdgeIterator::swap(SimpleOutEdgeIterator & ei_1, SimpleOutEdgeIterator & ei_2)
{
   std::swap( ei_1.from_state, ei_2.from_state );
   std::swap( ei_1.state_iterator, ei_2.state_iterator );
   std::swap( ei_1.state_finite_memory, ei_2.state_finite_memory );
   std::swap( ei_1.is_initial,ei_2.is_initial );
}

bool SimpleOutEdgeIterator::operator==(SimpleOutEdgeIterator const &  ei) const
{
   return (    from_state             == ei.from_state
               && state_iterator      == ei.state_iterator
               && state_finite_memory == ei.state_finite_memory
               && is_initial == ei.is_initial);
}
      
LatticeEdge SimpleOutEdgeIterator::operator*()
{
   switch( state_finite_memory ) {
   case use__state:
      assert(!is_initial);
      return LatticeEdge( from_state, from_state->back, from_state );
   default:
   case use__iterator:
      if(is_initial)
         return LatticeEdge( from_state, *state_iterator, NULL );
      else
         return LatticeEdge( from_state, (*state_iterator)->back, *state_iterator );
   }
}

SimpleOutEdgeIterator& SimpleOutEdgeIterator::operator++()
{
   switch( state_finite_memory ) {
   case use__state:
      state_finite_memory = use__iterator;
      break;
   default:
   case use__iterator:
      ++state_iterator;
      break;
   }
   return *this;
}

SimpleOutEdgeIterator::
SimpleOutEdgeIterator(DecoderState * const &  state, SimpleOverlay const &  g)
{
   from_state =  state;
   is_initial = false;
   if ( g.is_initial_state( state ) ) {
      state_finite_memory = use__iterator;
      state_iterator = g.get_initial_states_begin();
      is_initial = true;
   } else if ( state!=0 && state-> back ) {
      state_finite_memory = use__state;
      state_iterator = state->recomb.begin();
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
SimpleOutEdgeIterator::
SimpleOutEdgeIterator(
                      DecoderState * const &  state,
                      SimpleOverlay const &  g,
                      int  construct_end_iterator
                      ) {
   from_state =  state;
   is_initial = false;
   if ( g.is_initial_state( state ) ) {
      state_finite_memory = use__iterator;
      state_iterator = g.get_initial_states_end();
      is_initial = true;
   } else if ( state!=0 && state-> back ) {
      state_finite_memory = use__iterator;
      state_iterator = state->recomb.end();
   } else {
      state_finite_memory = use__iterator;
      state_iterator = g.get_initial_states_end();
   }
}


/**
 * Helps create a pair of SimpleOutEdgeIterator
 * @relates SimpleOutEdgeIterator
 * @param v
 */
LatticeEdges SimpleOverlay::getEdges(DecoderState * const &  v)
{
   return LatticeEdges(v,*this);
}

// Stolen from wordgraph.cc
void
SimpleOverlay::
insertEscapes(std::string &str, const char *charsToEscape, char escapeChar)
{
   for ( std::string::size_type find = str.find_first_of(charsToEscape, 0); find != std::string::npos;
         find = str.find_first_of(charsToEscape, find + 2))
   {
      str.insert(find, 1, escapeChar);
   } // for
} // insertEscapes

// Always true predicate, for when we want to print the entire lattice
struct alwaysTrue : public std::unary_function<const LatticeEdge&,bool>
{
   bool operator () (const LatticeEdge& value) {return true;}
};

// Predicate to compare an edge's inside-outside score to some pre-determined threshold
struct inOutThreshold : public std::unary_function<const LatticeEdge&,bool>
{
private:
   const double threshold;
   const map<Uint,double>& inOutScores;
public:
   inOutThreshold(const map<Uint,double>& inOut,double thresh)
      :threshold(thresh), inOutScores(inOut){}
   bool operator () (const LatticeEdge& value) {
      map<Uint,double>::const_iterator it = inOutScores.find(value.id());
      assert (it!=inOutScores.end());
      return it->second + 1e-12 >= threshold;
   }
};

template<typename Pred>
Uint
SimpleOverlay::
print_lattice( ostream& file, PrintFunc & print, Pred pred)
{
   Uint printed=0;
   file << "FINAL" << endl;
   for(DecoderStateIterator it=statesInOrderBegin();it!=statesInOrderEnd();++it)
   {
      DecoderState* cur = *it;

      // For each outgoing edge...
      LatticeEdges edges(cur,*this);
      for ( SimpleOutEdgeIterator ei = edges.begin(); ei != edges.end(); ++ei ) {
         const LatticeEdge& edge = *ei;
         // Check predicate
         if(pred(edge)) {
            // Print edge
            std::string phr = print(edge.info());
            insertEscapes(phr);
            double score = edge.score();
            if(!lattice_log_prob) score = exp(edge.score());
            file << '(' << edge.to()->id << " (" << edge.fromName()
                 << " \"" << phr << "\" " << score << "))" << endl;
            printed++;
         }
      }
   }
   return printed;
}

Uint SimpleOverlay::print_lattice( ostream& file, PrintFunc & print )
{
   return print_lattice(file,print,alwaysTrue());
}

DecoderStateIterator SimpleOverlay::statesInOrderBegin()
{
   buildInsideVector(); // Lazy initialization
   return states_inside_order->begin();
}

DecoderStateIterator SimpleOverlay::statesInOrderEnd()
{
   buildInsideVector(); // Lazy initialization
   return states_inside_order->end();
}

RDecoderStateIterator SimpleOverlay::statesOutOrderBegin()
{
   buildInsideVector(); // Lazy initialization
   return states_inside_order->rbegin();
}

RDecoderStateIterator SimpleOverlay::statesOutOrderEnd()
{
   buildInsideVector(); // Lazy initialization
   return states_inside_order->rend();
}

/**
 * Create a list of nodes in their inside (forward) order.
 * Good for dynamic programming.
 * Reverse this order to get outside (backward) order.
 */
void SimpleOverlay::buildInsideVector()
{
   if(states_inside_order==NULL)
   {
      states_inside_order = new vector<DecoderState*>();
      boost::dynamic_bitset<> reached(numStates());   // True after the first time we hit a node
      boost::dynamic_bitset<> completed(numStates()); // True once this node & all its children have been covered
      std::stack<DecoderState*> myStack;

      myStack.push(get_initial_state());
      while(!myStack.empty()) {

         DecoderState* cur = myStack.top();

         if(completed[cur->id]) {
            // Only want to emit each node once
            myStack.pop();
         }
         else {
            if(reached[cur->id]) {
               // If this is the second time we've hit this node, it's safe to emit
               myStack.pop();
               completed[cur->id]=true;
               states_inside_order->push_back(cur);
            }
            else {
               // The first time we hit a node, push all children to make sure they're covered
               reached[cur->id]=true;
               LatticeEdges edges(cur,*this);
               for ( SimpleOutEdgeIterator ei = edges.begin(); ei != edges.end(); ++ei ) {
                  const LatticeEdge& edge = *ei;
                  if(!completed[edge.to()->id]) {
                     assert(!reached[edge.to()->id]); // Check for cycles
                     myStack.push(edge.to());
                  }
               }
            }
         }
      }
   }
}

/**
 * @return The length of the source sentence for this lattice
 */
Uint SimpleOverlay::sourceLen()
{
   DecoderState* final = *initial_states_begin;
   return final->trans->numSourceWordsCovered;
}

/**
 * Viterbi inside. Find best score up to each node
 * @return Map from node id to log inside scores
 */
map<Uint,double> SimpleOverlay::inside()
{
   map<Uint,double> in; // To return
   boost::dynamic_bitset<> visited(numStates()); // To double-check inside order
   
   // For each state in inside order
   for(DecoderStateIterator it=statesInOrderBegin();it!=statesInOrderEnd();++it)
   {
      DecoderState* cur = *it;
      double stateScore = 0; // So an edge with no children gets 0 score (highest possible)
      bool first = true;
      
      // For each outgoing edge...
      LatticeEdges edges(cur,*this);
      for ( SimpleOutEdgeIterator ei = edges.begin(); ei != edges.end(); ++ei ) {
         const LatticeEdge& edge = *ei;
         assert( visited[edge.to()->id] ); // Need to have visited children first
         // Push score up edge from children
         double edgeScore = edge.score() + in[edge.to()->id];
         if( first || edgeScore>stateScore ) {
            first = false;
            stateScore = edgeScore;
         }
      }
      in[cur->id]=stateScore;
      visited[cur->id]=true;
   }
   return in;
}

/**
 * Viterbi outside. Find best completion score following this node
 * @param inside Output from the inside algorithm
 * @return       Map from node id to outside score
 */
map<Uint,double> SimpleOverlay::outside(const map<Uint,double>& in)
{
   map<Uint,double> out; // To return
   boost::dynamic_bitset<> visited(numStates()); // To double-check outside order

   // Initialize
   out[get_initial_state()->id]=0;
   
   // For each state in outside order
   for(RDecoderStateIterator it=statesOutOrderBegin();it!=statesOutOrderEnd();++it)
   {
      DecoderState* cur = *it;
      double stateScore = out[cur->id];
      // For each outgoing edge...
      LatticeEdges edges(cur,*this);
      for ( SimpleOutEdgeIterator ei = edges.begin(); ei != edges.end(); ++ei ) {
         const LatticeEdge& edge = *ei;
         assert ( !visited[edge.to()->id] ); // Must not have visited children first
         // Push score down edge to children
         double edgeScore = edge.score() + stateScore;
         map<Uint,double>::iterator it=out.find(edge.to()->id);
         if(it==out.end() || edgeScore > it->second)
            out[edge.to()->id]=edgeScore;
      }
      visited[cur->id]=true;
   }
   return out;
}
      
/**
 * Viterbi inside-outside. For each edge, find the score of the
 * best scoring path that includes that edge.
 * Calls inside and outside.
 * @return Map from edge id to inside-outside score
 */
map<Uint,double> SimpleOverlay::insideOutside()
{
   // Run inside and outside
   map<Uint,double> in=inside();
   map<Uint,double> out=outside(in);
   const double maxScore = in[get_initial_state()->id];
   // Inside of root should be outside of leaf
   if ( ! (abs(out[0]-maxScore) < 1e-8*(maxScore?abs(maxScore):1)) )
      error(ETFatal, "SimpleOverlay::insideOutside out[0]=%.16g maxScore=%.16g delta=%.16g is larger than 1e-8 relative tolerance, or %.16g\n",
            out[0], maxScore, out[0]-maxScore, 1e-8*(maxScore?abs(maxScore):1));
   map<Uint,double> inOut; // To return

   // Collect expectations over edges
   // Iterating nodes currently best way to iterate edges
   const double maxScore_plus_tolerance = maxScore + abs(maxScore*1e-8);
   for(DecoderStateIterator it=statesInOrderBegin();it!=statesInOrderEnd();++it)
   {
      DecoderState* cur = *it;
      // For each outgoing edge...
      LatticeEdges edges(cur,*this);
      for ( SimpleOutEdgeIterator ei = edges.begin(); ei != edges.end(); ++ei ) {
         const LatticeEdge& edge = *ei;
         map<Uint,double>::const_iterator inIt, outIt;
         assert ( inOut.find(edge.id()) == inOut.end() );           // Edge ids should be unique
         if ( (outIt=out.find(edge.from()->id)) == out.end() ) assert(false); // Should have out score banked
         if ( (inIt=in.find(edge.to()->id)) == in.end() ) assert(false);      // Should have in score banked
         double edgeScore = inIt->second + edge.score() + outIt->second;
         if ( !(edgeScore <= maxScore_plus_tolerance) ) // Score should be bounded by max inside score
            error(ETFatal, "SimpleOverlay::insideOutside edgeScore=%.16g maxScore=%.16g (tolerance for edgeScore is <=%.16g)\n",
                  edgeScore, maxScore, maxScore_plus_tolerance);
         inOut[edge.id()] = edgeScore;
      }
   }
   return inOut;
}

/**
 * Print a pruned lattice to a stream
 * @param file      Output stream
 * @param print     PrintFunc object specifies what edge attributes to print
 * @param density   Prune the lattice down to #density edges per word
 * @param len       Number of words to use in density calculation
 * @param verbosity Higher numbers = more debugging info
 * @return          Number of edges printed (not necessarily len*density)
 */
Uint SimpleOverlay::print_pruned_lattice(ostream& file, PrintFunc & print, double density, Uint len, Uint verbosity)
{
   // Run inside-outside to score all edges with their Viterbi scores
   map<Uint,double> edgeScores = insideOutside();

   // Check if we need to prune - should probably do this first, but whatever
   Uint newSize = (Uint)(len * density);
   Uint oldSize = edgeScores.size();
   if(oldSize <= newSize) {
      if(verbosity>=2)
         cerr << "No pruning necessary for " << oldSize << " edges (" << oldSize/(double)len << ")" << endl;
      return print_lattice(file,print); // No need to prune      
   }
   else {
      // Figure out score cut-off
      vector<double> scores;
      for(map<Uint,double>::const_iterator it=edgeScores.begin();it!=edgeScores.end();++it) {
         scores.push_back(it->second);
      }
      sort(scores.begin(),scores.end(),greater<double>());
      double threshold = scores[newSize];
      if(verbosity>=2)
         cerr << "Calculating density with len " << len
              << " gives threshold " << threshold
              << "; my high " << scores[0]
              << " my low " << scores[scores.size()-1]
              << endl;

      // Print
      Uint numPrinted = print_lattice(file,print,inOutThreshold(edgeScores,threshold));

      if(verbosity>=2)
         cerr << "Pruned from " << oldSize << " edges (" << oldSize/(double)len << ") "
              << "to " << numPrinted << " edges (" << numPrinted/(double)len << ")"
              << endl;

      return numPrinted;
   }
}
