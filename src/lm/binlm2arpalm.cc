/**
 * @author Samuel Larkin
 * @file binlm2arpalm.cc  
 * @brief Convert an NRC PortageII's binary language model to an ARPA LM format.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include "lmtrie.h"
#include "arg_reader.h"
#include "exception_dump.h"
#include "file_utils.h"
#include <limits>
#include "printCopyright.h"

static char help_message[] = "\n\
binlm2arpalm [-vocab VOC] [-order ORDER] BINLM_FILE [ARPALM_FILE]\n\
\n\
 Convert a language model file BINLM_FILE in NRC PortageII's binary\n\
 language model format to Doug Paul's ARPA file format.\n\
 The output is written to ARPALM_FILE.\n\
 Valid extensions for BINLM_FILE are .binlm and .binlm.gz.\n\
\n\
Options:\n\
\n\
 -vocab limit vocab to the words in VOC [don't]\n\
 -order limit the order of the LM to ORDER [true order of BINLM_FILE]\n\
 -help  print this help message\n\
\n\
";

static string arpalm_filename;
static string binlm_filename;
static string vocab_file("");
static Uint order(numeric_limits<Uint>::max());

//Functions declarations
static void getArgs(int argc, const char* const argv[]);

int MAIN(argc,argv)
{
   printCopyright(2009, "binlm2arpalm");
   getArgs(argc, argv);

   time_t start = time(NULL);

   cerr << "Converting NRC PortageII's binary LM " << binlm_filename
        << " to ARPA LM format " << arpalm_filename << endl;

   VocabFilter vocab(0);
   const bool limit_vocab = (!vocab_file.empty());
   if ( limit_vocab ) {
      vocab.read(vocab_file);
      cerr << "Read vocab (... " << (time(NULL) - start) << " secs)" << endl;
   }

   PLM* plm = PLM::Create(binlm_filename, &vocab, PLM::ClosedVoc, -INFINITY,
                         limit_vocab, order, NULL);
   if ( !plm ) error(ETFatal, "LM file %s is not a supported format LM.",
                        binlm_filename.c_str());
   LMTrie* lm = dynamic_cast<LMTrie*>(plm);
   if ( !lm ) error(ETFatal, "LM file %s is not a supported format LM.",
                        binlm_filename.c_str());


   order = lm->getOrder();
   cerr << "Loaded " << order << "-gram model (... "
        << (time(NULL) - start) << " secs)" << endl;

   // Write the binlm to the arpa file.
   oSafeMagicStream arpalm_file(arpalm_filename);
   lm->write2arpalm(arpalm_file, order);

   // Free-up the memory used by the lm.
   delete lm;
   lm = NULL;

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
   const char* switches[] = {"v", "vocab:", "order:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("vocab", vocab_file);
   arg_reader.testAndSet("order", order);

   // Get and validate the binlm filename.
   // It doesn't actually have to be a binlm though.
   arg_reader.testAndSet(0, "binlm_file", binlm_filename);
   error_unless_exists(binlm_filename);

   // Get and validate the arpalm filename.
   arg_reader.testAndSet(1, "arpalm_file", arpalm_filename);
   if ( arpalm_filename.empty() ) {
      const bool isZip = isZipFile(binlm_filename);
      arpalm_filename = removeZipExtension(BaseName(binlm_filename));
      if (isSuffix(".binlm", arpalm_filename)) {
         arpalm_filename = removeExtension(arpalm_filename);
      }
      if (isZip) {
         arpalm_filename += ".gz";
      }
   }

   if ( isSuffix(".binlm", arpalm_filename) || 
        isSuffix(".binlm.gz", arpalm_filename) )
      error(ETFatal,
         "The arpalm filename must NOT have .binlm or .binlm.gz as suffix.");
}

