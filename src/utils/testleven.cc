/**
 * @author Nicola Ueffing
 * @file testleven.cc implementation of Levenshtein alignment.
 *
 *
 * COMMENTS: test implementation of Levenshtein alignment
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "levenshtein.h"

using namespace Portage;

int main() {

  Levenshtein<char> lev;

  vector<char> a,b;

  a.push_back('a');
  a.push_back('b');
  a.push_back('c');
  a.push_back('d');
  a.push_back('e');

  b.push_back('a');
  b.push_back('c');
  b.push_back('f');
  b.push_back('e');
  b.push_back('g');
  b.push_back('h');

  vector<int> ins_costs(a.size());
  for (Uint i = 0; i < a.size(); ++i)
     ins_costs[i] = 1;

  vector<int> del_costs(b.size());
  for (Uint i = 0; i < b.size(); ++i)
     del_costs[i] = 1;
     // del_costs[i] = b[i] == 'g' || b[i] == 'h' ? 2 : 1;

  lev.setSubCost(3);

  // lev.setVerbosity(4);

  vector<int> res = lev.LevenAlig(a,b, &ins_costs, &del_costs);

  for (Uint i = 0; i < a.size(); ++i)
     cerr << a[i] << " ";
  cerr << endl;

  for (Uint i = 0; i < b.size(); ++i)
     cerr << b[i] << " ";
  cerr << endl;

  lev.dumpAlign(a, b, res, cerr);


//  cerr << lev.LevenDist(a,b) << endl;

  /***/

//   b.clear();
//   b.push_back('c');

//   lev.LevenAlig(a,b);
//   cerr << lev.LevenDist(a,b) << endl;

  /***/

//   a.clear(); b.clear();
//   a.push_back('h');

//   b.push_back('a');
//   b.push_back('c');
//   b.push_back('f');
//   b.push_back('e');
//   b.push_back('g');
//   b.push_back('h');

//   lev.LevenAlig(a,b);
//   cerr << lev.LevenDist(a,b) << endl;
}

