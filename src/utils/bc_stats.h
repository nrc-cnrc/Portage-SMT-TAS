/**
 * @author George Foster
 * @file bc_stats.h  Statistics for evaluating binary classification results.
 * 
 * 
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef BCSTATS_H
#define BCSTATS_H

#include <utility>
#include <cmath>
#include <map>
#include <vector>
#include <iostream>
#include "gfstats.h"
#include "gfmath.h"

namespace Portage {
using std::pair;
using std::vector;

/**
 * Stats are calculated over a labelled sample in the form of <double,Uint>
 * pairs, where the first component is a score assigned by the classifier
 * (higher scores mean the classifier thinks the example is more likely to be
 * correct); and the second is the true class of the example, either 0
 * (incorrect) or 1 (correct). Some stats (eg iroc) require that the sample be
 * sorted; for efficiency, no sorting is done until these functions are called,
 * and then it is done only once. The sample is stored internally (once).
 */
class BCStats {

   /// A sample in the form of a pair of double and int
   typedef pair<double,Uint> Example; 
   /// Iterator of sample
   typedef vector<Example>::iterator VEIter;

   /// Callable entity to compare to Example
   struct ExampleCompare {
      /**
       * Compares two Examples.
       * @param e1,e2  Examples to compare.
       * @return  Returns true if e1.double < e2.double
       */
      bool operator()(const Example& e1, const Example& e2) {
 	 return e1.first < e2.first;
      }
   };

   /// The sample list
   vector<Example> sample;

   bool sorted;
   double nll_;
   double max_acc;
   double iroc_;
   double max_f;

   void doSort();		///< sort & calc all sort-dependent stats
   
public:

   /**
    * Create from a sample.
    * @param beg, end iterators over sample: must point to Example objects.
    * @param prob interpret scores as probabilities
    */
   template<class ExampleIter> BCStats(ExampleIter beg, ExampleIter end, bool prob);
   /// Destructor.
   ~BCStats() {}
      
   Uint n;			///< size of sample
   Uint n1;			///< num correct examples
   Uint n0;			///< num incorrect examples
   double p1;			///< prior of correctness: n1 / n
   double p0;			///< prior of incorrectness n0 / n

   double nll();		///< negative log likelihood, in bits, in [-inf,0]
   double nllBase();		///< best nll for fixed-prob model (using priors)
   double ppx();		///< perplexity, in [0,inf], unform model gets 2
   double nce();		///< normalized cross entropy, nominally in [0,1]

   // all following functions force dataset sorting

   double maxAcc();		///< max accuracy, optimized on sample
   double minCer();		///< min error = 1 - maxAcc()
   double iroc();		///< intgrl under ROC curve, in [0,1], rand = .5
   double aroc();		///< normalized iroc: 2 * (iroc-.5), in [0,1]
   double maxF();               ///< max F = 2 * prec * recall / (prec + recall)

   /**
    * Wrap an output iterator around cout; for use with accs(), rocs(), precrec()
    */
   struct coutOp {
      /// Post incrementation.
      /// @return Returns the object so we can chain them together.
      struct coutOp& operator++(int) {return *this;}
      /// No op operator.
      /// @return Returns the object so we can chain them together.
      struct coutOp& operator*() {return *this;}
      /// Prints a pair of double to standard output
      /// @param p pair to print to standard output
      /// @return Returns the object so we can chain them together.
      struct coutOp& operator=(const pair<double,double>& p) {
	 cout << p.first << " " << p.second << endl;
	 return *this;
      }
      /// Prints a double to standard output.
      /// @param p double to print to standard output
      /// @return Returns the object so we can chain them together.
      struct coutOp& operator=(const double p) {
	 cout << p << " " << endl;
	 return *this;
      }
   };

   /**
    * Write accuracies for thresholds at every position in sample.
    * @param dest place to store n+1 double values; ith value corresponds to
    * considering all examples as correct that occur at or above position i in
    * the sorted dataset. 
    */
   template<class OutIter> void accs(OutIter dest) {
      doSort();
      Uint nc1 = n1, nc0 = 0;
      
      for (VEIter p = sample.begin(); p != sample.end(); ++p) {
	 *dest++ = (nc1+nc0)/(double)n;
	 if (p->second) --nc1;
	 else ++nc0;
      }
      *dest = (nc1+nc0)/(double)n;
   }

   /**
    * Same as the template version, but write stats to stdout.
    */
   void accs() {coutOp op; accs(op);}


   /**
    * Write roc values for thresholds at every position in sample.
    * @param dest place to store n+1 <double,double> pairs; ith pair is (ca,cr)
    * when considering all examples as correct that occur at or above position i in
    * the sorted dataset. 
    */
   template<class OutIter> void rocs(OutIter dest) {
      doSort();
      Uint nc1 = n1, nc0 = 0;
      for (VEIter p = sample.begin(); p != sample.end(); ++p) {
	 *dest++ = make_pair(nc1/(double)n1, nc0/(double)n0);
	 if (p->second) --nc1;
	 else ++nc0;
      }
      *dest = make_pair(nc1/(double)n1, nc0/(double)n0);
   }

   /**
    * Same as the template version, but write stats to stdout.
    */
   void rocs() {coutOp op; rocs(op);}

   /**
    * Write precision/recall values for thresholds at every position in sample.
    * @param dest place to store n+1 <double,double> pairs; ith pair is (prec,rec)
    * when considering all examples as correct that occur at or above position i in
    * the sorted dataset. 
    */
   template<class OutIter> void precrecs(OutIter dest) {
      doSort();
      Uint nc1 = n1, nc0 = 0;
      double rec = nc1 / (double) n1;
      double prec = (nc1+n0-nc0) ? nc1 / (double)(nc1+n0-nc0) : 1.0;
      *dest++ = make_pair(prec, rec);
      for (VEIter p = sample.begin(); p != sample.end(); ++p) {
	 if (p->second) --nc1;
	 else ++nc0;
	 rec = nc1 / (double) n1;
	 prec = (nc1+n0-nc0) ? nc1 / (double)(nc1+n0-nc0) : 1.0;
	 *dest++ = make_pair(prec, rec);
      }
   }

   /**
    * Same as the template version, but write stats to stdout.
    */
   void precrecs() {coutOp op; precrecs(op);}


   /**
    * Write scores in order that corresponds to stats written by accs(), rocs(), and precrecs().
    */
   template<class OutIter> void scores(OutIter dest) {
      doSort();
      for (VEIter p = sample.begin(); p != sample.end(); ++p)
	 *dest++ = p->first;
   }

   /**
    * Same as the template version, but write stats to stdout.
    */
   void scores() {coutOp op; scores(op);}
   
};

template<class ExampleIter> BCStats::BCStats(ExampleIter beg, ExampleIter end, bool prob)
{
   n1 = 0;
   nll_ = 0.0;
   sorted = false;
   for (ExampleIter p = beg; p != end; ++p) {
      if (p->second) ++n1;
      if (prob)
	 nll_ -= log2(p->second ? p->first : 1.0 - p->first);
      sample.push_back(*p);
   }
   n = sample.size();
   if (n == 0) return;
   n0 = n - n1;
   p1 = n1 / (double) n;
   p0 = n0 / (double) n;
   if (prob) nll_ /= n;
}

}

#endif
