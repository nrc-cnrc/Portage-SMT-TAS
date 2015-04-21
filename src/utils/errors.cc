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

Error_ns::ErrorCallback Error_ns::Current::errorCallback = Error_ns::defaultErrorCallBack;

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

Uint Portage::Error_ns::ErrorCounts::Fatal = 0;
Uint Portage::Error_ns::ErrorCounts::Warn = 0;
Uint Portage::Error_ns::ErrorCounts::Help = 0;
Uint Portage::Error_ns::ErrorCounts::Total = 0;
string Portage::Error_ns::ErrorCounts::last_msg;
void Portage::Error_ns::countErrorCallBack(ErrorType et, const string& msg)
{
   if (et == ETFatal)
      ++ErrorCounts::Fatal;
   else if (et == ETWarn)
      ++ErrorCounts::Warn;
   else
      ++ErrorCounts::Help;
   ++Error_ns::ErrorCounts::Total;
   Error_ns::ErrorCounts::last_msg = msg;
}

void Portage::Error_ns::defaultErrorCallBack(ErrorType et, const string& msg)
{
   if (et == ETFatal) {
      cerr << "Error: " << msg << endl;
      exit(1);
      assert(false);
   } else if (et == ETWarn) {
      cerr << "Warning: " << msg << endl;
   } else {
      cerr << msg << endl;
      exit(0);
      assert(false);
   }
}

void Portage::Error_ns::abortOnErrorCallBack(ErrorType et, const string& msg)
{
   if (et == ETFatal) {
      cerr << "Error: " << msg << endl;
      abort();
      assert(false);
   } else if (et == ETWarn) {
      cerr << "Warning: " << msg << endl;
   } else {
      cerr << msg << endl;
      exit(0);
      assert(false);
   }
}

void Portage::error(ErrorType et, const string& msg)
{
   if (Error_ns::Current::errorCallback) {
      (*Error_ns::Current::errorCallback)(et, msg);
   }
   #ifdef __KLOCWORK__
      if (et == ETFatal) abort();
   #endif
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

