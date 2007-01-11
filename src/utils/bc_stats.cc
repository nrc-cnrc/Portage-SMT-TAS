/**
 * @author George Foster
 * @file bc_stats.cc  Statistics for evaluating binary classification results.
 * 
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#include <algorithm>
#include <utility>
#include "bc_stats.h"

using namespace Portage;
using namespace std;

void BCStats::doSort() 
{
   if (!sorted) {
      sort(sample.begin(), sample.end(), ExampleCompare());
      Uint nc1 = n1, nc0 = 0;

      max_f = 2.0 * nc1 / (n+nc1-nc0);
      max_acc = 0.0;
      iroc_ = 0.0;

      for (VEIter p = sample.begin(); p != sample.end(); ++p) {
	 max_acc = max(max_acc, (nc1+nc0)/(double)n);
	 if (p->second) {
	    iroc_ += nc0 / (double) n0;
	    --nc1;
	 } else 
	    ++nc0;
         max_f = max(max_f, 2.0 * nc1 /(n+nc1-nc0));
      }
      iroc_ /= n1;
      sorted = true;
   }
}

double BCStats::nll() {return nll_;}
double BCStats::nllBase() {return -(p1*Portage::log2(p1) + p0*Portage::log2(p0));}
double BCStats::ppx() {return Portage::exp2(nll_);}
double BCStats::nce() {return (nllBase() - nll_) / nllBase();}

double BCStats::maxAcc() {doSort(); return max_acc;}
double BCStats::minCer() {doSort(); return 1.0-max_acc;}
double BCStats::iroc() {doSort(); return iroc_;}
double BCStats::aroc() {doSort(); return 2.0 * (0.5-iroc_);}
double BCStats::maxF() {doSort(); return max_f;}

