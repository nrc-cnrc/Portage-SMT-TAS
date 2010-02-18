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


/**
 * @author Darlene Stewart
 * @file mmsufa.dump.cc
 * @brief Dumps memory mapped suffix array file (.msa) in plain text format to stdout.
 */
#include "tpt_typedefs.h"
#include "ug_mm_sufa.h"
#include "ug_mm_ctrack.h"
#include "tpt_tokenindex.h"

#include <boost/program_options.hpp>

using namespace std;
using namespace ugdiss;
namespace po = boost::program_options;

static char base_help_message[] = "\n\
mmsufa.dump [options] TOKEN_INDEX_FILE MMCTRACK_FILE MMSUFA_FILE\n\
\n\
  Dump a binary memory mapped suffix array file MMSUFA_FILE (.msa) to stdout\n\
  as plain text.\n\
\n\
  TOKEN_INDEX_FILE (.tdx) is the corresponding token index for the corpus.\n\
  MMCTRACK_FILE (.mct) is the corresponding memory mapped corpus track file\n\
  for the corpus.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

string tdxFile, mctFile, msaFile;

TokenIndex T;
mmCtrack   C;
mmSufa     S;

void 
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ;
  options_help << o;
  
  po::options_description h("Hidden Options");
  h.add_options()
    ("tdx", po::value<string>(&tdxFile), "token index file (.tdx)")
    ("mct", po::value<string>(&mctFile), "memory mapped corpus track file (.mct)")
    ("msa", po::value<string>(&msaFile), "memory mapped suffix array file (.msa)")
    ;
  h.add(o);

  po::positional_options_description a;
  a.add("tdx",1);
  a.add("mct",1);
  a.add("msa",1);
  
  try {
    po::store(po::command_line_parser(ac,av).options(h).positional(a).run(), vm);
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

  if (msaFile.empty())
    cerr << efatal << "Memory mapped suffix array MMSUFA_FILE (.msa) required." << endl
         << help_message << exit_1;
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);

  C.open(mctFile);
  T.open(tdxFile); T.iniReverseIndex();
  S.open(msaFile, &C);

  S.dump(cout, T);
}
END_MAIN
