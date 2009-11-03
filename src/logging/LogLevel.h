/**
 * @author Samuel Larkin
 * @file LogLevel.h  New log level for log4cxx.
 *
 *
 * COMMENT: Specific log level for portage
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __PORTAGE_LOGLEVEL_H__
#define __PORTAGE_LOGLEVEL_H__

#ifndef NO_LOGGING

#include <log4cxx/level.h>

namespace Portage
{
   namespace Logging
   {
      using namespace log4cxx;

      /**
       * Adding level of granularity for verbose levels.
       * VERBOSE1 \f$\subset\f$ VERBOSE2 \f$\subset \ldots \subset\f$ VERBOSE9.
       * Thus when enabling VERBOSEX, LogLevel lower then X are also activated.
       */
      class LogLevel : public log4cxx::Level
      {
         DECLARE_LOG4CXX_LEVEL(LogLevel)

         public:
           /// New log level numerical values.
           enum LogLevelInt
           {
               VERBOSE9_INT = log4cxx::Level::INFO_INT + 1000,
               VERBOSE8_INT = log4cxx::Level::INFO_INT + 2000,
               VERBOSE7_INT = log4cxx::Level::INFO_INT + 3000,
               VERBOSE6_INT = log4cxx::Level::INFO_INT + 4000,
               VERBOSE5_INT = log4cxx::Level::INFO_INT + 5000,
               VERBOSE4_INT = log4cxx::Level::INFO_INT + 6000,
               VERBOSE3_INT = log4cxx::Level::INFO_INT + 7000,
               VERBOSE2_INT = log4cxx::Level::INFO_INT + 8000,
               VERBOSE1_INT = log4cxx::Level::INFO_INT + 9000,

               DEACTIVATED_INT = INT_MAX,   ///< Special value to deactivated all level of logging
               ACTIVATED_INT   = INT_MIN    ///< Special value to activated all level of logging
            };

            static const log4cxx::LevelPtr VERBOSE1;
            static const log4cxx::LevelPtr VERBOSE2;
            static const log4cxx::LevelPtr VERBOSE3;
            static const log4cxx::LevelPtr VERBOSE4;
            static const log4cxx::LevelPtr VERBOSE5;
            static const log4cxx::LevelPtr VERBOSE6;
            static const log4cxx::LevelPtr VERBOSE7;
            static const log4cxx::LevelPtr VERBOSE8;
            static const log4cxx::LevelPtr VERBOSE9;
            static const log4cxx::LevelPtr ACTIVATED;
            static const log4cxx::LevelPtr DEACTIVATED;

            /**
             * Constructor.
             * @param level the numerical value of the LogLevel
             * @param levelStr LogLevel's name
             * @param syslogEquivalent system log equivalent
             */
            LogLevel(int level, const String& levelStr, int syslogEquivalent);

            /// Destructor.
            virtual ~LogLevel();

            /**
             * Convert the string passed as argument to a level. If the
             * conversion fails, then this method returns \#DEBUG.
             * @param sArg string to convert to a LogLevel
             */
            static const log4cxx::LevelPtr& toLevel(const String& sArg);

            /**
             * Convert an integer passed as argument to a level. If the
             * conversion fails, then this method returns \#DEBUG.
             * @param val numerical LogLevel value to convert to a LogLevel
             */
            static const log4cxx::LevelPtr& toLevel(int val);

            /**
             * Convert an integer passed as argument to a level. If the
             * conversion fails, then this method returns the specified
             * default.
             * @param val numerical LogLevel value to convert to a LogLevel
             * @param defaultLevel Default LogLevel fallback
             */
            static const log4cxx::LevelPtr& toLevel(int val, const log4cxx::LevelPtr& defaultLevel);


            /**
             * Convert the string passed as argument to a level. If the
             * conversion fails, then this method returns the value of
             * defaultLevel.
             * @param sArg string to convert to a LogLevel
             * @param defaultLevel Default LogLevel fallback
             */
            static const log4cxx::LevelPtr& toLevel(const String& sArg, const log4cxx::LevelPtr& defaultLevel);
      };
   } // END LOGGING
} // END PORTAGE

#endif // NO_LOGGING
#endif   // __PORTAGE_LOGLEVEL_H__
