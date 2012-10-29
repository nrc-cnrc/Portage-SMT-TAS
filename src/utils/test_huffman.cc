/**
 * @author Eric Joanis
 * @file test_huffman.cc
 * @brief Build a huffman tree for a given distribution, and print some stats about it.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
#include "huffman.h"
#include <locale>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
test_huffman [options] [INFILE]\n\
\n\
  Read <value, frequency> pairs, one per line, with whitespace separating the\n\
  two fields, and build a HuffmanCoder for it, printing statistics about the\n\
  results.\n\
\n\
Options:\n\
\n\
  -v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string infile("-");
static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   printCopyright(2012, "test_huffman");
   getArgs(argc, argv);

   iSafeMagicStream istr(infile);

   vector<pair<string,Uint> > D;
   string line;
   vector<string> tokens;
   Uint64 total_count(0);
   while (getline(istr, line)) {
      splitZ(line, tokens);
      if (tokens.size() != 2) error(ETFatal, "Bad input at line %u", D.size()+1);
      Uint freq = conv<Uint>(tokens[1]);
      total_count += freq;
      D.push_back(make_pair(tokens[0], freq));
      if (D.size() % 1000000 == 0) cout << ".";
   }
   cout << endl;
   HuffmanCoder<string> H(D.begin(), D.end());
   cout.imbue(locale(""));
   cout << "Read " << D.size() << " lines." << endl
        << "Total count " << total_count << endl
        << "Huffman total cost " << Uint64(H.totalCost()) << endl
        ;
   
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "infile", infile);
}
