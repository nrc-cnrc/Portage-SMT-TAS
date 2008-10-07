/**
 * @author Aaron Tikuisis,  modified by Matthew Arnold
 * @file linemax-cc.h  Implementation of line max
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "rescoring_general.h"
#include "bleu.h"
#include <algorithm>
#include <cassert>
#include "portage_defs.h"

namespace Portage
{
   /*Comment on fix:
     The bug fixed was in findSentenceIntervals.  In some occasions it was
     returning values out of range for gammas (inf, nan) so a fix was added
     in to prevent that and the second problem was that on some occasions it
     picked values that were already chosen (ie. ie said that the "line" for
     sentence 2 crossed the target line more than once, which is impossible,
     so a fix was added in to prevent a k value from being selected more
     than once, and not just prevent the selection of the previous k-value.
    */

   /**
    * Check if -inf < x < inf => x is finite
    * @param x  operand
    * @return Returns true if x is finite
    */
   inline bool finite(double x) {
      return (x != INFINITY && x != -INFINITY);
   }

   template <class ScoreStats>
   void LineMax<ScoreStats>::findSentenceIntervals(Uint &numchanges,
         double *& gamma,
         ScoreStats *& dBLEU,
         ScoreStats & curScore,
         linemaxpt *& myHeappoint,
         int s,
         const uVector& p,
         const uVector& dir,
         const uMatrix& H,
         const vector<ScoreStats>& scoreStats)
   // If a heap-point is not added, myHeappoint will be NULL.
   // All arrays indexed by s are replaced here by their s-th entry, ie..
   // numChanges = numChanges[s],
   // gamma = gamma[s],
   // dBLEU = dBLEU[s],
   // H = H[s],
   // scoreStats = scoreStats[s]
   {
      using namespace boost::numeric;
      /* For the given source sentence (f_s):
         The estimation function, \hat{e}(f_s, \lambda) is defined as follows:
         \hat{e}(f_s, \lambda)       = argmax_{0 <= k < K}
                                        ( \sum_{m=1}^M \lambda_m h_m(e_k, f_s) )
                                     = max_index (H * \lambda)

         (H is the K x M matrix whose (k,m)-th entry is h_m(e_k, f_s).)

         We consider the values of \hat{e} as we vary \lambda along the line:
         p + \gamma * dir.  As a function of \gamma (abusing notation), we have
         \hat{e}(\gamma) = max_index (H * p + \gamma * H * dir).

         Denote A = \gamma * H * dir, B = H * p.  Let
         L(\gamma) = \gamma * A + B,
         and let l_k(\gamma) be the k-th entry in L(\gamma).  Each l_k(\gamma) is
         clearly a line, called the k-th line.
         Clearly, the value of \hat{e}(\gamma) can only change at points where two
         different lines, l_i(\gamma) and l_j(\gamma) intersect.

         We do the following to completely determine the function \hat{e}(\gamma):
          i)         Determine some point \gamma_0 such that no lines intersect for
                     any \gamma <= \gamma_0
          ii)        Determine oldk = max_index L(\gamma_0).  Let oldgamma = \gamma_0.
          iii)       Determine newgamma = min (\gamma coordinate of intersection
                                               between l_k and l_oldk),
                                   newk = argmin (\gamma coordinate of intersection
                                                  between l_k and l_oldk),
                     where min, argmin range over all k which are different from
                     newk and for which the intersection happens after oldgamma.
                     If there is no such newgamma, go to step (v).
          iv)        It has been determined that \hat{e} takes the value oldk on
                     (oldgamma, newgamma),
                     (or (-\infty, newgamma) if oldgamma = \gamma_0).
                     Set oldk = newk, oldgamma = newgamma, and go to step (iii)
          v)          It has been determined that \hat{e} takes the value oldk on
                     (oldgamma, \infty),
                     (or (-\infty, \infty) if oldgamma = \gamma_0).
                     \hat{e} has been determined piecewise, so done.

         We store each newgamma (in ascending order) in gamma (an array), and the
         number of newgamma's in numchanges.  Since there are K different lines, we
         know a priori that there are at most (K-1) newgamma's to store.
         In practice here, we can forget the specific values of \hat{e} but remember
         their contribution to the BLEU score.  Thus, we tally up the total
         statistics for the BLEU score at \gamma_0 in curScore, and for the change in
         \hat{e} at gamma[i], we store the change in BLEU statistics in dBLEU[i].
         */

      assert(H.size1() == scoreStats.size());

      const Uint K(H.size1());
      const Uint M(H.size2());

      const uVector A(ublas::prec_prod(H, dir));
      const uVector B(ublas::prec_prod(H, p));
      uVector C(K);
      uVector pt(M);
      double sortA[K];

      numchanges = 0;

      bool found[K]; //array to track sentences we've seen
      fill(found, found+K, false);

      // Find all the cusps along the curve max_{k} (A[k]*x + B[k])

      // First, find an x-coordinate that occurs before any cusp
      for (Uint k(0); k<K; ++k) {
         sortA[k] = A(k);
         if (isnan(sortA[k])) sortA[k] = -INFINITY;
      } // for

      sort(sortA, sortA + K);
      double minDA = INFINITY;

      for (Uint k(0); k<K-1; ++k) {
         if (finite(sortA[k]) && finite(sortA[k+1]) && sortA[k+1] != sortA[k]) {
            minDA = min(minDA, sortA[k+1] - sortA[k]);
         } // if
      } // for

      double oldgamma(0.0f);
      if (minDA == INFINITY) {
         oldgamma = INFINITY;
      } else {
         double minB = INFINITY;
         double maxB = -INFINITY;
         for (Uint k(0); k<K; ++k) {
            const double x(B(k));
            if (finite(x)) {
               minB = min(minB, x);
               maxB = max(maxB, x);
            } // if
         } // for

         oldgamma = (minB - maxB) / minDA;
         // oldgamma = min_{i1,j1,i2,j2} (B[i1] - B[j1]) / (A[j2] - A[i2])
         //         <= min_{i,j} (B[i] - B[j]) / (A[j] - A[i])
         //         <= min_{i,j} (B[i] - B[j]) / (A[j] - A[i])
         // oldgamma - 1 would be our \gamma_0 (for step (i)), if we don't
         // have any infinite values in A.
      }

      // If any entry in A is +/-INFINITY, then the "line" for that candidate
      // sentence will take values from {+INFINITY, -INFINITY, NaN}.  When
      // the "line" changes from one of these values to another, consider
      // that to be an intersection point of that "line" with every other
      // line.
      // The value can only change at gamma where the vector p + gamma * dir
      // contains an entry of 0.
      // Here, we find the minimum gamma such that p + gamma * dir has a zero
      // entry, and the final \gamma_0 will be less than the minimum of this
      // and the previous oldgamma.

      // pt_m = -p_m / dir_m, for all m
      // ie. pt_m is the gamma s.t. p + gamma * dir = 0
      pt = p;
      for (Uint m(0); m<M; ++m)
         pt(m) /= dir(m);
      pt *= -1.0f;

      // In case any dir_m = 0, for some m, replace all non-finite values in
      // pt with 0
      for (Uint m(0); m<M; ++m) {
         if (!finite(pt(m))) {
            pt(m) = 0;
         }
      }
      // Subtract one here so that it's strictly less than any intersection point.
      oldgamma = min(oldgamma, *std::min_element(pt.begin(), pt.end())) - 1;

      // Determine argmax_{k} a[k]*oldgamma + b[k]:

      // C = B + oldgamma * A = H * (p + oldgamma * dir)
      // Calculate pt = p + oldgamma * dir, then calculate C
      // This should avoid inconsistency with infinity, which pops up with
      // the other way of computing C.
      pt = oldgamma * dir + p;
      C  = ublas::prec_prod(H, pt);

      // Initially, oldk = index of maximum element in C
      Uint oldk(my_vector_max_index(C));
#pragma omp critical (findSentenceIntervals_curBLEU)
{
      curScore += scoreStats[oldk];
} // ends omp critical section

      numchanges = 0;
      while (true) {
         // Find the line whose intersection with the oldk-th line occurs next
         double newgamma(oldgamma);
         int newk(-1);
         if (A(oldk) == INFINITY) {
            // Not going to do any better as gamma gets bigger
            break;
         }
         else if (A(oldk) == -INFINITY || isnan(A(oldk))) {
            newgamma = INFINITY;
            // Find if/when this "line" changes from +INFINITY to -INFINITY or NaN
            for (Uint m(0); m<M; ++m) {
               if ((H(oldk, m) ==  INFINITY && dir(m) < 0) ||
                     (H(oldk, m) == -INFINITY && dir(m) > 0))
               {
                  newgamma = min(newgamma, -p(m) / dir(m));
                  // Find gamma s.t. p_m + dir_m * gamma = 0
                  // The first point where this occurs (under the
                  // conditions in that if statement above) should be
                  // where the "line" becomes -INFINITY
               }
            }
            if (newgamma <= oldgamma || newgamma == INFINITY) {
               break;
            }
            else {
               // Find maximum at curgamma.
               pt = newgamma * dir + p;
               C = ublas::prec_prod(H, pt);
               newk = my_vector_max_index(C);
               while (newk != -1 && found[newk]) {
                  //while we've seen this newk already - catch to make
                  //sure we select a good value
                  C(newk) = -INFINITY;
                  newk = my_vector_max_index(C);
                  if (C(newk) == -INFINITY && found[newk])
                     newk = -1;
               }
               //newk = -1 || found[newk] == false
            } // if
         }
         else {
            for (Uint k(0); k<K; ++k) {
               //check to see if we haven't seen yet, not just if he
               //wasn't the last one picked
               if (!found[k]) {
                  double curgamma(0.0f);
                  if (A(k) == INFINITY || isnan(A(k))) {
                     curgamma = -INFINITY;
                     // Find where this "line" changes from -INFINITY or
                     // NaN to +INFINITY
                     for (Uint m(0); m<M; ++m) {
                        if ((H(k, m) ==  INFINITY && dir(m) > 0) ||
                              (H(k, m) == -INFINITY && dir(m) < 0))
                        {
                           curgamma = max(curgamma, -p(m) / dir(m));
                           // Find gamma s.t. p_m + dir_m * gamma = 0
                           // The last point where this occurs (under
                           // the conditions in that if statement
                           // above) should be where the "line"
                           // becomes +INFINITY
                        } // if
                     } // for
                  }
                  else {
                     curgamma = (B(k) - B(oldk)) / (A(oldk) - A(k));
                     // curgamma = (B[k] - B[oldk]) / (A[k] - A[oldk])
                     // This is the x component in the intersection of
                     // the lines:
                     // y = A[k] * x + B[k] , y = A[oldk] * x + B[oldk]
                  } // if

                  if (curgamma > oldgamma && (newk == -1 || curgamma < newgamma)) {
                     newgamma = curgamma;
                     newk = k;
                  } // if
               } // if
            } // for
         } // if

         //gamma unacceptable value (-inf, inf, nan) or no new intersection found
         if (newk == -1 || !finite(newgamma)) {
            // no new intersections
            break;
         } // if

         // Remember stuff for this intersection
         assert(numchanges < K);
         gamma[numchanges] = newgamma;
         dBLEU[numchanges] = scoreStats[newk] - scoreStats[oldk];
         numchanges++;

         oldk = newk;
         oldgamma = newgamma;
         found[newk] = true;
      } // while

      if (numchanges > 0) {
         myHeappoint = new linemaxpt();
         myHeappoint->gamma = gamma[0];
         myHeappoint->s = s;
         myHeappoint->i = 0;
      }
      else {
         myHeappoint = NULL;
      } // if
   } // ends LineMax<ScoreStats>::findSentenceIntervals


   ////////////////////////////////////////
   // LINEMAX
   template <class ScoreStats>
   void LineMax<ScoreStats>::operator()(uVector& p, uVector& dir)
   {
      assert(vH.size() == allScoreStats.size());

      const Uint S(vH.size());

      // If the best range found is (-\infty, t) or (t, \infty), we use
      // t - SMALL or t + SMALL respectively as the final gamma.
      const double SMALL(1.0f);

      Uint numchanges[S];

      ScoreStats curScoreStats;     // Accumulate the current BLEU statistics.

      // Store the linemaxpt values for the least gamma in each partition;
      // will subsequently become a heap.
      linemaxpt* heappoints[S];

      int s;
#pragma omp parallel for private(s)
      for (s=0; s<int(S); ++s) {
         findSentenceIntervals(numchanges[s],
                               gammaWorkSpace[s],
                               scoreWorkSpace[s],
                               curScoreStats,  // Needs a one time lock
                               heappoints[s],  // clean up null pointers
                               s,        // const
                               p,        // const
                               dir,      // const
                               vH[s],    // const
                               allScoreStats[s]); // const
      } // for

      // Remove the empty heap points and recalculate the heap size
      linemaxpt** last_heappoint = remove_if(heappoints, heappoints+S, linemaxpt::isNull);
      int heapsize(last_heappoint - heappoints);       // Number of members in heappoints

      /*
         Using the previous computations, we now determine the intervals on which
         the BLEU score is constant.
         Essentially, we order the gamma[s][i]'s from least to greatest:
         gamma[s_1][i_1] <= gamma[s_2][i_2] <= .. <= gamma[s_N][i_N]
         and compute the BLEU score on the intervals (-\infty, gamma[s_1][i_1]),
         (gamma[s_N][i_N], \infty), and (gamma[s_n][i_n], gamma[s_{n+1}][i_{n+1}])
         for all n.

         The BLEU stats for (-\infty, gamma[s_1][i_1]) are already stored in
         curScore.  For each (s,i), we have recorded (in scoreWorkSpace) the change in the
         BLEU stats from the interval just before gamma[s][i] to the interval just
         after gamma[s][i].  By iterating through the (s, i)'s in order by
         gamma[s][i] (ie.  iterating through the (s_n, i_n)'s in order by n), the
         BLEU stats for each interval are computed by updating the stats for the
         previous interval.

         In practice here, we already have
         gamma[s][0] <= gamma[s][1] <= .. <= gamma[s][numchanges[s] - 1]
         for each s, and this can be used to order the gamma[s][i]'s more
         efficiently (similar to mergesort).
         We use a heap containing triples (gamma, s, i), with the ordering that puts
         the triple with the least value for gamma at the root of the heap.  The
         following outlines how we iterate through the (s, i) in order:
         i)    Initially, produce a heap containing (gamma[s][0], s, 0) for
         each s. (The heap has at it's root the triple (gamma, s, i) for
         which gamma is least.)  (In the special case that numchanges[s]
         = 0 for some s, we obviously cannot have (gamma[s][0], s, 0) to
         the heap since there is no gamma[s][0].)
         ii)   At each iteration, remove the top, (gamma, s, i), of the heap
         (the next lowest gamma) and if i+1 < numchanges[s], add
         (gamma[s][i+1], s, i+1) to the heap.
         iii)  Repeat (ii) until the heap is empty.
         Our heap is contained in the array heappoints.
      */
      double maxscore(0.0f);  // Will hold the best BLEU score
      double maxgamma(0.0f);  // Will hold the gamma which produces the best BLEU score

      if (heapsize == 0) {
         // Special situation: no matter what gamma is, the BLEU score is
         // the same.
         maxscore = curScoreStats.score();
         maxgamma = 0;  // TODO: is 0 appropriate? I think so
         // cerr << "score at \\gamma = 0: " << curScoreStats.score() << endl;
      }
      else {
         // Create the heap
         make_heap(heappoints, heappoints + heapsize, linemaxpt::greater);

         maxscore = curScoreStats.score();
         maxgamma = heappoints[0]->gamma - SMALL;

         double oldgamma(0.0f);  // Holds the left endpoint of the interval whose stats are in curScoreStats
         while (true) {
            // Put max element at end of heap
            pop_heap(heappoints, heappoints + heapsize, linemaxpt::greater);
            // Update BLEU statistics
            curScoreStats += scoreWorkSpace[heappoints[heapsize - 1]->s][heappoints[heapsize - 1]->i];
            // Save left endpoint of the new interval
            oldgamma = heappoints[heapsize - 1]->gamma;

            heappoints[heapsize - 1]->i++;
            // Determine whether there is a new point to add to the heap
            // (same s, but i increases)
            if (heappoints[heapsize - 1]->i < numchanges[heappoints[heapsize - 1]->s]) {
               // Add point (gamma[s][i], s, i) to the heap
               heappoints[heapsize - 1]->gamma =
                  gammaWorkSpace[heappoints[heapsize - 1]->s][heappoints[heapsize - 1]->i];
               push_heap(heappoints, heappoints + heapsize, linemaxpt::greater);
            }
            else {
               // Decrease heap size
               delete heappoints[heapsize - 1];
               --heapsize;
            } // if

            // Exit loop if there are no new points (heap is empty)
            if (heapsize == 0) {
               break;
            } // for

            // Determine the BLEU score for the interval (oldgamma, heappoints[0]->gamma).
            const double curscore = curScoreStats.score();
            // Determine if this is the new best score AND if the interval is non-empty
            if (curscore > maxscore && heappoints[0]->gamma != oldgamma) {
               // New best score
               maxscore = curscore;
               // Use the midpoint of this range.
               maxgamma = (heappoints[0]->gamma + oldgamma) / 2;
            }
         }


         // Consider final score

         const double curscore = curScoreStats.score();
         if (curscore > maxscore) {
            // New best score
            maxscore = curscore;
            maxgamma = oldgamma + SMALL;
         }
      }

      // Return values appropriately.
      dir *= maxgamma;  // dir = (maxgamma - 1) *  dir + dir = maxgamma * dir
      p += dir;
   } // ends LineMax<ScoreStats>::operator()
} // ends namespace Portage
