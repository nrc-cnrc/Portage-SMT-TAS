/**
 * @author George Foster
 * @file errors.cc  Exceptions and error handling.
 * 
 * 
 * COMMENTS: 
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include "errors.h"

using namespace Portage;
using namespace std;

Error::Error(const char* fmt, ...)
{
   vector<char> buf(1000);
   va_list arglist;
   va_start(arglist, fmt);
   vsnprintf(&buf[0], buf.size(), fmt, arglist);
   va_end(arglist);
   msg = &buf[0];
}

void Portage::error(ErrorType et, const string& msg)
{
   string hdr;
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

