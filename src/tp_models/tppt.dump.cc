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
 * @file tppt.dump.cc
 * @brief Outputs to stdout the content of a tightly packed phrase table.
 */
#include "tppt.h"
#include <iostream>

using namespace ugdiss;
using namespace std;

static char help_message[] = "\n\
tppt.dump [-h|-sort|-nosort] TPPT_BASE_NAME\n\
\n\
  Output a tightly packed phrase table to stdout in plain text format.\n\
\n\
  TPPT_BASE_NAME is either the name of a directory holding the files comprising\n\
  the tightly packed phrase table (cbk, src.tdx, tppt, trg.repos.dat, trg.tdx),\n\
  or it is the base name of the set of files comprising the tightly packed\n\
  phrase table (TPPT_BASE_NAME.cbk, TPPT_BASE_NAME.src.tdx, TPPT_BASE_NAME.tppt,\n\
  TPPT_BASE_NAME.trg.repos.dat, TPPT_BASE_NAME.trg.tdx)\n\
\n\
  Options:\n\
   -h         print this help message\n\
   -[no]sort  sort/don't sort the output as it is being produced. [sort]\n\
\n\
";

int MAIN(argc, argv)
{
  if (argc < 2)
    cerr << efatal << "TPPT_BASE_NAME required." << endl
         << help_message << exit_1;
  if (!strcmp(argv[1], "-h"))
    {
      cerr << help_message << endl;
      exit(0);
    }

  bool sortoutput = true;
  bool found_an_option = false;
  if (!strcmp(argv[1], "-nosort")) { found_an_option = true; sortoutput = false; }
  if (!strcmp(argv[1], "-sort")) { found_an_option = true; sortoutput = true; }

  if (argc > 3 or (!found_an_option && argc > 2))
    cerr << efatal << "Too many arguments." << endl << help_message << exit_1;
  string tppt_name = found_an_option ? argv[2] : argv[1];

  TpPhraseTable T;
  T.open(tppt_name);
  T.dump(cout, sortoutput);
}
END_MAIN
