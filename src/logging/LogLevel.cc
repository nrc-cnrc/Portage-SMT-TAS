/**
 * @author Samuel Larkin
 * @file LogLevel.cc  Specific log level for portage.
 *
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef NO_LOGGING

#include "LogLevel.h"
#include <log4cxx/helpers/stringhelper.h>

using namespace Portage::Logging;
using namespace log4cxx;
using namespace log4cxx::helpers;

IMPLEMENT_LOG4CXX_LEVEL(LogLevel)

#define VERBOSE1_STR _T("VERBOSE1")
#define VERBOSE2_STR _T("VERBOSE2")
#define VERBOSE3_STR _T("VERBOSE3")
#define VERBOSE4_STR _T("VERBOSE4")
#define VERBOSE5_STR _T("VERBOSE5")
#define VERBOSE6_STR _T("VERBOSE6")
#define VERBOSE7_STR _T("VERBOSE7")
#define VERBOSE8_STR _T("VERBOSE8")
#define VERBOSE9_STR _T("VERBOSE9")
#define ACTIVATED_STR _T("ACTIVATED")
#define DEACTIVATED_STR _T("DEACTIVATED")

const LevelPtr LogLevel::VERBOSE1 = new LogLevel(LogLevel::VERBOSE1_INT, VERBOSE1_STR, 6);
const LevelPtr LogLevel::VERBOSE2 = new LogLevel(LogLevel::VERBOSE2_INT, VERBOSE2_STR, 6);
const LevelPtr LogLevel::VERBOSE3 = new LogLevel(LogLevel::VERBOSE3_INT, VERBOSE3_STR, 6);
const LevelPtr LogLevel::VERBOSE4 = new LogLevel(LogLevel::VERBOSE4_INT, VERBOSE4_STR, 6);
const LevelPtr LogLevel::VERBOSE5 = new LogLevel(LogLevel::VERBOSE5_INT, VERBOSE5_STR, 6);
const LevelPtr LogLevel::VERBOSE6 = new LogLevel(LogLevel::VERBOSE6_INT, VERBOSE6_STR, 6);
const LevelPtr LogLevel::VERBOSE7 = new LogLevel(LogLevel::VERBOSE7_INT, VERBOSE7_STR, 6);
const LevelPtr LogLevel::VERBOSE8 = new LogLevel(LogLevel::VERBOSE8_INT, VERBOSE8_STR, 6);
const LevelPtr LogLevel::VERBOSE9 = new LogLevel(LogLevel::VERBOSE9_INT, VERBOSE9_STR, 6);
const LevelPtr LogLevel::ACTIVATED = new LogLevel(LogLevel::ACTIVATED_INT, ACTIVATED_STR, 1);
const LevelPtr LogLevel::DEACTIVATED = new LogLevel(LogLevel::DEACTIVATED_INT, DEACTIVATED_STR, 1);

LogLevel::LogLevel(int level, const String& levelStr, int syslogEquivalent)
: Level(level, levelStr, syslogEquivalent)
{
}

LogLevel::~LogLevel()
{
}

const LevelPtr& LogLevel::toLevel(const String& sArg)
{
   return toLevel(sArg, VERBOSE1);
}

const LevelPtr& LogLevel::toLevel(int val)
{
   return toLevel(val, VERBOSE1);
}

const LevelPtr& LogLevel::toLevel(int val, const LevelPtr& defaultLevel)
{
   switch(val)
   {
      case 1: return VERBOSE1;
      case 2: return VERBOSE2;
      case 3: return VERBOSE3;
      case 4: return VERBOSE4;
      case 5: return VERBOSE5;
      case 6: return VERBOSE6;
      case 7: return VERBOSE7;
      case 8: return VERBOSE8;
      case 9: return VERBOSE9;

      case VERBOSE1_INT: return VERBOSE1;
      case VERBOSE2_INT: return VERBOSE2;
      case VERBOSE3_INT: return VERBOSE3;
      case VERBOSE4_INT: return VERBOSE4;
      case VERBOSE5_INT: return VERBOSE5;
      case VERBOSE6_INT: return VERBOSE6;
      case VERBOSE7_INT: return VERBOSE7;
      case VERBOSE8_INT: return VERBOSE8;
      case VERBOSE9_INT: return VERBOSE9;

      case ACTIVATED_INT: return ACTIVATED;
      case DEACTIVATED_INT: return DEACTIVATED;

      default: return defaultLevel;
   }
}

const LevelPtr& LogLevel::toLevel(const String& sArg, const LevelPtr& defaultLevel)
{
   if (sArg.empty())
    {
       return defaultLevel;
    }

    String s = StringHelper::toUpperCase(sArg);

    if(s == (VERBOSE1_STR)) return VERBOSE1;
    if(s == (VERBOSE2_STR)) return VERBOSE2;
    if(s == (VERBOSE3_STR)) return VERBOSE3;
    if(s == (VERBOSE4_STR)) return VERBOSE4;
    if(s == (VERBOSE5_STR)) return VERBOSE5;
    if(s == (VERBOSE6_STR)) return VERBOSE6;
    if(s == (VERBOSE7_STR)) return VERBOSE7;
    if(s == (VERBOSE8_STR)) return VERBOSE8;
    if(s == (VERBOSE9_STR)) return VERBOSE9;
    if(s == (ACTIVATED_STR)) return ACTIVATED;
    if(s == (DEACTIVATED_STR)) return DEACTIVATED;

    return defaultLevel;
}

#endif // NO_LOGGING
