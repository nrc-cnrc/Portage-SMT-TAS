/**
 * @author George Foster
 * @file mx-dist2weights.cc   Calculate mixture weights from distance stats
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <numeric>
#include <math.h>
#include <file_utils.h>
#include <arg_reader.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
mx-dist2weights [-v] method dist1 [dist2 ...]\n\
\n\
Calculate mixture weights from distance statistics output by mx-calc-distances\n\
- larger values indicate better fit. Each <dist> file contains statistics from\n\
a single distance measure, one line per component. <method> is either the\n\
keyword 'normalize' (each dist file is normalized & results are averaged) or\n\
the name of a file containing one line for each <dist> file, with the info:\n\
   trans norm wt, \n\
where:\n\
   - trans is a transformation applied to each distance, one of: \n\
     'ident'       - dist (ie, no transformation)\n\
     'sigmoid' a b - 1/(1+exp(a * (b-dist)))\n\
   - norm is normalization over all values in each <dist>, one of:\n\
     'ident'       - don't normalize\n\
     'norm'        - normalize\n\
   - wt is the weight used to combine transformed, normalized distances, one of:\n\
     w             - multiply by <w>, before renormalizing across <dist> files\n\
";

// Sensible initial values: a = 1, b = 0.5
double sigmoid(double x, double a, double b) {
   return 1.0 / (1.0 + exp(a * (b-x)));
}

// globals

static bool verbose = false;
static string method = "";
static vector<string> distfiles;

static void getArgs(int argc, char* argv[]);

void checkToks(Uint pos, const vector<string>& toks, Uint lineno, const string& methfile)
{
   if (pos >= toks.size()) 
      error(ETFatal, "not enough tokens on line %d of %s", lineno, method.c_str());
}

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   // read and check distance files

   vector < vector<double> > distvects(distfiles.size()); // i -> ith distance vector
   string filecontents;
   for (Uint i = 0; i < distfiles.size(); ++i) {
      split(gulpFile(distfiles[i].c_str(), filecontents), distvects[i]);
      if (i > 0 && distvects[i].size() != distvects[0].size())
	 error(ETFatal, "file %s doesn't contain same number of distances as %s",
	       distfiles[i].c_str(), distfiles[0].c_str());
   }
      
   // convert distances to weights

   vector<double> wts(distvects[0].size());

   if (method == "normalize") {
      for (Uint i = 0; i < distvects.size(); ++i) {
	 double z = accumulate(distvects[i].begin(), distvects[i].end(), 0.0);
	 for (Uint j = 0; j < distvects[i].size(); ++j)
	    wts[j] += distvects[i][j] / z;
      }
      for (Uint i = 0; i < wts.size(); ++i) // average
	 wts[i] /= distvects.size();
   } else {
      IMagicStream istr(method);
      string line;
      vector<string> toks;
      for (Uint i = 0; i < distvects.size(); ++i) {
	 if (!getline(istr, line))
	    error(ETFatal, "not enough lines in %s for number of <dist> files", 
		  method.c_str());
	 splitZ(line, toks);
	 Uint pos = 0;

	 // transforming method
	 checkToks(pos, toks, i+1, method);
	 if (toks[pos] == "sigmoid") {
	    checkToks(++pos, toks, i+1, method);
	    double a = conv<double>(toks[pos]);
	    checkToks(++pos, toks, i+1, method);
	    double b = conv<double>(toks[pos]);
	    for (Uint j = 0; j < distvects[i].size(); ++j)
	       distvects[i][j] = sigmoid(distvects[i][j], a, b);
	 } else if (toks[pos] != "ident")
	    error(ETFatal, "expecting either 'ident' or 'sigmoid' as token %d on line %d of %s", 
		  pos+1, i+1, method.c_str());

	 // normalizing method
	 checkToks(++pos, toks, i+1, method);
	 if (toks[pos] == "norm") {
	    double z = accumulate(distvects[i].begin(), distvects[i].end(), 0.0);
	    for (Uint j = 0; j < distvects[i].size(); ++j)
	       distvects[i][j] /= z;
	 } else if (toks[pos] != "ident")
	    error(ETFatal, "expecting either 'ident' or 'norm' as token %d on line %d of %s", 
		  pos+1, i+1, method.c_str());
	 

	 // weighting method
	 checkToks(++pos, toks, i+1, method);
	 double w = conv<double>(toks[pos]);
	 for (Uint j = 0; j < distvects[i].size(); ++j)
	    wts[j] += w * distvects[i][j];

	 // at end?
	 if (++pos != toks.size())
	    error(ETFatal, "too many tokens on line %d of %s", i+1, method.c_str());


	 // final normalization
	 double z = accumulate(wts.begin(), wts.end(), 0.0);
	 for (Uint j = 0; j < wts.size(); ++j)
	    wts[j] /= z;
      }
   }
   

   // output

   for (Uint i = 0; i < wts.size(); ++i)
      cout << wts[i] << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   
   arg_reader.testAndSet(0, "method", method);
   arg_reader.getVars(1, distfiles);
}
