/**
 * @author Nicola Ueffing
 * @file testleven.cc implementation of Levenshtein alignment.
 *
 *
 * COMMENTS: test implementation of Levenshtein alignment
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "levenshtein.h"
#include "str_utils.h"
#include "file_utils.h"

using namespace Portage;

int main(int argc, const char* const argv[]) {

  if (argc!=3) {
    cerr << "Usage: " << argv[0] << "  <hypfile>  <reffile>" << endl << endl;
    exit(1);
  }
  
  string hypfilename = argv[1];
  string reffilename = argv[2];

  iSafeMagicStream hypfile(hypfilename);
  iSafeMagicStream reffile(reffilename);

  Levenshtein<string> lev;

  vector<string> hyp, ref;

  string hypline, refline;
  while ( getline(hypfile, hypline) && getline(reffile, refline) ) {
    
    hyp.clear(); ref.clear();
    
    split(hypline, hyp, " \n");
    split(refline, ref, " \n");

    vector<int> ins_costs(hyp.size());
    for (Uint i = 0; i < hyp.size(); ++i)
      ins_costs[i] = 1;

    vector<int> del_costs(ref.size());
    for (Uint i = 0; i < ref.size(); ++i)
      del_costs[i] = 1;
    // del_costs[i] = ref[i] == 'g' || ref[i] == 'h' ? 2 : 1;

    int incomplRef = 2;
    
    lev.setSubCost(1);
    
    lev.setVerbosity(1);

    vector<int> res = lev.LevenAlig(hyp, ref, incomplRef, &ins_costs, &del_costs);

    /*
      for (Uint i = 0; i < hyp.size(); ++i)
      cerr << hyp[i] << " ";
      cerr << endl;    
      for (Uint i = 0; i < ref.size(); ++i)
      cerr << ref[i] << " ";
      cerr << endl;
    */

    cerr << "Distance: " << lev.getLevenDist() << endl;
    lev.dumpAlign(hyp, ref, res, cerr);

  }
  
  if (getline(reffile, refline)) {
    cerr << "ERROR: less hypotheses than references!" << endl;
  }

  if (getline(hypfile, hypline)) { 
    cerr << "ERROR: more hypotheses than references!" << endl;
  }

}

