/**
 * @author Samuel Larkin
 * @file map_number.cc
 * @brief Replaces digits of a number into a token (@).
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "number_mapper.h"
#include "MagicStream.h"
#include "str_utils.h"
#include "arg_reader.h"
#include <iterator>

using namespace Portage;

static char help_message[] = "\n\
map_number [-map type] [-token token] [-input file] \n\
\n\
Substitutes digits in words by token.\n\
\n\
Options:\n\
\n\
-v       verbose [false].\n\
-input   input file to process [-].\n\
-map     map type (Not yet fully implemented) [prefixNumber].\n\
-token   token to replace digits [@].\n\
";

static bool verbose = false;
static string input_file = "-";
static string map_type = "prefixNumber";
static char token = '@';

static void getArgs(int argc, const char* const argv[]);

int main(const int argc, const char* argv[]) {
   getArgs(argc, argv);

   if (verbose)
      cerr << "Using " << map_type << " with " << token << endl;

   iMagicStream input(input_file);
   string line;
   vector<string> converted_input;
   // Create an object that will wrap a Number mapper and make it compatible with split.
   // This way, all the mapping will occur while we are splitting the input sentence.
   NumberMapper::baseNumberMapper::functor  mapper(map_type, token);

   while (getline(input, line)) {
      // Processing a new sentence make sure we have a clean environment
      converted_input.clear();

      // Processes each word
      split(line.c_str(), converted_input, mapper);

      // Writes the result to stout
      copy(converted_input.begin(), converted_input.end(), ostream_iterator<string>(cout, " "));
      cout << endl;
   }

   // Free up the mapper
   mapper.free();
}

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "t:", "input:", "map:", "token:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 0, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("input", input_file);
   arg_reader.testAndSet("map", map_type);
   arg_reader.testAndSet("token", token);
}   
