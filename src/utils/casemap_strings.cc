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

using namespace Portage;

void CaseMapStrings::init()
{
   if (loc.name().find("utf8", 0) != string::npos ||
       loc.name().find("utf-8", 0) != string::npos ||
       loc.name().find("UTF8", 0) != string::npos ||
       loc.name().find("UTF-8", 0) != string::npos) {
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
   loc(loc_name), utf8(NULL)
{
   init();
}

CaseMapStrings::CaseMapStrings(const locale& loc) :
   loc(loc), utf8(NULL)
{
   init();
}

string& CaseMapStrings::toUpper(const string& in, string& out) const
{
   if (utf8)
      return utf8->toUpper(in, out);
   out.resize(in.size());
   for (Uint i = 0; i < in.size(); ++i)
      out[i] = toupper(in[i], loc);
   return out;
}

string& CaseMapStrings::toLower(const string& in, string& out) const
{
   if (utf8)
      return utf8->toLower(in, out);
   out.resize(in.size());
   for (Uint i = 0; i < in.size(); ++i)
      out[i] = tolower(in[i], loc);
   return out;
}

string& CaseMapStrings::capitalize(const string& in, string& out) const
{
   if (utf8)
      return utf8->capitalize(in, out);
   if (&in != &out) out = in;
   if (out.size())
      out[0] = toupper(out[0], loc);
   return out;
}

string& CaseMapStrings::decapitalize(const string& in, string& out) const
{
   if (utf8)
      return utf8->decapitalize(in, out);
   if (&in != &out) out = in;
   if (out.size())
      out[0] = tolower(out[0], loc);
   return out;
}
