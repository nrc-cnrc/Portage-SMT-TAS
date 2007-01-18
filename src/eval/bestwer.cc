/**
 * @author Aaron Tikuisis
 * @file bestwer.cc  Program that finds the best possible wer score given a set of source and nbest.
 *
 * $Id$
 * 
 * Evaluation Module
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 * 
 * This file contains a main program that computes the best mWER or mPER score
 * obtainable by taking one target sentence from each of a set of N-best lists.
 */

#include "wer.h"
#include <exception_dump.h>
#include <portage_defs.h>
#include <str_utils.h>
#include <file_utils.h>
#include <printCopyright.h>

using namespace std;
using namespace Portage;

static const char *HELP =
"Usage: %s [-v] [-n nbest [-n nbest [ .. ]]] [-per] -best-trans-files tfile1 tfile2 ... tfile\n\
	-ref-files rfile1 rfile2 ... rfileR\n\
Outputs the best mWER or mPER score for the given data.\n\
Each tfiles should contain a set of k best translation candidates (for constant k)\n\
for the n-th source sentence, and each rfiler should contain S reference translations,\n\
such that the n-th line corresponds to the n-th source sentence.\n\
\n\
Options:\n\
 -v	Verbose.\n\
 -n nbest\n\
	Specify the number of candidate translations to choose from; for each source,\n\
	only the first nbest lines will be used as candidates.  If more than one -n\n\
	flag is given, multiple evaluations are done.  If -n is not specified, all\n\
	candidates are used.\n\
 -per	Use mPER score (otherwise, mWER score is used).\n\
";

int argListIndex(const char *lookfor, int argc, const char * const *argv)
{
    for (int i = 1; i < argc; i++)
    {
	if (strcmp(argv[i], lookfor) == 0) return i;
    }
    return -1;
}

bool inArgList(const char *lookfor, int argc, const char * const *argv)
{
    return argListIndex(lookfor, argc, argv) != -1;
}

int MAIN(argc, argv)
{
    printCopyright(2004, "bestwer");
    int vindex = argListIndex("-v", argc, argv);
    bool verbose = (vindex != -1);
    int pindex = argListIndex("-per", argc, argv);
    bool per = (pindex != -1);
    const char *evalString = per ? "mPER" : "mWER";
    int transIndex = argListIndex("-best-trans-files", argc, argv);
    int refIndex = argListIndex("-ref-files", argc, argv);
    vector<Uint> numBest;
    Uint maxNumBest = 0;
    int lastnIndex = 0;
    for (int i = argListIndex("-n", argc - lastnIndex, argv + lastnIndex); i != -1; i =
	    argListIndex("-n", argc - lastnIndex, argv + lastnIndex))
    {
	lastnIndex += i;
	if (lastnIndex + 1 >= argc)
	{
	    error(ETFatal, "Missing nbest value");
	} // if
	int next = atoi(argv[lastnIndex + 1]);
	if (next <= 0)
	{
	    error(ETFatal, "Invalid value for nbest");
	} // if
	numBest.push_back((Uint)next);
	maxNumBest = max(maxNumBest, next);
    } // for
    
    if (transIndex == -1 || refIndex == -1 || max(vindex, max(pindex, lastnIndex)) >
	    min(transIndex, refIndex) || inArgList("-h", argc, argv))
    {
	error(ETHelp, HELP, argv[0]);
    } // if
    
    Uint S;
    Uint R;
    Uint K = 0;
    if (refIndex > transIndex)
    {
	S = refIndex - transIndex - 1;
	R = argc - refIndex - 1;
    } else
    {
	S = argc - transIndex - 1;
	R = transIndex - refIndex - 1;
    } // if
    if (S == 0 || R == 0)
    {
	error(ETHelp, HELP, argv[0]);
    } // if
    
    const char * const *transFiles = argv +  transIndex + 1;
    const char * const *refFiles = argv + refIndex + 1;
    
    // Count the number of lines
    string buf;
    {
	IMagicStream in(transFiles[0]);
	getline(in, buf);
	while (!in.eof())
	{
	    K++;
	    buf.clear();
	    getline(in, buf);
	} // while
    }
    
    if (0 == K)
    {
	error(ETFatal, "Empty list of best translations");
    } // if
    
    if (numBest.size() == 0)
    {
	numBest.push_back(K);
	maxNumBest = K;
    } else
    {
	if (maxNumBest > K)
	{
	    error(ETFatal, "numBest is greater than the number of candidate translations");
	} // if
    } // if
    
    if (verbose)
    {
	cout << "S = " << S << endl;
	cout << "R = " << R << endl;
	cout << "K = " << K << endl;
	cout << "Reading target sentences" << endl;
    } // if
    string *tgt_sents[S];
    string *ref_sents[S];
    for (Uint s = 0; s < S; s++)
    {
	tgt_sents[s] = new string[maxNumBest];
	ref_sents[s] = new string[R];
	IMagicStream in(transFiles[s]);
	for (Uint k = 0; k < maxNumBest; k++)
	{
	    getline(in, tgt_sents[s][k]);
	} // for
	if (in.eof())
	{
	    error(ETFatal, "Inconsistent file length (in file %s)", transFiles[s]);
	} // if
	getline(in, buf);
	if (!in.eof() && maxNumBest == K)
	{
	    error(ETFatal, "Inconsistent file length (in file %s)", transFiles[s]);
	} // if
    } // for
    if (verbose) cout << "Reading reference sentences" << endl;
    for (Uint r = 0; r < R; r++)
    {
	IMagicStream in(refFiles[r]);
	for (Uint s = 0; s < S; s++)
	{
	    getline(in, ref_sents[s][r]);
	} // for
	if (in.eof())
	{
	    error(ETFatal, "Inconsistent file length (in file %s)", refFiles[r]);
	} // if
	getline(in, buf);
	if (!in.eof())
	{
	    error(ETFatal, "Inconsistent file length (in file %s)", refFiles[r]);
	} // if
    } // for
    
    for (vector<Uint>::const_iterator it = numBest.begin(); it != numBest.end(); it++)
    {
	if (verbose)
	{
	    if (*it > 1)
	    {
		cout << "Computing scores using " << *it << " top candidates" << endl;
	    } else
	    {
		cout << "Computing scores using 1 top candidate" << endl;
	    } // if
	} // if
	
	// Compute the total error rate score
	Uint totalScore = 0;
	for (Uint s = 0; s < S; s++)
	{
	    // Tokenize all the sentences
	    vector<string> tgts[*it];
	    for (Uint k = 0; k < *it; k++)
	    {
		split(tgt_sents[s][k], insert_iterator<vector<string> >(tgts[k],
			    tgts[k].begin()));
	    } // for
	    vector<string> refs[R];
	    for (Uint r = 0; r < R; r++)
	    {
	    split(ref_sents[s][r], insert_iterator<vector<string> >(refs[r],
			refs[r].begin()));
	    } // for
	    
	    // Find the minimum error between a test sentence and a reference sentence
	    Uint bestTgtIndex = 0;
	    Uint bestRefIndex = 0;
	    Uint curBest = tgts[0].size() + refs[0].size();
	    for (Uint k = 0; k < *it && curBest > 0; k++)
	    {
		for (Uint r = 0; r < R && curBest > 0; r++)
		{
		    Uint curScore = per ? find_mPER(tgts[k], refs[r]) : find_mWER(tgts[k],
			    refs[r]);
		    if (curScore < curBest)
		    {
			curBest = curScore;
			bestTgtIndex = k;
			bestRefIndex = r;
		    } // if
		} // for
	    } // for
	    if (verbose)
	    {
		cout << "Best match for sentence " << s << ": test=\"" <<
		    tgt_sents[s][bestTgtIndex] << "\", ref=\"" << ref_sents[s][bestRefIndex]
		    << "\" (index=" << bestTgtIndex << ", " << evalString << "=" << curBest <<
		    ")" << endl;
	    } // if
	    totalScore += curBest;
	} // for
	
	if (*it == K && numBest.size() == 1)
	{
	    cout << "Result: ";
	} else if (*it > 1)
	{
	    cout << "Result using " << *it << " candidates: ";
	} else
	{
	    cout << "Result using 1 candidate: ";
	} // if
	cout << totalScore << endl;
    } // for
    
    // Clean up
    for (Uint s = 0; s < S; s++)
    {
	delete [] tgt_sents[s];
	delete [] ref_sents[s];
    } // for
} END_MAIN
