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



// (c) 2007,2008 Ulrich Germann
/**
 * @author Ulrich Germann
 * @file tokenindex.dump.cc
 * @brief Dumps a TokenIndex (vocab file for TPPT and TPLM) to stdout.
 */
#include <iostream>
#include <iomanip>

#include "tpt_tokenindex.h"

using namespace std;
using namespace ugdiss;

static char help_message[] = "\n\
tokenindex.dump [-h] TOKEN_INDEX_FILE\n\
\n\
  Dump a TokenIndex (vocab file for TPPT and TPLM) to stdout as plain text.\n\
\n\
  TOKEN_INDEX_FILE is the name of the token index file (.tdx) to be dumped.\n\
";

int MAIN(argc, argv)
{
  if (argc < 2)
    cerr << efatal << "TOKEN_INDEX_FILE required." << endl
         << help_message << exit_1;
  if (!strcmp(argv[1], "-h"))
    {
      cerr << help_message << endl;
      exit(0);
    }
  if (argc > 2)
    cerr << efatal << "Too many arguments." << endl << help_message << exit_1;

  TokenIndex I(argv[1]);
  vector<char const*> foo = I.reverseIndex();
  for (size_t i = 0; i < foo.size(); i++)
    cout << setw(10) << i << " " << foo[i] << endl;
}
END_MAIN
