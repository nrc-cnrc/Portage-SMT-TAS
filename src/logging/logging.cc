/**
 * @author Samuel Larkin
 * @file logging.cc  Logging activation.
 *
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */
        
#include "logging.h"

using namespace Portage;

#ifdef NO_LOGGING
// Define a static logger variable so that it references
Logging::logger Portage::Logging::rootLogger        = Logging::getLogger("Some.root.logger");
Logging::logger Portage::Logging::verboseRootLogger = Logging::getLogger("verbose");
Logging::logger Portage::Logging::debugRootLogger   = Logging::getLogger("debug");
Logging::logger Portage::Logging::outputRootLogger  = Logging::getLogger("output");

void Logging::init()
{ }
#else
// Define a static logger variable so that it references
Logging::logger Portage::Logging::rootLogger        = log4cxx::Logger::getRootLogger();
Logging::logger Portage::Logging::verboseRootLogger = Logging::getLogger("verbose");
Logging::logger Portage::Logging::debugRootLogger   = Logging::getLogger("debug");
Logging::logger Portage::Logging::outputRootLogger  = Logging::getLogger("output");

void Logging::init()
{
   using namespace log4cxx;

   // Guard to prevent multiple initialization
   static bool bHasBeenInit(false);

   if (!bHasBeenInit)
   {
      // Set up a simple configuration that logs on the console.
      //
      rootLogger->setLevel(Level::ERROR);
      verboseRootLogger->setLevel(Level::OFF);
      outputRootLogger->setLevel(Level::OFF);
      debugRootLogger->setLevel(Level::OFF);

      // TODO verify the STDERR ConsoleAppender
      AppenderPtr stdout = new ConsoleAppender(new SimpleLayout());
      stdout->setName("stdout");
      AppenderPtr stderr = new ConsoleAppender(new SimpleLayout(),
         ConsoleAppender::SYSTEM_ERR);
      stderr->setName("stderr");
      AppenderPtr stdblank = new ConsoleAppender(new BlankLayout());
      stdblank->setName("stdblank");
      AppenderPtr stderrblank = new ConsoleAppender(new BlankLayout(),
         ConsoleAppender::SYSTEM_ERR);
      stderrblank->setName("stderrblank");

      verboseRootLogger->addAppender(stderrblank);
      outputRootLogger->addAppender(stdblank);
      debugRootLogger->addAppender(stderr);

      // TODO remove HACK
      // Temporary linking hack to include to symbols to be able to use them in the logger.properties
      ConsoleAppender ca;
      FileAppender fa;
      BlankLayout bl;
      SimpleLayout sl;
      PatternLayout pl;

      bHasBeenInit = true;
   }
}
#endif //LOGGING

