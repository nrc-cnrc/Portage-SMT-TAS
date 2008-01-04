/**
 * @author George Foster
 * @file cosine_distances.cc    Calculate various kinds of cosine distance for a given file
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
#include "arg_reader.h"
#include "doc_vect.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
cosine_distances [-v][-n nl] models vocfile [infile [outfile]]\n\
\n\
For each model listed in the file <models>, calculate the cosine score to\n\
<vocfile>. Both <models> and <vocfile> must contain, per line: any string,\n\
followed by whitespace, followed by a real number (the string may contain\n\
whitespace). Output is written to stdout, one number per line in <models>.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-df Smoothing term on inverse doc frequencies: augment number of docs by this amount\n\
    (higher is smoother; -1 means don't apply IDF's at all) [-1]\n\
";

// globals

static bool verbose = false;
static string models;
static string vocfile;
static double df = -1;

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   vector<string> model_list;
   IMagicStream ifs(models);
   string line;
   while (getline(ifs, line))
      model_list.push_back(trim(line));

   DocVectSet dvs;

   if (verbose) cerr << "reading source files" << endl;
   dvs.read(model_list);
   dvs.addDocVect(vocfile);

   if (df != -1) 
      dvs.tfidf(df); // possibly revisit this (works on ALL doc vects)

   if (verbose) cerr << "writing phrase-table distances" << endl;
   Uint K = model_list.size(); 
   const DocVectSet::DocVect& srcfile_vect = dvs.getDocVect(K);
   for (Uint k = 0; k < K; ++k)
      cout << dvs.cosine(srcfile_vect, dvs.getDocVect(k)) << endl;

   if (verbose) cerr << "done" << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "df:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("df", df);

   arg_reader.testAndSet(0, "models", models);
   arg_reader.testAndSet(1, "vocfile", vocfile);
}
