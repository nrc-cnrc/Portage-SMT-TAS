/**
 * @author Samuel Larkin
 * @file logging.cc  Logging activation.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
        
#include "logging.h"
#include <fstream>

using namespace Portage;

#ifdef NO_LOGGING
// Define a static logger variable so that it references
Logging::logger Portage::Logging::rootLogger        = Logging::getLogger("Some.root.logger");
Logging::logger Portage::Logging::verboseRootLogger = Logging::getLogger("verbose");
Logging::logger Portage::Logging::debugRootLogger   = Logging::getLogger("debug");
Logging::logger Portage::Logging::outputRootLogger  = Logging::getLogger("output");

const int Logging::LogLevel::VERBOSE1 = 1;
const int Logging::LogLevel::VERBOSE2 = 2;
const int Logging::LogLevel::VERBOSE3 = 3;
const int Logging::LogLevel::VERBOSE4 = 4;
const int Logging::LogLevel::VERBOSE5 = 5;
const int Logging::LogLevel::VERBOSE6 = 6;
const int Logging::LogLevel::VERBOSE7 = 7;
const int Logging::LogLevel::VERBOSE8 = 8;
const int Logging::LogLevel::VERBOSE9 = 9;

const int Logging::Level::ON  = 0;
const int Logging::Level::OFF = 1;

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

#ifndef NO_LOGGING
      std::ifstream is;
      if (is.open(".log4cxx.properties.automatic"), is) {
         log4cxx::PropertyConfigurator::configure(".log4cxx.properties.automatic");
         is.close();
      } 
#endif

      bHasBeenInit = true;
   }
}
#endif //LOGGING

