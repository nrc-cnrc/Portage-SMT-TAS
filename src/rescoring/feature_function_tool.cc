/**
 * @author Samuel Larkin
 * @file feature_function_tool.cc 
 * @brief Program that handles querying of a rescoring-model.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "exception_dump.h"
#include "arg_reader.h"
#include "logging.h"
#include "file_utils.h"
#include "featurefunction_set.h"
#include <printCopyright.h>

using namespace Portage;
using namespace std;

/// Program gen_feature_values usage.
static char help_message[] = "\n\
feature_function_tool [-v][-p prefix] -check rescoring-model\n\
\n\
   Verifies the rescoring-model integrity.\n\
\n\
or \n\
feature_function_tool [-v] <-c | -a> feature arg\n\
\n\
   Check if the feautre requires the alignment.\n\
   Returns the feature's complexity\n\
\n\
Options:\n\
\n\
-v   Write progress reports to cerr.\n\
-c   What is the complexity of feature.\n\
-a   Does feature requires the alignment.\n\
-p   Feature function prefix.\n\
";

// main
int MAIN(argc, argv)
{
   printCopyright(2007, "feature_function_tool");

   // Do this here until we use argProcessor for this program.
   Logging::init();

   const char* switches[] = {"v", "c", "a", "check:", "p:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 2, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   bool   verbose = false;
   bool   bComplexity = false;
   bool   bAlignment = false;
   string rescoring_model;

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("c", bComplexity);
   arg_reader.testAndSet("a", bAlignment);
   arg_reader.testAndSet("check", rescoring_model);

   if (bComplexity || bAlignment) {
      if (arg_reader.numVars() != 2)
         error(ETFatal, "You need to specify feature and arg");
      string name;
      string argument;
      arg_reader.testAndSet(0, "feature", name);
      arg_reader.testAndSet(1, "arg", argument);

      FeatureFunctionSet ffset;
      // feature ahs no prefix, is not dynamic, uses NullDeleter, doesn't load models
      ptr_FF  ff(FeatureFunctionSet::create(name, argument, NULL, false, true, false));
      if (!ff)
         error(ETFatal, "unknown feature: %s", name.c_str());

      if (bComplexity) {
         cout << "Complexity: " << (Uint)ff->cost();
         if (verbose) cout << " " << ff->complexitySting();
         cout << endl;
      }
      if (bAlignment) 
         cout << "Alignment: " << (ff->requires() & FF_NEEDS_ALIGNMENT ? "TRUE" : "FALSE") << endl;
   }
   else if (!rescoring_model.empty()) {
      if (arg_reader.numVars() != 0)
         error(ETFatal, "You don't need to specify feature and arg");
      string prefix;
      arg_reader.testAndSet("p", prefix);
      FeatureFunctionSet ffset;
      // the ffset is not dynamic, doesn't use NullDeleter, doesn't load models
      const Uint numFF = ffset.read(rescoring_model, verbose, prefix.c_str(), false, false, false);
      printf("Found %d feature functions in %s\n", numFF, rescoring_model.c_str());
      if (ffset.check())
         cout << "Ok" << endl;
      else
         cout << "Some error(s) occurred, please see previous warning" << endl;
   }
} END_MAIN
