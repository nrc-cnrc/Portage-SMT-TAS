/**
 * @author Samuel Larkin
 * @file argProcessor.cc  Argument processor base class that takes care of loggers also.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include <argProcessor.h>
#include <iomanip>
#include <cstdlib> // exit()

using namespace Portage;

Logging::logger verboseLogger(Logging::getLogger("verbose.main")); ///< A global verbose logger for every application
Logging::logger debugLogger(Logging::getLogger("debug.main"));     ///< A global debug logger for every application
Logging::logger outputLogger(Logging::getLogger("output.main"));   ///< A global output logger for every application

string getLevelString(const Logging::level& level);

Portage::argProcessor::SWITCHES_DESC Portage::argProcessor::m_switches[] = {
   {"loggers", "Displays all available loggers and exits"},
   {"verbose:", "Verbose level"},
   {"debug", "Activates the debugging logger"},
   {"wfile:", "Watch File Name"},
   {"wtime:", "Watch File Delay"}};


argProcessor::argProcessor(Uint nNumberSwitches,
   const char* const switchesList[],
   int min,
   int max,
   const char* helpMsg,
   const char* helpSwitch,
   bool print_help_on_error,
   const char* alt_help_msg,
   const char* alt_help_switch)
   : mp_arg_reader(0)
   , m_nVerboseLevel(0)
   , m_logger(Logging::getLogger("debug.argProcessor"))
   , m_lDefaultWatchTime(60000)
{
   Logging::init();

   LOG_INFO(m_logger, "argProcessor constructor");

   // Adding default verbose switch to the list
   const char** switches = new const char*[nNumberSwitches+NUMBER_SWITCHES];
   for (Uint i(0); i<NUMBER_SWITCHES; ++i)
   {
      switches[nNumberSwitches+i] = m_switches[i].name;
   }

   for (Uint i(0); i<nNumberSwitches; ++i)
   {
      switches[i] = switchesList[i];
   }

   mp_arg_reader = new ArgReader(nNumberSwitches+NUMBER_SWITCHES, switches, min, max, helpMsg, helpSwitch, print_help_on_error, alt_help_msg, alt_help_switch);
   LOG_ASSERT(m_logger, mp_arg_reader != 0, "Unable to allocate buffer for SwitchesList");

   // TODO log not yet activated
   LOG_DEBUG(m_logger, "Number of switches: %d", nNumberSwitches+1);

   delete[] switches;
}


argProcessor::~argProcessor()
{
   if (mp_arg_reader != 0)
   {
      delete mp_arg_reader;
      mp_arg_reader = 0;
   }
}


void argProcessor::processArgs(Uint argc, const char* const argv[])
{
   LOG_INFO(m_logger, "Entering ProcessArg");

   mp_arg_reader->read(argc-1, argv+1);

   // DEBUG
   if (isEnabled(DEBUG))
   {
#ifndef NO_LOGGING
      debugLogger->setLevel(Logging::LogLevel::ACTIVATED);
#endif
   }

   // PROCESSING VERBOSE LEVEL
   if (isEnabled(VERBOSE))
   {
      mp_arg_reader->testAndSet(m_switches[VERBOSE].name, m_nVerboseLevel);
      if (m_nVerboseLevel > 9)
         m_nVerboseLevel = 9;
#ifndef NO_LOGGING
      verboseLogger->setLevel(Logging::LogLevel::toLevel(m_nVerboseLevel));
#endif
   }

   // LOGPROPERTIES FILE
   if (isEnabled(WFILE))
   {
      std::string sFileName;
      Uint lWatchTime(m_lDefaultWatchTime);

      mp_arg_reader->testAndSet(m_switches[WFILE].name, sFileName);
      mp_arg_reader->testAndSet(m_switches[WTIME].name, lWatchTime);

#ifndef NO_LOGGING
      log4cxx::PropertyConfigurator::configureAndWatch(sFileName, lWatchTime);
#endif
   }

   // LOGGERS
   if (isEnabled(LOGGERS))
   {
      displayLoggers();
      exit(0);
   }

   // CHILD ARGUMENT PARSING
   processArgs();
   printSwitchesValue();
}


// DISPLAY ALL AVAILABLE LOGGERS
void argProcessor::displayLoggers(ostream& os) const
{
#ifndef NO_LOGGING
   typedef std::map<log4cxx::String, log4cxx::AppenderPtr> AppenderMap;
   AppenderMap registry;

   // DISPLAYS SOME GENERAL INFO TO CHANGE THE INI FILE
   os << "# IMPORTANT there is two classes of levels: Level and LogLevel" << endl;
   os << "# ALL > DEBUG > INFO > WARN > ERROR > FATAL > OFF from #org.apache.log4j.Level" << endl;
   os << "#    syntaxe: WARN#org.apache.log4j.Level" << endl;
   os << "# VERBOSE1 < VERBOSE2 < ... < VERBOSE9 from #org.apache.log4j.LogLevel" << endl;
   os << "#    syntaxe: VERBOSE3#org.apache.log4j.LogLevel" << endl;
   os << "# ACTIVATED | DEACTIVATED from #org.apache.log4j.LogLevel" << endl;
   os << "#    syntaxe: ACTIVATED#org.apache.log4j.LogLevel" << endl;
   os << "# you can inherited the level by simply putting INHERITED without the qualifier" << endl;
   os << "# To change the appenders please refer to log4cxx help" << endl;
   os << "# also available as appenders are different file appenders" << endl;
   os << "# a blank appender was created to out unformatted logs" << endl;
   os << endl;

   // DISPLAYS LOGGERS
   os << "log4j.rootLogger = " << getLevelString(Logging::rootLogger->getLevel()) << endl << endl;
   log4cxx::LoggerList ll = Logging::rootLogger->getLoggerRepository()->getCurrentLoggers();

   // Get max width for pretty print
   Uint maxWidth(0);
   {
      log4cxx::LoggerList::const_iterator itl(ll.begin());
      for (; itl!=ll.end(); ++itl)
         if (maxWidth < (*itl)->getName().size())
            maxWidth = (*itl)->getName().size();
   }

   log4cxx::LoggerList::const_iterator itl(ll.begin());
   for (; itl!=ll.end(); ++itl)
   {
      os << "log4j.logger." << (*itl)->getName() << std::setw(maxWidth-(*itl)->getName().size()+1) << " " << "= " << getLevelString((*itl)->getLevel());

      log4cxx::AppenderList al = (*itl)->getAllAppenders();
      log4cxx::AppenderList::const_iterator ita(al.begin());
      for (; ita!=al.end(); ++ita)
      {
         os << ", " << (*ita)->getName();
         registry[(*ita)->getName()] = (*ita);
      }
      os << endl;
   }
   os << endl;

   // DISPLAYS APPENDERS AND THEIR LAYOUTS
   AppenderMap::const_iterator  ita(registry.begin());
   char szBuffer[1024];
   for (; ita!=registry.end(); ++ita)
   {
      const log4cxx::AppenderPtr& appender = ita->second;
      snprintf(szBuffer, sizeof(szBuffer), "log4j.appender.%s", appender->getName().c_str());
      os << szBuffer << " = org.apache.log4j." << appender->getClass().getName() << endl;

      if (appender->getClass().getName() == "ConsoleAppender")
      {
         log4cxx::ConsoleAppenderPtr console = appender;
         os << szBuffer << ".target = " << console->getTarget() << endl;
      }

      // LAYOUTS
      if (appender->requiresLayout())
      {
         const log4cxx::LayoutPtr& layout = appender->getLayout();
         os << szBuffer << ".layout = org.apache.log4j." << layout->getClass().getName() << endl;
      }
      os << endl;
   }

   // DISPLAYS LOGLEVEL
   os << "# Custom log level are verbose1 to verbose9:" << endl;
   os << "# " << getLevelString(Logging::LogLevel::VERBOSE1) << endl;
   os << endl;

   // EXAMPLE OF SIMPLE FILE APPENDER
   os << "# How to setup a file appender" << endl;
   os << "# log4j.appender.F=org.apache.log4j.FileAppender" << endl;
   os << "# log4j.appender.F.File=file.log" << endl;
   os << "# log4j.appender.F.layout = org.apache.log4j.SimpleLayout" << endl;
   os << "# log4j.logger.debug = DEBUG, F" << endl;
   os << endl;

   // EXAMPLE OF ROLLING FILE APPENDER
   os << "# How to setup a rolling appender 100k file with one backup file" << endl;
   os << "# log4j.appender.R=org.apache.log4j.RollingFileAppender" << endl;
   os << "# log4j.appender.R.File=rollingFile.log" << endl;
   os << "# log4j.appender.R.MaxFileSize=100KB" << endl;
   os << "# Keep one backup file" << endl;
   os << "# log4j.appender.R.MaxBackupIndex=1" << endl;
   os << "# log4j.appender.R.layout=org.apache.log4j.PatternLayout#" << endl;
   os << "# log4j.appender.R.layout.ConversionPattern=%p %t %c - %m%n" << endl;
   os << "# log4j.logger.debug = DEBUG, R, F" << endl;
   os << endl;

   // LOGGING CHILD TO MULTIPLE FILES
   os << "# Example parent logs in F1 and child in F1 and F2" << endl;
   os << "# log4j.logger.parent    = DEBUG, F1" << endl;
   os << "# log4j.logger.child     = INFO, F2" << endl;
   os << endl;

   // BREAKING LINK BETWEEN PARENT AND CHILDREN
   os << "# Example parent logs in F1 and child in F2 only" << endl;
   os << "# log4j.logger.parent    = DEBUG, F1" << endl;
   os << "# log4j.logger.child     = INFO, F2" << endl;
   os << "# log4j.additivity.child = FALSE"<< endl;
   os << endl;
#endif
}


// DISPLAYS HELP FOR BASE ARG OBJ
void argProcessor::displayHelp(ostream& os) const
{
   for (Uint i(0); i<NUMBER_SWITCHES; ++i)
   {
      os << "- " << m_switches[i].name << " => " << m_switches[i].desc << endl;
   }
}


// Set the verbose level
void argProcessor::setVerboseLevel(Uint level)
{
   m_nVerboseLevel = level;
#ifndef NO_LOGGING
   verboseLogger->setLevel(Logging::LogLevel::toLevel(m_nVerboseLevel));
#endif
}


void argProcessor::printSwitchesValue()
{
   LOG_INFO(m_logger, "argProcessor::printSwitchesValue");
}


////////////////////////////////////////////////////////////////////////////////
// CLASS HELPERS FUNCTION
string getLevelString(const Logging::level& level)
{
   if (level != 0)
   {
      ostringstream oss;
#ifdef NO_LOGGING
      oss << "LOGGING deactivated at compile";
#else
      oss << level->toString() << "#org.apache.log4j." << level->getClass().getName();
#endif
      return oss.str();
   }

   // TODO inhereted instead of off
   return "INHERITED";
}


bool argProcessor::checkVerbose(const char* const _switch)
{
   vector<string> allVerbose;
   mp_arg_reader->testAndSet(_switch, allVerbose);
   setVerboseLevel(allVerbose.size());

   return getVerboseLevel() > 0;
}
