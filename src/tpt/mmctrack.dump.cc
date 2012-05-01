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
 * @file mmctrack.dump.cc
 * @brief Dumps memory mapped corpus track file (.mct) in plain text format to stdout.
 */
#include "tpt_typedefs.h"
#include "ug_mm_ctrack.h"
#include "tpt_tokenindex.h"

#include <boost/program_options.hpp>
#include <fstream>

using namespace std;
using namespace ugdiss;
namespace po = boost::program_options;

static char base_help_message[] = "\n\
mmctrack.dump [options] TOKEN_INDEX_FILE MMCTRACK_FILE [RANGE1...RANGEN]\n\
\n\
  Dump a binary memory mapped corpus track file MMCTRACK_FILE (.mct) to stdout\n\
  as plain text.\n\
  TOKEN_INDEX_FILE (.tdx) is the corresponding token index for the corpus.\n\
  Each RANGEi is a 0-based range in the form N1[-N2] of sentences to dump.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

string tdxFile,mctFile, rangeFile;
vector<string> range;
bool withNumbers;

TokenIndex V;
mmCtrack   C;

void 
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("numbers,n", "print sentence ids as first token")
    ("file,f", po::value<string>(&rangeFile), "read ranges from file, one per line")
    ("tdx", po::value<string>(&tdxFile), "token index file (.tdx)")
    ("mct", po::value<string>(&mctFile), "memory mapped corpus track file (.mct)")
    ("range", po::value<vector<string> >(&range), "range (0-n)*")
    ;
  options_help << o;

  po::positional_options_description a;
  a.add("tdx",1);
  a.add("mct",1);
  a.add("range",-1);
  

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

  if (tdxFile.empty())
    cerr << efatal << "Token index TOKEN_INDEX_FILE (.tdx) required." << endl
         << help_message << exit_1;

  if (mctFile.empty())
    cerr << efatal << "Memory mapped corpus track MMCTRACK_FILE (.mct) required." << endl
         << help_message << exit_1;

  withNumbers = vm.count("numbers");
}

void 
printRange(size_t start, size_t stop)
{
  for (;start < stop; start++)
    {
      if (withNumbers) cout << start << " ";
      cout << C.str(start,V) << endl;
    }
}

void 
printRange(const string &range) {
  istringstream buf(range);
  size_t first,last; uchar c;
  buf>>first;
  if (buf.peek() == '-')
    buf>>c>>last;
  else
    last=first;
  if (last >= C.size())
    last = C.size() - 1;
  if (buf.eof() && first <= last)
    printRange(first,last+1);
  else
    cerr << ewarn << "Invalid range '" << range << "'." << endl;
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);
  V.open(tdxFile); 
  V.iniReverseIndex();
  C.open(mctFile);
  if (rangeFile.size()) {
    ifstream f(rangeFile.c_str());
    string line;
    while (f.good()) {
      getline(f, line);
      printRange(line);
    }
  } else if (range.size()) {
    for (size_t i = 0; i < range.size(); i++)
      printRange(range[i]);
  } else 
    printRange(0,C.size());
}
END_MAIN
