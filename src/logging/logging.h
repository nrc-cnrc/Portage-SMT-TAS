/**
 * @author Samuel Larkin
 * @file logging.h  Basic logging functions.
 *
 *
 * COMMENT: Main file to include for logging
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __PORTAGE_LOGGER_H__
#define __PORTAGE_LOGGER_H__

#ifdef NO_LOGGING
   #include <sstream>
#else
   // include log4cxx header files.
   #include <log4cxx/logger.h>
   #include <log4cxx/propertyconfigurator.h>
   #include <log4cxx/consoleappender.h>
   #include <log4cxx/fileappender.h>
   #include <log4cxx/simplelayout.h>
   #include <log4cxx/patternlayout.h>
   #include <log4cxx/helpers/exception.h>
   #include "blanklayout.h"
   #include "LogLevel.h"
   #include <stdarg.h>
#endif

#include <assert.h>

namespace Portage
{
#ifdef NO_LOGGING
   // Define macros for logging
   // s... is the ellipsis named s and to prevent an error from the compiler if there is no ellipsis
   // we must use ## s that is ##<with_one_spcae>s a fix from the gnu compiler that will make the proper
   // macro by removing the extra comma
   // example: LOG(mylogger, "message without ellipsis")
   //    will result in the correct substitution:
   //       Logging::log(__FILE__, __LNE__, mylogger, Logging:LogLevel::VERBOSE1, "message without ellipsis");
   //    and not the invalid version (compiler error):
   //       Logging::log(__FILE__, __LNE__, mylogger, Logging:LogLevel::VERBOSE1, "message without ellipsis", );
   #define LOG(logger, msg, s...);
   #define LOG_DEBUG(logger, msg, s...);
   #define LOG_INFO(logger, msg, s...);
   #define LOG_WARN(logger, msg, s...);
   #define LOG_ERROR(logger, msg, s...);
   #define LOG_FATAL(logger, msg, s...);


   #define LOG_VERBOSE1(logger, msg, s...);
   #define LOG_VERBOSE2(logger, msg, s...);
   #define LOG_VERBOSE3(logger, msg, s...);
   #define LOG_VERBOSE4(logger, msg, s...);
   #define LOG_VERBOSE5(logger, msg, s...);
   #define LOG_VERBOSE6(logger, msg, s...);
   #define LOG_VERBOSE7(logger, msg, s...);
   #define LOG_VERBOSE8(logger, msg, s...);
   #define LOG_VERBOSE9(logger, msg, s...);

   #define LOG_ASSERT(logger, assertion, pattern, s...) assert(#assertion);


   namespace Logging
   {
      /// Stub when not using/compiling logging.
      class dumbLogger
      {
         public:
            /// Stub when not using/compiling logging.
            template <class T>
            void setLevel(const T& t) {}
            /// Stub when not using/compiling logging.
            inline bool isDebugEnabled() {return false;}
      };
      typedef unsigned    level;   ///< stub when not using/compiling logging
      typedef dumbLogger* logger;  ///< stub when not using/compiling logging

      /// Stub when not using/compiling logging.
      inline logger getLogger(const char* const loggerName) {
         static dumbLogger dumb;
         return &dumb;
      }

      // Defines a globally accessible logger variable
      // Must be extern since its definition is in the logging.cc
      extern logger rootLogger;            ///< Globally accessible root logger
      extern logger verboseRootLogger;     ///< Globally accessible verbose logger
      extern logger debugRootLogger;       ///< Globally accessible debug logger
      extern logger outputRootLogger;      ///< Globally accessible output logger

      /// Stub when not using/compiling logging.
      inline void log(const char* const filename,
         const int nline,
         logger& aLogger,
         const level& aLevel,
         const char* const pattern,
         ...)
      { }

      /// Stub when not using/compiling logging.
      void init();
   } // END Logging
#else  // LOGGING
   // Define macros for logging
   // s... is the ellipsis named s and to prevent an error from the compiler if there is no ellipsis
   // we must use ## s that is ##<with_one_spcae>s a fix from the gnu compiler that will make the proper
   // macro by removing the extra comma
   // example: LOG(mylogger, "message without ellipsis")
   //    will result in the correct substitution:
   //       Logging::log(__FILE__, __LNE__, mylogger, Logging:LogLevel::VERBOSE1, "message without ellipsis");
   //    and not the invalid version (compiler error):
   //       Logging::log(__FILE__, __LNE__, mylogger, Logging:LogLevel::VERBOSE1, "message without ellipsis", );
   //@{
   //!  Logging made simpler for different level of severity.
   #define LOG(logger, msg, s...)       Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::ACTIVATED, msg, ## s);
   #define LOG_DEBUG(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, log4cxx::Level::DEBUG, msg, ## s);
   #define LOG_INFO(logger, msg, s...)  Logging::log(__FILE__, __LINE__, logger, log4cxx::Level::INFO, msg, ## s);
   #define LOG_WARN(logger, msg, s...)  Logging::log(__FILE__, __LINE__, logger, log4cxx::Level::WARN, msg, ## s);
   #define LOG_ERROR(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, log4cxx::Level::ERROR, msg, ## s);
   #define LOG_FATAL(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, log4cxx::Level::FATAL, msg, ## s);


   #define LOG_VERBOSE1(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE1, msg, ## s);
   #define LOG_VERBOSE2(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE2, msg, ## s);
   #define LOG_VERBOSE3(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE3, msg, ## s);
   #define LOG_VERBOSE4(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE4, msg, ## s);
   #define LOG_VERBOSE5(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE5, msg, ## s);
   #define LOG_VERBOSE6(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE6, msg, ## s);
   #define LOG_VERBOSE7(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE7, msg, ## s);
   #define LOG_VERBOSE8(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE8, msg, ## s);
   #define LOG_VERBOSE9(logger, msg, s...) Logging::log(__FILE__, __LINE__, logger, Logging::LogLevel::VERBOSE9, msg, ## s);

   #define LOG_ASSERT(logger, assertion, pattern, s...) { \
      if (!(assertion)) { \
         if (strlen(pattern) > 0) { \
            char szBuffer[1024]; \
            snprintf(szBuffer, sizeof(szBuffer)-2, pattern, ## s); \
            szBuffer[1023] = '\0'; \
            ::log4cxx::StringBuffer oss; \
            oss << "ASSERTION FAILED (" << __FILE__ << ":" << __LINE__ << "): "<< #assertion << " => " << szBuffer; \
            logger->assertLog(false, oss.str()); \
            assert(!#assertion);}}}
   //@}


   namespace Logging
   {
      typedef log4cxx::LoggerPtr logger;   ///< Portage logger definition
      typedef log4cxx::LevelPtr  level;    ///< Portage level definition

      /**
       * Portage definition to get a logger by it's name.
       * @param loggerName name of the logger to retrieve
       */
      inline logger getLogger(const char* const loggerName)
      {
         return log4cxx::Logger::getLogger(loggerName);
      }

      // Defines a globally accessible logger variable
      // Must be extern since its definition is in the logging.cc
      extern logger rootLogger;                ///< Globally accessible root logger
      extern logger verboseRootLogger;         ///< Globally accessible verbose logger
      extern logger debugRootLogger;           ///< Globally accessible debug logger
      extern logger outputRootLogger;          ///< Globally accessible output logger

      /**
       * Logs a message.
       * This function is not intended to be used directly but rather
       * servers as a single function for all the LOG* MACROS
       * @param filename file name from which the log is emitted
       * @param nline line number from which the log is emitted
       * @param aLogger a logger in which to log the event
       * @param aLevel severity of log event
       * @param pattern printf style message to log
       * @param ... the parameters for pattern
       */
      inline void log(const char* const filename,
         const int nline,
         logger& aLogger,
         const level& aLevel,
         const char* const pattern,
         ...)
      {
         if (aLogger->isEnabledFor(aLevel))
         {
            if (strlen(pattern) > 0)
            {
               char szBuffer[1024];
               va_list params;
               va_start(params, pattern);
               vsnprintf(szBuffer, sizeof(szBuffer)-2, pattern, params);
               szBuffer[1023] = '\0';
               va_end(params);
               aLogger->forcedLog(aLevel, szBuffer, filename, nline);
            }
         }
      }

      /// Initialises the logging mechanism.
      /// Must be called to setup the default logging mechanism of Portage.
      void init();
   } // END Logging
#endif // NO_LOGGING
} // END Portage

#endif   // __PORTAGE_LOGGER_H__
