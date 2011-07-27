// This file is derivative work from Ulrich Germann's Tightly Packed Tries
// package (TPTs and related software).
//
// Original Copyright:
// Copyright 2005-2009 Ulrich Germann; all rights reserved.
// Under licence to NRC.
//
// Copyright for modifications:
// Technologies langagieres interactives / Interactive Language Technologies
// Inst. de technologie de l'information / Institute for Information Technology
// Conseil national de recherches Canada / National Research Council Canada
// Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
// Copyright 2008-2010, Her Majesty in Right of Canada



// (c) 2007-2009 Ulrich Germann. All rights reserved.
/**
 * @author Ulrich Germann
 * @file vocab.build.cc
 * @brief Reads tokenized corpus text from stdin and writes a binary vocab
 * file and/or TokenIndex.
 *
 * First step in mmsufa generation. Normally called via build-tp-suffix-array.sh.
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/program_options.hpp>

#include "ug_vocab.h"

using namespace std;
using namespace ugdiss;
namespace po = boost::program_options;

static char base_help_message[] = "\n\
vocab.build [options] <[--vcb] VOCAB_FILE | --tdx TOKEN_INDEX_FILE>\n\
\n\
  Read tokenized text from stdin and write the VOCAB_FILE (.vcb) and/or\n\
  TOKEN_INDEX_FILE (.tdx).\n\
\n\
  This is the first step in creating a memory mapped suffix array for a corpus.\n\
  This program is called from build-tp-suffix-array.sh.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

bool   alpha=false, nosort=false, quiet=false;
string oFile;
const string toksep=" ";

string tdxFile,vcbFile; 

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("quiet,q",   "don't print progress information")
    ("tdx", po::value<string>(&tdxFile), "name of TokenIndex file (.tdx) (output)")
    ("vcb", po::value<string>(&vcbFile), "name of Vocab file (.vcb) (output)")
    ("alpha",  "sort alphabetically instead of by frequency")
    ("nosort", "don't sort at all")
#if 0
    // UNTESTED!
    ("toksep", po::value<string>(&toksep)->default_value(" "), 
     "token separator")
#endif
    ;
  options_help << o;
  po::positional_options_description a;
  a.add("vcb",1);

  try {
    po::store(po::command_line_parser(ac,av).options(o).positional(a).run(), vm);
    po::notify(vm);
  } catch(std::exception& e) {
    cerr << efatal << e.what() << endl << help_message << exit_1;
  }

  if (vm.count("help"))
    {
      cerr << help_message << endl;
      exit(0);
    }

  if (vcbFile.empty() && tdxFile.empty())
    cerr << efatal << "VOCAB_FILE or --tdx TOKEN_INDEX_FILE required." << endl
         << help_message << exit_1;

  quiet  = vm.count("quiet");
  alpha  = vm.count("alpha");
  nosort = vm.count("nosort");

  if (alpha && nosort)
    cerr << ewarn << "Only one of --alpha or --nosort may be used.\n"
         << "Sorting alphabetically and ignoring --nosort." << endl;
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);
  string lbuf;
  size_t lctr=0;
  Vocab V;

  while (getline(cin,lbuf))
    {
      istringstream line(lbuf);
      size_t a = lbuf.find_first_not_of(toksep,0);
      if (a == string::npos)
        continue;
      while (a != string::npos)
        {
          const size_t z = lbuf.find_first_of(toksep,a);
          if (z != string::npos)
            V[lbuf.substr(a,z-a)].cnt++;
          else
            {
              V[lbuf.substr(a)].cnt++;
              break;
            }
          a = lbuf.find_first_not_of(toksep,z);
        }
      if (!quiet && ++lctr%1000==0) 
	cerr << (lctr%1000) << "K sentences processed." << endl;
    }
  if (!quiet) cerr << lctr << " sentences processed in total." << endl;

  for (size_t i = 0; i < V.size(); i++)
    V.totalCount += V[i].cnt;

  if (alpha)
    {
      if (!quiet) cerr << "Sorting alphabetically..." << endl;
      V.sortAlphabetically();
    }
  else if (!nosort)
    {
      if (!quiet) cerr << "Optimizing IDs..." << endl;
      V.optimizeIDs();
    }

  if (!vcbFile.empty())
    {
      if (!quiet) cerr << "Writing vocabulary file..." << endl;
      V.pickle(vcbFile);
    }

  if (!tdxFile.empty())
    {
      if (!quiet) cerr << "Writing token index file..." << endl;
      ofstream tdx(tdxFile.c_str());
      if (tdx.fail())
        cerr << efatal << "Unable to open TokenIndex file '" << tdxFile
             << "' for writing." << exit_1;
      V.toTokenIndex(tdx);
      if (tdx.fail())
        cerr << efatal << "Writing TokenIndex file '" << tdxFile << "' failed."
             << exit_1;
      tdx.close();
    }
  if (!quiet) cerr << "Done" << endl;
}
END_MAIN

