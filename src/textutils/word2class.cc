/**
 * @author Darlene Stewart
 * @file word2class.cc
 * @brief  Program to map words in a text file to word classes, replacing
 * each word by its corresponding word class number.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "file_utils.h"
#include "arg_reader.h"
#include "wordClassMapper.h"
#include "errors.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
word2class [options] IN CLASSES [OUT]\n\
\n\
  Map words in IN to word classes, replacing each word by its corresponding\n\
  word class number from CLASSES (e.g. classes file output by mkcls) writing\n\
  the result to OUT.\n\
\n\
Options:\n\
\n\
  -no-error    Don't issue fatal error for presence of unknown word class\n\
               mapping errors [do]\n\
  -h    Print this help message\n\
  -v    Print progress reports to cerr\n\
  -d    Print debug information to cerr\n\
";

static bool no_error = false;
static bool verbose = false;
static bool debug = false;
static string in_file;
static string classes_file;
static string out_file("-");


void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"no-error", "v", "d"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("no-error", no_error);
   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", debug);

   arg_reader.testAndSet(0, "IN", in_file);
   arg_reader.testAndSet(1, "CLASSES", classes_file);
   arg_reader.testAndSet(2, "OUT", out_file);
}


struct classIdConverter {
   Uint map_errors;
   IWordClassesMapper* word_classes;

   classIdConverter(const string& filename)
      : map_errors(0)
      , word_classes(getWord2ClassesMapper(filename, "<unk>"))
   { }

   bool operator()(const char* const w, string& dest) {
      dest = (*word_classes)(w);
      if (dest == "<unk>")
         ++map_errors;
      if (debug) cerr << "  class of " << w << " : " << dest << nf_endl;
      return true;
   }
};


int main(int argc, char* argv[])
{
   printCopyright(2013, "word2class");
   getArgs(argc, argv);
   iSafeMagicStream txt_in(in_file);
   oSafeMagicStream txt_out(out_file);


   Uint lineno = 0;
   Uint tok_count = 0;
   string line;
   vector<string> cls;
   classIdConverter converter(classes_file);
   while (getline(txt_in, line)) {
      ++lineno;

      if (debug)
         cerr << "in(" << lineno << "): " << line << nf_endl;

      cls.clear();
      split(line.c_str(), cls, converter);

      tok_count += cls.size();

      txt_out << join(cls) << nf_endl;
   }

   txt_out.flush();

   if (converter.map_errors > 0)
      cerr << converter.map_errors << " word mapping errors." << endl;
   cerr << tok_count-converter.map_errors << " of " << tok_count << " words mapped to word classes." << endl;
   if (converter.map_errors > 0)
      error(no_error ? ETWarn : ETFatal,
            "Output contains %d word class mapping errors (<unk>).", converter.map_errors);
}

