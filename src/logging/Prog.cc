/**
 * @author Samuel Larkin
 * @file Prog.cc  Example of program using argProcessor.
 * 
 * This file servers as a guide on how to use the argProcessor class
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include <exception_dump.h>
#include <errors.h>


using namespace Portage;
using namespace Portage::YourApplication;

////////////////////////////////////////////////////////////////////////////////////
// MAIN
//
int MAIN(argc, argv)
{
   ARG arg(argc, argv);

   // Now you have access to
   // arg.bVerbose
   // arg.bS1
   // arg.bS2
   // arg.sT1;
   // arg.sT2;
   //
   // and also to the loggers {verboseLogger | debugLogger | outputLogger} intended for use in the main function
   // and the more general logger Logging::{verboseRootLogger | debugRootLogger | outputRootLogger}
   // please don't use the general logger but branch yourself one from them like this
   // Logging::logger myLogger(Logging::getLogger(verbose.mylogger));
   //
   // LOG_INFO(verboseLogger, "Getting values");
   // LOG_DEBUG(debugLogger, "Some verbose info: %d");
   // LOG_ASSERT(verboseLogger, a bool condition, "some msg to be displayed and a string: %s", string.c_str());
   //
   // Uint monInt = function1(true);

} END_MAIN
