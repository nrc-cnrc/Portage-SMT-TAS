/**
 * @author Samuel Larkin
 * @file test_lm_order.cc
 * @brief Determine the order of a language model file.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lmtrie.h"
#include "arg_reader.h"
#include "exception_dump.h"
#include "file_utils.h"
#include <limits>

static char help_message[] = "\n\
test_lm_order [-order ORDER] lm_file\n\
 Returns the order of the loaded language model.\n\
\n\
WARNING: this pgm will load your lm into memory.\n\
\n\
Options:\n\
\n\
 -order limit the order of the LM to ORDER [true order of binlm_file]\n\
 -help  print this help message\n\
\n\
";

static string lm_filename;
static Uint order(numeric_limits<Uint>::max());

//Functions declarations
static void getArgs(int argc, const char* const argv[]);

int MAIN(argc,argv)
{
   getArgs(argc, argv);

   time_t start = time(NULL);

   VocabFilter vocab(0);
   const bool limit_vocab = false;

   PLM* plm = PLM::Create(lm_filename, &vocab, PLM::ClosedVoc, -INFINITY,
                         limit_vocab, order, NULL);
   if ( !plm ) {
      error(ETFatal, "LM file %s is not a supported format LM.", lm_filename.c_str());
   }

   cerr << "Loaded " << plm->getOrder() << "-gram model (... "
        << (time(NULL) - start) << " secs)" << endl;

   // Free-up the memory used by the lm.
   delete plm;
   plm = NULL;

   Uint seconds(time(NULL) - start);
   Uint mins = seconds / 60;
   const Uint hours = mins / 60;
   mins = mins % 60;
   seconds = seconds % 60;

   cerr << "Program done in " << hours << "h" << mins << "m"
        << seconds << "s" << endl;

} END_MAIN


// arg processing
void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "order:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("order", order);

   // Get and validate the binlm filename.
   // It doesn't actually have to be a binlm though.
   arg_reader.testAndSet(0, "lm_file", lm_filename);
   error_unless_exists(lm_filename);
}

