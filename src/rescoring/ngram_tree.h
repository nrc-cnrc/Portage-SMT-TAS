/**
 * @author Nicola Ueffing
 * @file ngram_tree.h Tree class for ngrams with their posterior probabilities
 *
 * $Id$
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada 
 * 
 * Contains the declaration of NgramTree
 */

#ifndef NGRAMTREE_H
#define NGRAMTREE_H

#include <stdio.h>
#include <assert.h>
#include <vector>
#include <set>
#include "confidence_score.h"

using namespace std;

namespace Portage {
   class NgramNode {
   private:
      ConfScore       c;
      string          w;
      set<NgramNode*> kids;
    
   public:
      NgramNode() : c(ConfScore()), w("</s>"), kids(set<NgramNode*>()) {}
      NgramNode(const string &word) : c(ConfScore()), w(word), kids(set<NgramNode*>()) {}
      ~NgramNode() {}

      /**
       * recursively delete all successors
       */    
      void clear();

      /**
       * reset posterior prob.
       */
      void resetPost();

      /**
       * Check if a node has a successor with word 'word'
       * Return pointer to kid or NULL
       */
      NgramNode* getKid(const string &word);

      /**
       * Add a child to a node
       */
      void       addKid(NgramNode* n);

      /**
       * Update the confidence value at a node (by given prob d and rank n)
       */
      void       update(const double &d, const int &n);

      /**
       * Return the confidence of a node
       */
      ConfScore  getPost();

      /**
       * Normalize the confidence score with given c
       */
      void       normalize(const ConfScore &c);

   };
    
   class NgramTree {
   private:
      NgramNode* root;
      int        numNodes;
    
   public:
      NgramTree() : root(new NgramNode()),numNodes(0) {}
      NgramTree(const NgramTree &t) : root(t.root),numNodes(t.numNodes) {}
      ~NgramTree() { clear(); }

      /**
       * Initialize the tree
       */
      void init();

      /**
       * Initialize the tree
       */
      void clear();

      /**
       * For a given n-gram and prob. and rank:
       *  Check whether all nodes for the words exists and insert new ones if
       *  necessary Update posterior probs. accordingly
       */
      void       update(const vector<string> &ngram, const double &d, const int &n);

      /**
       * Look for n-gram in tree and return success
       */
      bool       find(const vector<string> &ngram, const int &len);

      /**
       * Return number of nodes in the tree
       */
      int        size();

      /**
       * Return root of tree
       */
      NgramNode* getRoot();

      /**
       * normalize the posterior probabilities of all n-grams in the tree
       */
      void       normalize();

      /**
       * Obtain the posterior probability of a n-gram
       */
      ConfScore  getPost(vector<string> &ng);
   };
} // namespace


#endif
