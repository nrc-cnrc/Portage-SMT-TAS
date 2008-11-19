/**
 * @author Aaron Tikuisis
 * @file fmax.cc  fmax.
 *
 * $Id$
 *
 * K-best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "fmax.h"
#include <perm_iterator.h>
#include <portage_defs.h>
#include <algorithm> // sort()
#include <cstring> // memset()

using namespace std;
using namespace Portage;

/*
 * A node in a 2D doubly-linked list.
 */
template <class T>
struct LLNode2D
{
   LLNode2D * next[2];
   LLNode2D * prev[2];
   T item;
}; // LLNode2D

/*
 * Check consistency of this node and it's neighbours.
 */
template <class T>
static bool checkLLNode(LLNode2D<T> *n)
{
   bool good = true;
   for (Uint i = 0; i < 2; i++)
   {
      good = good && (n->next[i] == NULL || n->next[i]->prev[i] == n);
      good = good && (n->prev[i] == NULL || n->prev[i]->next[i] == n);
   } // for
   return good;
} // checkLLNode

namespace Portage
{
   /// 2D-array coordinates
   struct IPair
   {
      Uint v[2];
      Uint &operator[](Uint i)
      {
         return v[i];
      } // operator[]
      const Uint &operator[](Uint i) const
      {
         return v[i];
      } // operator[]
   }; // IPair

   /// Callable entity for sorting in decreasing order elements of a matrice.
   class IPairComp
   {
      private:
         double **fvals;
      public:
         IPairComp(double **fvals): fvals(fvals) {}
         bool operator()(const IPair &l1, const IPair &l2)
         {
            return fvals[l1[0]][l1[1]] > fvals[l2[0]][l2[1]];
         } // operator()
         bool operator()(const LLNode2D< IPair > *l1, const LLNode2D< IPair > *l2)
         {
            return operator()(l1->item, l2->item);
         } // operator()
   }; // IPairComp

   /**
    * Uses llnodes to create a 2D linked list.  llheads[0] is indexed by s, with
    * llheads[0][s] being the head node of a linked list containing all entries (s, t) (s
    * fixed, t = 0, .. , |T| - 1) and in descending order by v(s, t).  Similarly,
    * llheads[1] is indexed by t, with llheads[1][t] being the head node of a linked list
    * containing all entries (s, t) (t fixed, s = 0, .. , |S| - 1) and in descending
    * order by v(s, t).
    * @param fvals      2D array representing the function \f$v:SxT \rightarrow [0, \inf)\f$.
    * @param llheads    A pair of vectors into which the heads of the linked lists are
    *                   stored.
    * @param totalSize  The size of the set S and of the set T respectively.
    * @param llnodes    A pointer to an array of |S||T| nodes that are used to make the
    *                   linked list.
    */
   static void createSortedLL2D(double **fvals, vector< LLNode2D< IPair > *> llheads[2], Uint
         totalSize[2], LLNode2D< IPair > *llnodes)
   {
      IPairComp comp(fvals);

      // Initialize the item's in each node
      for (Uint s = 0; s < totalSize[0]; s++)
      {
         for (Uint t = 0; t < totalSize[1]; t++)
         {
            llnodes[s * totalSize[1] + t].item[0] = s;
            llnodes[s * totalSize[1] + t].item[1] = t;
         } // for
      } // for

      for (Uint i = 0; i < 2; i++)
      {
         Uint index[2];
         for (index[i] = 0; index[i] < totalSize[i]; index[i]++)
         {
            // Use a vector to store the current LLNode2D's, so that they can be
            // sorted
            vector<LLNode2D< IPair > *> curVals;
            for (index[1 - i] = 0; index[1 - i] < totalSize[1 - i]; index[1 - i]++)
            {
               curVals.push_back(llnodes + index[0] * totalSize[1] + index[1]);
            } // for

            // Sort in descending order by fvals[node->item[0]][node->item[1]]
            sort(curVals.begin(), curVals.end(), comp);

            // Use the sorted list to create a linked list
            for (Uint j = 0; j < totalSize[1 - i]; j++)
            {
               curVals[j]->prev[i] = (j == 0 ? NULL : curVals[j - 1]);
               curVals[j]->next[i] = (j + 1 == totalSize[1 - i] ? NULL : curVals[j + 1]);
            } // for
            llheads[i].push_back(curVals.front());
         } // for
      } // for
   } // createSortedLL2D

   /**
    * Removes the given node from one of the two linked lists that it is in.
    * @param node       The node to remove from the linked lists.  This node is not
    *                   modified (so it won't "know" that it's not in the list, even
    *                   though the list will "know" that it's gone).
    * @param llheads    The two vectors of head nodes of the linked lists.
    * @param dir        The direction of the linked list to remove this from.
    */
   static void removeNode(LLNode2D< IPair > *node, vector< LLNode2D< IPair > *> llheads[2], Uint dir)
   {
      if (node->prev[dir] != NULL)
      {
         node->prev[dir]->next[dir] = node->next[dir];
      } else
      {
         llheads[dir][node->item[dir]] = node->next[dir];
      } // if
      if (node->next[dir] != NULL)
      {
         node->next[dir]->prev[dir] = node->prev[dir];
      } // if
   } // removeNode

   /**
    * Sets bitMap[i] = {true if \f$i \in\f$ set, false otherwise; for all i = 0, .. , size.
    * @param bitMap    A boolean array in which the bit map values are stored.
    * @param set       The set of Uint's to represent using bitMap.  This must be a
    *                  subset of (0, .. , size - 1}.
    * @param size      The size of the entire set.
    */
   static void makeSetBitMap(bool *bitMap, const vector<Uint> &set, Uint size)
   {
      memset(bitMap, false, size * sizeof(bool));
      for (vector<Uint>::const_iterator it = set.begin(); it < set.end(); it++)
      {
         bitMap[*it] = true;
      } // for
   } // makeSetBitMap

   /**
    * We represent subsets \f$S'\f$, \f$T'\f$ of \f$S\f$, \f$T\f$ resp. using the vectors sets and in this
    * function check whether for all \f$s' \in S'\f$, \f$t' \in T'\f$, \f$s \notin S'\f$, \f$t \notin T'\f$,
    * \f$v(s', t') \ge v(s', t) + v(s, t')\f$
    * This has \f$O(K^2)\f$ run-time, where \f$K\f$ is the size of the sets.
    * @param fvals     2D array representing the function \f$v:SxT \rightarrow [0, \inf)\f$.
    * @param llheads   A pair of vectors containing the heads of linked lists.
    *                  llheads[0] is indexed by s with llheads[0][s] being the head of
    *                  the linked list containing entries \f$(s, t)\f$ (\f$s\f$ fixed, \f$t \in T\f$) in
    *                  descending order by \f$v(s, t)\f$; and for llheads[1] indexed by \f$t\f$.  Not
    *                  all members of the original sets \f$S\f$ and \f$T\f$ must be in these lists
    *                  (ie. the list sizes may be smaller than the respective totalSize)
    *                  and llheads[i][j] may be NULL to indicate an entry no longer in
    *                  the set \f$S\f$ or \f$T\f$.
    * @param sets      A pair of vectors representing the sets \f$S'\f$, \f$T'\f$.  These two sets
    *                  (and vectors) must have the same size.
    * @param totalSize The total size of the original sets \f$S\f$, \f$T\f$.
    */
   static bool isDecomposableSubset(double **fvals, vector< LLNode2D< IPair > *> llheads[2],
         vector<Uint> sets[2], Uint totalSize[2])
   {

      /*
      cerr << "isDecomposableSubset() called with sets:" << endl;
      for (Uint i = 0; i < 2; i++)
      {
         cerr << "{" << sets[i].front();
         for (vector<Uint>::const_iterator it = sets[i].begin() + 1; it < sets[i].end();
               it++)
         {
            cerr << ", " << *it;
         } // for
         cerr << "}" << endl;
      } // for
      */

      assert(sets[0].size() == sets[1].size());

      bool result = true;

      // Create bit-maps to represent the sets
      bool inSet0[totalSize[0]];
      bool inSet1[totalSize[1]];
      bool *inSet[] = {inSet0, inSet1};
      for (Uint i = 0; i < 2; i++)
      {
         makeSetBitMap(inSet[i], sets[i], totalSize[i]);
      } // for

      // Compute:
      // max_{t \notin T'} v(s', t) for each s' \in S' (stored in maxVals[0][i] where
      // sets[i] == s'), and
      // max_{s \notin S'} v(s, t') for each t' \in T' (stored in maxVals[1][i] where
      // sets[i] == t').
      // At the same time, ensure that
      // max_{t \notin T'} v(s', t) >= min_{t' \in T'} v(s', t') for all s' \in S', and
      // max_{s \notin S'} v(s, t') >= min_{s' \in S'} v(s', t') for all t' \in T' (*)
      double maxVals[2][sets[0].size()];
      for (Uint i = 0; i < 2; i++)
      {
         for (Uint j = 0; j < sets[i].size() && result; j++)
         {
            LLNode2D< IPair> *node = llheads[i][sets[i][j]];
            Uint k = 0;
            for (; inSet[1 - i][node->item[1 - i]]; node = node->next[i], k++) {}
            if (k < sets[1 - i].size())
            {
               // The appropriate condition (*) (above) fails.
               //cerr << "k = " << k << ", too small" << endl;
               result = false;
            } // if
            maxVals[i][j] = fvals[node->item[0]][node->item[1]];
         } // for
      } // for

      // Now manually check for each (s', t') \in S'xT' that
      // v(s', t') >= max_{t \notin T'} v(s', t) + max_{s \notin S'} v(s, t')
      for (Uint s = 0; s < sets[0].size() && result; s++)
      {
         for (Uint t = 0; t < sets[1].size() && result; t++)
         {
            result = (fvals[sets[0][s]][sets[1][t]] >= maxVals[0][s] + maxVals[1][t]);
            if (!result)
            {
               //cerr << "Test " << sets[0][s] << " " << sets[1][t] << " fails" << endl;
            } // if
         } // for
      } // for
      if (result)
      {
         //cerr << "Test passes" << endl;
      }
      return result;
   } // isDecomposableSubset

   /**
    * Finds exact optimum by brute force.  sets[0] must be at least as big as sets[1],
    * and (currently), this is not the most efficient if it is bigger.  This is
    * O(sets[0].size()!) (but if we handled the case when sets[0] is bigger than sets[1]
    * better, it could be O(sets[0].size()! / (sets[0].size() - sets[1].size())!)).
    * @param fvals     2D array representing the function \f$v:SxT \rightarrow [0, \inf)\f$.
    * @param llheads   A pair of vectors containing the heads of linked lists.
    *                  llheads[0] is indexed by s with llheads[0][s] being the head of
    *                  the linked list containing entries (s, t) (s fixed, \f$t \in T\f$) in
    *                  descending order by \f$v(s, t)\f$; and for llheads[1] indexed by t.  Not
    *                  all members of the original sets \f$S\f$ and \f$T\f$ must be in these lists
    *                  (ie. the list sizes may be smaller than the respective totalSize)
    *                  and llheads[i][j] may be NULL to indicate an entry no longer in
    *                  the set \f$S\f$ or \f$T\f$.
    * @param sets      A pair of vectors representing the sets \f$S'\f$, \f$T'\f$.
    * @param func      The array representing the optimizing function found.
    *                  For each \f$s \in sets[0]\f$, func[s] is set by this function.
    */
   static void maxOverSubset(double **fvals, vector< LLNode2D< IPair > *> llheads[2],
         vector<Uint> sets[2], int *func)
   {

      /*
      cerr << "maxOverSubset() called with sets:" << endl;
      for (Uint i = 0; i < 2; i++)
      {
         cerr << "{" << sets[i].front();
         for (vector<Uint>::const_iterator it = sets[i].begin() + 1; it < sets[i].end();
               it++)
         {
            cerr << ", " << *it;
         } // for
         cerr << "}" << endl;
      } // for
      */

      assert(sets[0].size() >= sets[1].size());

      vector<Uint> maxVals;
      double max = -INFINITY;

      PermutationIterator perms(sets[0].size());
      while (true)
      {
         double curVal = 0;

         for (Uint i = 0; i < sets[0].size(); i++)
         {
            if (perms.vals[i] < sets[1].size())
            {
               curVal += fvals[sets[0][i]][sets[1][perms.vals[i]]];
            } // if
         } // for
         if (curVal > max)
         {
            max = curVal;
            maxVals.clear();
            maxVals.insert(maxVals.end(), perms.vals, perms.valsEnd);
         } // if
         if (!perms.step()) break;
      } // while
      for (Uint i = 0; i < sets[0].size(); i++)
      {
         func[sets[0][i]] = (maxVals[i] < sets[1].size() ? (int)sets[1][maxVals[i]] : -1);
      } // for
   } // maxOverSubset

   /**
    * Finds approximate optimum by heuristic.
    * @param fvals     2D array representing the function \f$v:SxT \rightarrow [0, \inf)\f$.
    * @param llheads   A pair of vectors containing the heads of linked lists.
    *                  llheads[0] is indexed by s with llheads[0][s] being the head of
    *                  the linked list containing entries (s, t) (s fixed, \f$t \in T\f$) in
    *                  descending order by v(s, t); and for llheads[1] indexed by t.  Not
    *                  all members of the original sets \f$S\f$ and \f$T\f$ must be in these lists
    *                  (ie. the list sizes may be smaller than the respective totalSize)
    *                  and llheads[i][j] may be NULL to indicate an entry no longer in
    *                  the set \f$S\f$ or \f$T\f$.
    * @param sets      A pair of vectors representing the sets \f$S'\f$, \f$T'\f$.
    * @param totalSize The total size of the original sets \f$S\f$, \f$T\f$.
    * @param func      The array representing the optimizing function found.
    *                  For each \f$s \in sets[0]\f$, func[s] is set by this function.
    */
   void approxMaxOverSubset(double **fvals, vector< LLNode2D< IPair > *> llheads[2],
         vector<Uint> sets[2], Uint totalSize[2], int *func)
   {
      // Optimizes the restricted function uses a greedy heuristic.
      bool taken[totalSize[1]];
      for (Uint i = 0; i < sets[1].size(); i++)
      {
         taken[sets[1][i]] = false;
      } // for

      // Create bit-maps to represent the sets
      vector<bool> inSet[2];
      for (Uint i = 0; i < 2; i++)
      {
         inSet[i].insert(inSet[i].end(), totalSize[i], false);
         for (vector<Uint>::const_iterator it = sets[i].begin(); it < sets[i].end(); it++)
         {
            inSet[i][*it] = true;
         } // for
      } // for

      // Initialize values very greedily:
      // T* = T'
      // for each s \in S',
      //   f(s) = argmax_{t \in T*} v(s, t)
      //   T* = T* \ f(s)

      for (Uint i = 0; i < sets[0].size(); i++)
      {
         if (i >= sets[1].size())
         {
            func[sets[0][i]] = -1;
         } else
         {
            LLNode2D< IPair > *node;
            for (node = llheads[0][sets[0][i]];
                  node != NULL && !(inSet[1][node->item[1]] &&
                     !taken[node->item[1]]); node = node->next[0]) {}
            assert(node != NULL);
            func[sets[0][i]] = node->item[1];
            taken[node->item[1]] = true;
         } // if
      } // for

      // Now make improvements by swapping function values f(s), f(s') and by replacing
      // f(s) with t where t is not taken.
      bool keepGoing = true;
      while (keepGoing)
      {
         keepGoing = false;
         for (Uint i = 0; i < sets[0].size(); i++)
         {
            for (Uint j = i + 1; j < sets[0].size(); j++)
            {
               double curVal = 0;
               double newVal = 0;
               if (func[sets[0][i]] != -1)
               {
                  curVal += fvals[sets[0][i]][func[sets[0][i]]];
                  newVal += fvals[sets[0][j]][func[sets[0][i]]];
               } // if
               if (func[sets[0][j]] != -1)
               {
                  curVal += fvals[sets[0][j]][func[sets[0][j]]];
                  newVal += fvals[sets[0][i]][func[sets[0][j]]];
               } // if
               if (newVal > curVal)
               {
                  keepGoing = true;
                  // Swap function values
                  Uint tmp = func[sets[0][i]];
                  func[sets[0][i]] = func[sets[0][j]];
                  func[sets[0][j]] = tmp;
               } // if
            } // for
            if (totalSize[1] > totalSize[0])
            {
               assert(func[sets[0][i]] != -1);
               LLNode2D< IPair > *node;
               for (node = llheads[0][sets[0][i]]; node != NULL &&
                     fvals[sets[0][i]][node->item[1]] >
                     fvals[sets[0][i]][func[sets[0][i]]] &&
                     !(inSet[1][node->item[1]] && !taken[node->item[1]]); node =
                     node->next[0]) {}
               assert(node != NULL);
               if (fvals[sets[0][i]][node->item[1]] >
                     fvals[sets[0][i]][func[sets[0][i]]])
               {
                  keepGoing = true;
                  // Replace function value
                  taken[func[sets[0][i]]] = false;
                  func[sets[0][i]] = node->item[1];
                  taken[func[sets[0][i]]] = true;
               } // if
            } // if
         } // for
      } // while
   } // approxMaxOverSubset

   /**
    * Finds a \f$K_0\f$ such that for \f$K < K_0\f$, the pair of subsets created using s will
    * definitely not be a decomposable subset containing s.
    * @param llheads   A pair of vectors containing the heads of linked lists.
    *                  llheads[0] is indexed by s with llheads[0][s] being the head of
    *                  the linked list containing entries (s, t) (s fixed, \f$t \in T\f$) in
    *                  descending order by \f$v(s, t)\f$; and for llheads[1] indexed by t.  Not
    *                  all members of the original sets \f$S\f$ and \f$T\f$ must be in these lists
    *                  (ie. the list sizes may be smaller than the respective totalSize)
    *                  and llheads[i][j] may be NULL to indicate an entry no longer in
    *                  the set \f$S\f$ or \f$T\f$.
    * @param s         The value \f$s \in S\f$ to find the minimum \f$K\f$ for.
    * @return          The minimum \f$K\f$ found (>= 1).
    */
   Uint findMinK(vector< LLNode2D< IPair > *> llheads[2], Uint s)
   {
      Uint k = 1;
      LLNode2D< IPair > *cur;
      assert(s < llheads[0].size());
      assert(llheads[0][s]->item[1] < llheads[1].size());
      for (cur = llheads[1][llheads[0][s]->item[1]]; cur != NULL; cur = cur->next[1], k++)
      {
         if (cur->item[0] == s) break;
      } // for
      //cerr << "minK[" << s << "]=" << k << endl;
      assert(cur != NULL);
      return k;
   } // findMinK

   double max_1to1_func(double **fvals, Uint numS, Uint numT, int *func)
   {
      if (numT == 0)
      {
         for (Uint i = 0; i < numS; i++)
         {
            func[i] = -1;
         } // for
         return 0;
      } else if (numS == 0)
      {
         return 0;
      } // if

      Uint totalSize[] = {numS, numT};

      LLNode2D< IPair > llnodes[totalSize[0] * totalSize[1]];
      vector<LLNode2D< IPair > *> llheads[2];

      createSortedLL2D(fvals, llheads, totalSize, llnodes);
      for (Uint i = 0; i < totalSize[0] * totalSize[1]; i++)
      {
         assert(checkLLNode(llnodes + i));
      } // for

      // First, we try to decompose S, T into subsets S', T' of equal size K <= 4 such a
      // (1-1) function f:S -> T maximizing \sum_{s \in S} v(s, f(s)) will have the
      // property that f(S') = T'.  This is the case under the following condition
      // (which makes use of the fact that our function v is non-negative):
      // v(s',t') >= v(s',t) + v(s,t'),   for all s' \in S', t' \in T', s \notin S',
      //                                  t \notin T)
      // Checking this condition takes O(K^2) time.
      //
      // For given s_0 \in S, we construct a potential T' by taking K elements
      // {t_1, ... , t_K} from T that maximize v(s_0, t_i) and then taking K elements
      // {s_1, ... , s_K} from S that maximize v(s_i, t_1).  This set will only work if
      // s_0 is one of the s_i (i = 1, ... , K).  We precompute a minimum K for which
      // this will be true.  Also, whenever we decompose away some set S, this minimum K
      // might become smaller; hence, we keep track of the best possible minimum K so
      // that we know when to recompute the minimum K.

      Uint minK[numS];
      int bestMinK[numS];
      for (Uint s = 0; s < numS; s++)
      {
         minK[s] = findMinK(llheads, s);
         bestMinK[s] = minK[s];
      } // for

      Uint curK = 1;
      while (true)
      {
         // As we decompose away sets, keep track of the number of things we get rid
         // of
         Uint numDropped = 0;
         for (Uint s = 0; s < totalSize[0] && min(numS, numT) - numDropped > curK; s++)
         {
            if (llheads[0][s] != NULL && minK[s] <= curK)
            {
               // Construct potential sets to decompose away
               vector<Uint> sets[2];
               for (LLNode2D< IPair > *cur = llheads[0][s]; sets[1].size() < curK; cur
                     = cur->next[0])
               {
                  sets[1].push_back(cur->item[1]);
               } // for
               for (LLNode2D< IPair > *cur = llheads[1][sets[1][0]]; sets[0].size() <
                     curK; cur = cur->next[1])
               {
                  sets[0].push_back(cur->item[0]);
               } // for

               if (isDecomposableSubset(fvals, llheads, sets, totalSize))
               {
                  maxOverSubset(fvals, llheads, sets, func);
                  // Remove from linked lists
                  for (Uint i = 0; i < 2; i++)
                  {
                     for (vector<Uint>::const_iterator it = sets[i].begin(); it <
                           sets[i].end(); it++)
                     {
                        for (LLNode2D< IPair > *node = llheads[i][*it]; node !=
                              NULL; node = node->next[i])
                        {
                           removeNode(node, llheads, 1 - i);
                        } // for
                        llheads[i][*it] = NULL;
                     } // for
                  } // for
                  numDropped += curK;
               } // if
            } // if
         } // for
         if (numDropped == 0)
         {
            curK++;
         } else
         {
            curK = 1;
         } // if
         numS -= numDropped;
         numT -= numDropped;
         if (curK > 4 || curK >= numS || curK >= numT)
         {
            //cerr << "breaking" << endl;
            break;
         } // if

         for (Uint s = 0; s < totalSize[0]; s++)
         {
            if (minK[s] > curK && llheads[0][s] != NULL)
            {
               // If minK[s] <= curK then there's no point in improving it since we
               // will try to use s anyways.
               bestMinK[s] -= numDropped;
               if (bestMinK[s] <= (int)curK)
               {
                  // If bestMinK[s] > curK then we put off recomputing minK[s] since
                  // it can't possibly be good enough.

                  // Recompute minK[s]
                  minK[s] = findMinK(llheads, s);
                  bestMinK[s] = minK[s];
               } // if
            } // if
         } // for
      } // while

      // Now that we've decomposed away everything we're sure about, use the
      // approxMaxOverSubset greedy approach to take care of the rest.
      if (numS > 0)
      {
         // Create the remaining sets
         vector<Uint> remaining[2];

         for (Uint i = 0; i < 2; i++)
         {
            Uint j;
            for (j = 0; j < totalSize[i]; j++)
            {
               if (llheads[i][j] != NULL)
               {
                  remaining[i].push_back(j);
               } // if
            } // for
         } // for

         // Find the maximizing function
         approxMaxOverSubset(fvals, llheads, remaining, totalSize, func);
      } // if

      // Finally, compute the optimal value given by the function
      double result = 0;
      for (Uint i = 0; i < totalSize[0]; i++)
      {
         double v = (func[i] == -1 ? 0 : fvals[i][func[i]]);
         //cerr << "f(" << i << ")=" << func[i] << "; v(" << i << ", " << func[i] << ")=" << v << endl;
         result += v;
      } // for
      return result;
   } // max_1to1_func

   double max_func(double **fvals, Uint numS, Uint numT, Uint *func)
   {
      if (numT == 0)
      {
         for (Uint s = 0; s < numS; s++)
         {
            func[s] = 0;
         } // for
         return 0;
      } else
      {
         double totalVal = 0;
         // This is much easier than max_1to1_func; we just maximize each component.
         for (Uint s = 0; s < numS; s++)
         {
            double curVal = fvals[s][0];
            func[s] = 0;
            for (Uint t = 1; t < numT; t++)
            {
               if (fvals[s][t] > curVal)
               {
                  func[s] = t;
                  curVal = fvals[s][t];
               } // if
            } // for
            totalVal += curVal;
         } // for
         return totalVal;
      } // if
   } // max_func()
} // Portage
