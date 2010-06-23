/**
 * @author George Foster
 * @file errors.cc  Exceptions and error handling.
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

#include "errors.h"
#include <vector>
#include <stdarg.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>

using namespace Portage;
using namespace std;

Error_ns::ErrorCallback Error_ns::dummy::errorCallback = Error_ns::defaultErrorCallBack;

Error::Error(const char* fmt, ...)
{
   vector<char> buf(1000);
   va_list arglist;
   va_start(arglist, fmt);
   vsnprintf(&buf[0], buf.size(), fmt, arglist);
   va_end(arglist);
   msg = &buf[0];
}

void Portage::Error_ns::nullErrorCallBack(ErrorType et, const string& msg) {}

void Portage::Error_ns::defaultErrorCallBack(ErrorType et, const string& msg)
{
   if (et == ETFatal) {
      cerr << "Error: " << msg << endl;
      exit(1);
   } else if (et == ETWarn) {
      cerr << "Warning: " << msg << endl;
   } else {
      cerr << msg << endl;
      exit(0);
   }
}

void Portage::error(ErrorType et, const string& msg)
{
   if (Error_ns::dummy::errorCallback) {
      (*Error_ns::dummy::errorCallback)(et, msg);
   }
}

void Portage::error(ErrorType et, const char* fmt, ...)
{
   vector<char> buf(5000);
   va_list arglist;
   va_start(arglist, fmt);
   vsnprintf(&buf[0], buf.size(), fmt, arglist);
   va_end(arglist);
   string msg(&buf[0]);

   error(et, msg);
}

