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
 * This namespace encapsulates the callbacks that do the real work for
 * Portage::error().  Intended for unit testing, this callback mechanism allows
 * the disabling or changing of error()'s default behaviour.
 */
namespace Error_ns {
   /// This callback does nothing, effectively disabling error().
   void nullErrorCallBack(ErrorType et, const string& msg);
   /// counter variables for the count error callback
   namespace ErrorCounts {
      extern Uint Fatal;
      extern Uint Warn;
      extern Uint Help;
      extern Uint Total;
      extern string last_msg;
      inline void clear() { Fatal = Warn = Help = Total = 0; last_msg.clear(); }
   };
   /// This callback counts errors, also disabling error() but having it
   /// leave a trace -- useful for unit testing.
   void countErrorCallBack(ErrorType et, const string& msg);
   /// This default call back does the normal work advertized in the
   /// documentation of error().
   void defaultErrorCallBack(ErrorType et, const string& msg);
   /// This call back uses abort() instead of exit(1) on error
   void abortOnErrorCallBack(ErrorType et, const string& msg);
   /// The signature required of a valid errorCallback.
   typedef void (*ErrorCallback)(ErrorType et, const string& msg);
   namespace Current {
      /// This static function pointer holds the error callback currently in
      /// effect.  Change its value to change what error() does.
      extern ErrorCallback errorCallback;
   };
}

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
