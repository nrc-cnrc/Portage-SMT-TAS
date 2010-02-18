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
 * @file vocab.dump.cc
 * @brief Dumps a Vocab file (.vcb) in plain text format to stdout.
 *
 * First step in mmsufa generation. Normally called via mmsufa.compile.sh.
 */
#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>

#include "ug_vocab.h"

using namespace std;
using namespace ugdiss;

static char help_message[] = "\n\
vocab.dump [-h] VOCAB_FILE\n\
\n\
  Dump a corpus vocab file VOCAB_FILE (.vcb) to stdout as plain text.\n\
";

int MAIN(argc, argv)
{
   if (argc < 2)
     cerr << efatal << "VOCAB_FILE required." << endl << help_message << exit_1;
   if (!strcmp(argv[1], "-h"))
     {
       cerr << help_message << endl;
       exit(0);
     }
   if (argc > 2)
     cerr << efatal << "Too many arguments." << endl << help_message << exit_1;

   Vocab V;
   V.open(argv[1], Vocab::BIN);
   V.dump(cout, false, true, true, false);
}
END_MAIN
