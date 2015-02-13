/**
 * @author Eric Joanis
 * @file arpalm2binlm.cc  
 * @brief Convert an ARPA LM file to the PortageII BinLM format.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lmtext.h"
#include "arg_reader.h"
#include "exception_dump.h"
#include "printCopyright.h"
#include "portage_defs.h"
#include "file_utils.h"

using namespace std;
using namespace Portage;

static char help_message[] = "\n\
arpalm2binlm [-vocab VOC] [-order ORDER] lm_file [binlm_file]\n\
\n\
 Convert language model file lm_file from Doug Paul's ARPA file format to\n\
 NRC PortageII's binary language model file format.\n\
 The output is written to binlm_file, or lm_file.binlm in the current directory\n\
 if not specified.  Valid extensions for binlm_file are .binlm and .binlm.gz.\n\
\n\
Options:\n\
\n\
 -vocab limit vocab to the words in VOC [don't]\n\
 -order limit the order of the LM to ORDER [true order of lm_file]\n\
 -help  print this help message\n\
\n\
";


static string lm_filename;
static string binlm_filename;
static string vocab_file("");
static Uint order(0);

//Functions declarations
static void getArgs(int argc, const char* const argv[]);

int MAIN(argc,argv)
{
   printCopyright(2006, "arpalm2binlm");

   getArgs(argc, argv);

   time_t start = time(NULL);

   cerr << "Converting ArpaLM " << lm_filename
        << " to BinLM " << binlm_filename << endl;

   VocabFilter vocab(0);
   const bool limit_vocab = (vocab_file != "");
   if ( limit_vocab ) {
      vocab.read(vocab_file);
      cerr << "Read vocab (... " << (time(NULL) - start) << " secs)" << endl;
   }

   PLM* lm = PLM::Create(lm_filename, &vocab, PLM::ClosedVoc, -INFINITY,
                         limit_vocab, order, NULL);
   order = lm->getOrder();
   cerr << "Loaded " << order << "-gram model (... "
        << (time(NULL) - start) << " secs)" << endl;

   LMText* lmtext = dynamic_cast<LMText*>(lm);
   if ( !lmtext ) error(ETFatal, "LM file %s is not an ARPA formatted LM.",
                        lm_filename.c_str());

   if ( isSuffix(".gz", binlm_filename) ) {
      const string binlm_tempfile = binlm_filename.substr(0, binlm_filename.size()-3);
      lmtext->write_binary(binlm_tempfile);
      cerr << "Wrote binlm (... " << (time(NULL) - start) << " secs)" << endl;
      if ( system(("gzip -9fq " + binlm_tempfile).c_str()) != 0 )
         error(ETFatal, "Problem gzipping %s", binlm_tempfile.c_str());
      cerr << "Compressed binlm (... " << (time(NULL) - start) << " secs)"
           << endl;
   } else {
      lmtext->write_binary(binlm_filename);
      cerr << "Wrote binlm (... " << (time(NULL) - start) << " secs)" << endl;
   }

   // Free-up the memory used by the lm.
   delete lmtext;
   lmtext = NULL;

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
   arg_reader.testAndSet(0,"lm_file", lm_filename);
   error_unless_exists(lm_filename);
   arg_reader.testAndSet(1,"binlm_file", binlm_filename);
   if ( binlm_filename == "" )
      binlm_filename = addExtension(BaseName(lm_filename), ".binlm");
   if ( !isSuffix(".binlm", binlm_filename) && 
        !isSuffix(".binlm.gz", binlm_filename) )
      error(ETFatal,
         "The binlm filename must have .binlm or .binlm.gz as suffix.");
}

