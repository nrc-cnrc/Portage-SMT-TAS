/**
 * @author Eric Joanis
 * @file testcompactphrase.cc File to test some strange behaviour between
 *                            Phrase (vector<Uint>) and CompactPhrase.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <vector>
#include "portage_defs.h"
#define COMPACT_PHRASE_DEBUG
#include "compact_phrase.h"
#include "canoe_general.h"

using namespace Portage;
using namespace std;

// main

struct phrase_ref {
   const CompactPhrase& ref;
   phrase_ref(const CompactPhrase& ref) : ref(ref) {}
};

int main(int argc, char* argv[])
{
   cerr << "A" << endl;
   VectorPhrase p(3,35);
   cerr << "B" << endl;
   CompactPhrase cp(p);
   cerr << "C" << endl;
   vector<pair<CompactPhrase, double> > v1;
   v1.reserve(10);
   cerr << "D" << endl;
   v1.push_back(make_pair(p,1.1));  // silently converts p to a CompactPhrase
   cerr << "E" << endl;
   v1.push_back(make_pair(cp,1.1));
   cerr << "F" << endl;
   cerr << "0: " << v1[0].first.c_str();
   cerr << "G" << endl;
   cerr << "1: " << v1[1].first.c_str();
   cerr << "H" << endl;

   {
      const CompactPhrase& ref(p);
      //cerr << endl << "&p" << &p << "&ref" << &ref << endl;
      cerr << endl << "&p == &ref: " << bool((void*)&p == (void*)&ref) << endl;
      cerr << "I" << endl;
   }
   cerr << "J" << endl;

   //vector<phrase_ref> vref;
   //vref.reserve(1);
   phrase_ref *r;
   {
      //vref.push_back(phrase_ref(p));
      //phrase_ref r(p);
      r = new phrase_ref(p);
      //cerr << endl << "&p" << &p << "&r->ref" << &(r->ref) << endl;
      cerr << endl << "&p == &r->ref: " << bool((void*)&p == (void*)&r->ref) << endl;
      cerr << "cp.c_str()" << cp.c_str() << endl;
      cerr << "r->ref.c_str()" << r->ref.c_str() << endl;
      cerr << "K" << endl;
   }
   cerr << "L" << endl;
   char cossin[1000];
   memset(cossin, 0, 1000);
   cerr << "cp.c_str()" << cp.c_str() << endl;
   cerr << "r->ref.c_str()" << r->ref.c_str() << endl;
   cerr << "M" << endl;

}
