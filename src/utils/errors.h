/**
 * @author George Foster
 * @file errors.h Exceptions and error handling. 
 * 
 * 
 * COMMENTS: 
 * 
 * This contains the Error exception class and the error() signal-and-abort
 * function. Since the latter requires including iostream (which implies
 * non-negligeable overhead), might want to split these up someday.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <string>
#include "portage_defs.h"

namespace Portage {

using std::string;

/**
 * Basic exception class.
 */
class Error {
   string msg;  ///< Error string

public:

   /// Construct with given error message.
   /// @param msg message describing the error
   Error(const string& msg) : msg(msg) {}

   /**
    * Construct with given error message spec, in printf (3) format.
    * @param fmt printf style message
    * @param ... arguments needed to format the string
    */
   Error(const char* fmt, ...);

   /// Get the error message.
   /// @return Error message
   const string& message() const {return msg;}
};

/**
 * Types of error.
 */
enum ErrorType {
   ETFatal,   ///< print "Error: <msg>" and exit 1
   ETWarn,    ///< print "Warning: <msg>" and return
   ETHelp     ///< print "<msg>" and exit 0
};

/**
 * Signal an error or warning message on cerr, and possibly quit.
 * @param et type of error: 
 *   ETFatal, print "Error: <msg>" and exit 1
 *   ETWarn, print "Warning: <msg>" and return
 *   ETHelp, print "<msg>" and exit 0
 * @param msg details of error
 */
extern void error(ErrorType et, const string& msg);

/**
 * Signal an error or warning message on cerr, and possibly quit.
 * @param et type of error: if ETFatal, exit after printing message.
 * @param fmt details of error, in printf (3) format.
 * @param ... arguments needed to format the string
 */
extern void error(ErrorType et, const char* fmt, ...);

}
#endif
