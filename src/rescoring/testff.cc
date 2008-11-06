/**
 * @author Aaron Tikuisis
 * @file testff.cc  Program that tests the feature function interface.
 * $Id$
 * 
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "featurefunction.h"
#include "lm_ff.h"
#include <iostream>

using namespace std;
using namespace Portage;

int main(int argc, char* argv[])
{
	 if (argc > 1) {
	    cerr << "Program that tests the feature function interface." << endl;
		 exit(1);
	 }

    string tgt("the speaker : i have the honour to inform the house that the following members have been appointed as members of the board of internal economy for the purposes and under the provisions of the act to amend the parliament of canada act , chapter 32 , statutes of canada , 1997 , namely : mr. boudria and mr. mitchell , members of the queen 's privy council ; ms. catterall and mr. saada , representatives of the government caucus ; mr. strahl and mr. reynolds , representatives of the canadian alliance caucus ; mr. bergeron , representative of the bloc quebecois ; mr. blaikie , representative of the new democratic caucus ; and mr. mackay , representative of the progressive conservative caucus .");
    string src("");
    NgramFF ff("testdata/en_lm");
    
    Sentences sources;
    sources.push_back(Sentence(src));

    vector<string> targets;
    targets.push_back(tgt);
    Nbest nbest(targets);
    
    ff.init(&sources);
    ff.source(0, &nbest);

    cout << ff.value(0) << endl;
}
