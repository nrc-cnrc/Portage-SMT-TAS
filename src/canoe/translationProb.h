/**
 * @author Samuel Larkin
 * @file translationProb.h  Contains a class that finds and sums the probability
 * of a translation for all aligments in the lattice.
 *
 * $Id$
 *
 * Translation Probability Calculator
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */
             
#ifndef __TRANSLATION_PROBABILITY_H__
#define __TRANSLATION_PROBABILITY_H__

#include <phrasedecoder_model.h>
#include <decoder.h>
#include <quick_set.h>
#include <map>
#include <vector>
#include <string>

namespace Portage {

using namespace std;

/// Definition for a set of Tokens.
typedef vector<unsigned int> TOKENS;
/// Definition for a set of DecoderStates.
typedef vector<DecoderState*> VDS;

/**
 * Traverses the lattice to calculate the total probability for all path
 * producing a translation.
 * Can't be use as is because to properly calculate the total probability for a
 * translation, we must only sum up the features which depend on alignments.
 */
class translationProb
{
   private:
      /// Definition for a probability type.
      typedef double Prob;
      /// Definition for a node identification type.
      typedef unsigned int nodeID;
      /// Definition for length type.
      typedef unsigned int Length;
      /// Definition for the suffixes length type.
      typedef Length SuffixLength;
      
      /// Keeping track of which node's probability we've already computed.
      /// A bit of dynamic programming to speed up things.
      class Parcouru
      {
         public:
            /// Defines marker when we find a new nodeID which we don't have it's probability.
            static const Prob NOT_SEEN;
         
         private:
            /**
             * Definition for a key of an already computed node.
             * Note that it's insufficient just to keep track of a node's id
             * since we are traversing the lattice in a backward manner, from
             * the end of a translation towards its beginning, a node represent
             * a prefix but nothing garantees what suffixes can be appended to
             * that node thus producing completely differents translations.
             * For that reason we keep track of the prefixes also as part of
             * the key.
             */
            typedef pair<nodeID, SuffixLength> Key;
            /// Definition of a map to keep track of a masse for a node.
            typedef map<Key, Prob> Seen;
            /// Definition for an iterator for what was already computed.
            typedef Seen::const_iterator SIT;

            /// Contains the probabilities of DecoderStates that were already computed.
            Seen m_seen;
            
         public:
            /**
             * Verifies if the probability of a state was already calculated and if so returns it.
             * @param[in] id  the nodeID we are looking for.
             * @param[in] sl  the suffix length since a node can be part of two differents paths.
             * @return Returns the probability masse of a node given the a traverse suffix length.
             */
            Prob find(const nodeID& id, const SuffixLength& sl) const {
               SIT sit = m_seen.find(Key(id, sl));
               if (sit == m_seen.end())
                  return NOT_SEEN;
               else
                  return sit->second;
            }

            /**
             * Adds a computed probability to a node.
             * @param[in] id  the nodeID we are looking for.
             * @param[in] sl  the suffix length since a node can be part of two differents paths.
             * @param[in] p   the computed probability for the path leading to nodeID.
             * @return  Returns
             */
            Prob add(const nodeID& id, const SuffixLength& sl, const Prob& p) {
               assert(find(id, sl) == NOT_SEEN);

               return (m_seen[Key(id, sl)] += p);
            }
      };
   
      PhraseDecoderModel&  m_pdm;  ///< model from which the states were created.
   
   public:
      /**
       * Constructor.
       * @param[in] pdm  model from which the DecoderState were created.
       */
      translationProb(PhraseDecoderModel& pdm) : m_pdm(pdm) {}
      
      /**
       * Entry point to calculate the total weight for a set of translations.
       * @param[in] filename     file containing translations we want to get their total weight.
       * @param[in] finalStates  vector of all final DecoderStates (similar to a unique acceptor state).
       * @param[in] dMasse       the weight of the entire lattice (used to verify computation).
       */
      void find(const string& filename, const VDS& finalStates, const Prob& dMasse);

   private:
      /**
       * Calculates from a node, the sublattice total probability weight.
       * @param[in] sent  partial translation we are currently computing.
       * @param[in] ds    current node in the lattice.
       * @param[in] vu    keeps track of which node we already computed (dynamic programming).
       * @param[in] sl    suffix length coverted so far.
       * @return Returns the sublattice weight for the node at DecoderState.
       */
      double recursion(TOKENS sent, const DecoderState * const ds, Parcouru& vu, const SuffixLength& sl = 0) const;
      
      /**
       * Transforms a strings translation into a uint translation (speed-up hack).
       * @param[in]  hypothese  input string translation.
       * @param[out] tokens     transformed output uint translation.
       */
      void getTokens(const string& hypothese, TOKENS& tokens) const;
}; // ends class translationProb

} // ends namespace Portage

#endif  // __TRANSLATION_PROBABILITY_H__
