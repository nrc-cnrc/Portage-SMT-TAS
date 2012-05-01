/**
 * @author George Foster
 * @file casemap_strings.cc
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include "casemap_strings.h"
#include "errors.h"
#include <stdexcept>
#include <cstring>
#include <iostream>

using namespace Portage;

void CaseMapStrings::init()
{
   if (loc->name().find("utf8", 0) != string::npos ||
       loc->name().find("utf-8", 0) != string::npos ||
       loc->name().find("UTF8", 0) != string::npos ||
       loc->name().find("UTF-8", 0) != string::npos) {
#ifndef NOICU
      utf8 = new UTF8Utils();
#else
      error(ETFatal,
         "Compilation with ICU was disabled at build time.\n"
         "To use casemapping with a utf8 locale, install ICU, edit the ICU variable in\n"
         "PORTAGEshared/src/Makefile.user-conf and recompile.");
#endif
   }
}

CaseMapStrings::CaseMapStrings(const char* loc_name) :
   loc(NULL), utf8(NULL)
{
   try {
      if ( loc_name && loc_name[0] != '\0' ) {
#ifdef Darwin
         if ((strcmp(loc_name, "C") != 0) && (strcmp(loc_name, "POSIX") != 0)) {
            error(ETFatal, "Locale name %s is not valid. C++ locale class on Darwin supports only C and POSIX locales.", loc_name);
            return;
         }
#endif
         loc = new locale(loc_name);
      } else
         loc = new locale("POSIX");
      init();
   } catch (std::runtime_error& e) {
      error(ETFatal, "Locale name %s is not valid. See /usr/lib/locale for a list of valid locales.", loc_name);
   }
}

CaseMapStrings::CaseMapStrings(const locale& loc) :
   loc(NULL), utf8(NULL)
{
   this->loc = new locale(loc);
   init();
}

CaseMapStrings::~CaseMapStrings() {
   delete loc;  loc = NULL;
   delete utf8; utf8 = NULL;
}

string& CaseMapStrings::toUpper(const string& in, string& out) const
{
   if (utf8)
      return utf8->toUpper(in, out);
   out.resize(in.size());
   for (Uint i = 0; i < in.size(); ++i)
      out[i] = toupper(in[i], *loc);
   return out;
}

string& CaseMapStrings::toLower(const string& in, string& out) const
{
   if (utf8)
      return utf8->toLower(in, out);
   out.resize(in.size());
   for (Uint i = 0; i < in.size(); ++i)
      out[i] = tolower(in[i], *loc);
   return out;
}

string& CaseMapStrings::capitalize(const string& in, string& out) const
{
   if (utf8)
      return utf8->capitalize(in, out);
   if (&in != &out) out = in;
   if (out.size())
      out[0] = toupper(out[0], *loc);
   return out;
}

string& CaseMapStrings::decapitalize(const string& in, string& out) const
{
   if (utf8)
      return utf8->decapitalize(in, out);
   if (&in != &out) out = in;
   if (out.size())
      out[0] = tolower(out[0], *loc);
   return out;
}
