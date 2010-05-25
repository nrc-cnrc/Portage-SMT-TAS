/**
 * @author Eric Joanis
 * @file ibmcat.cc 
 * @brief Program that displays IBM and HMM models on STDOUT, even when they're binary.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include "ibm.h"
#include "hmm_jump_strategy.h"
#include "hmm_aligner.h"
#include "arg_reader.h"
#include "file_utils.h"

using namespace Portage;
using namespace std;

static const char help_message[] = "\n\
ibmcat MODEL_FILE\n\
\n\
  Display MODEL_FILE in plain text, whether is it a text or binary TTable,\n\
  IBM1, IBM2 positional or HMM dist model file.  If the file is in text format,\n\
  this is equivalent to zcat or cat.  If the file is in binary format, it is\n\
  converted to its plain text equivalent for human-readable display.\n\
\n\
  Normally, IBM2 and HMM models come in file pairs, but this program accepts\n\
  either file separately, so that it can work more intuitively like cat/zcat\n\
  would.\n\
\n\
Options:\n\
  -v    Write progress reports to cerr.\n\
  -h    Print this help message.\n\
";

static const char* switches[] = { "v" };

static ArgReader arg_reader(ARRAY_SIZE(switches), switches,
                            1, 1, help_message, "-h", true);
static bool verbose = false;
static string model;

void getArgs(int argc, char* argv[])
{
   arg_reader.read(argc-1, argv+1);
   arg_reader.testAndSet(0, "model", model);
}

int main(int argc, char* argv[])
{
   getArgs(argc, argv);
   const string ibm2_ext = IBM2::posParamFileName("");
   const string hmm_ext = HMMAligner::distParamFileName("");
   const string base_model_name = removeZipExtension(model);
   const size_t dot = base_model_name.rfind(".");
   if (dot != string::npos) {
      if (base_model_name.substr(dot) == ibm2_ext) {
         // process IBM2 positional parameter file
         // Here, we should in theory instanciate an IBM2 model to process the
         // binary format, but IBM2 doesn't actually have a binary format for
         // its .pos file, so we just do a "zcat".
         if ( verbose ) cerr << "IBM2 model file is not binary, using simple zcat." << endl;
         iSafeMagicStream in(model);
         cout << in.rdbuf();
         return 0;
      } else if (base_model_name.substr(dot) == hmm_ext) {
         // process HMM dist parameter file
         iSafeMagicStream is(model);
         HMMJumpStrategy* s = HMMJumpStrategy::CreateAndRead(is, model.c_str());
         cout << s->getMagicString() << endl;
         s->write(cout, false);
         delete s;
         return 0;
      }
   }

   // process TTable / IBM1 model
   TTable::output_in_plain_text(cout, model, verbose);
}

