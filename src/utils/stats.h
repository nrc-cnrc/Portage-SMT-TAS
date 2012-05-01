/**
 * @author Evan Stratford
 * @file stats.h  Statistics reporting utilities.
 * 
 * COMMENTS: 
 *
 * A general-purpose stat collection and reporting library. The Stat class
 * serves as a base class for various stat collectors (totals, histograms,
 * mean and variance, etc.)
 *
 * HistogramStat my_hist("my-hist");
 * // ...
 * my_hist.add(42);
 * my_hist.add(28, 6);
 * // ...
 * my_hist.write(cout);
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef STATS_H
#define STATS_H

#include <iostream>
#include <map>
#include "portage_defs.h"
#include <string>
#include <vector>

namespace Portage {

/**
 * Base class for stats.
 */
class Stat {
   friend class TestStats;
protected:
   string name;
   template <typename T>
   void print(ostream& out, const string& fieldName, T val) const {
      out << name << "-" << fieldName << " = " << val << "\n";
   }
public:
   Stat(const string& name) : name(name) {}
   virtual ~Stat() {}
   virtual void write(ostream& out) const = 0;
}; // class Stat

class CountStat : public Stat {
   friend class TestStats;
   Uint c;
public:
   CountStat(const string& name) : Stat(name), c(0) {}
   void incr() { c++; }
   virtual void write(ostream& out) const;
}; // class CountStat

/**
 *
 */
class TotalStat : public Stat {
   friend class TestStats;
   int c;
public:
   TotalStat(const string& name) : Stat(name), c(0) {}
   void add(int k = 1) { c += k; }
   virtual void write(ostream& out) const;
}; // class TotalStat

/**
 *
 */
class AvgVarStat : public Stat {
   friend class TestStats;
   Uint n;
   double m, v;
public:
   AvgVarStat(const string& name) : Stat(name), n(0), m(0.0), v(0.0) {}
   void add(double k) { n++; const double d=k-m; m += d/n; v += d*(k-m); }
   virtual void write(ostream& out) const;
}; // class AvgVarStat

class AvgVarTotalStat : public AvgVarStat {
   friend class TestStats;
   double total;
public:
   AvgVarTotalStat(const string& name) : AvgVarStat(name), total(0) {}
   void add(double k) { total += k; AvgVarStat::add(k); }
   virtual void write(ostream& out) const;
}; // class AvgVarTotalStat

class HistogramStat : public Stat {
   friend class TestStats;
   map<int, Uint> h;
public:
   HistogramStat(const string& name) : Stat(name) {}
   void add(int k, Uint t = 1) { h[k] += t; }
   virtual void write(ostream& out) const;
};

} // namespace Portage

#endif
