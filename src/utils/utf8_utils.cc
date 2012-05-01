/**
 * @author George Foster
 * @file utf8_utils.cc
 *
 * Implementation note: I noticed after this was finished that there are
 * actually functions in ICU that operate on UTF8 strings directly, which might
 * have saved a lot of trouble. (See ucasemap.h). Something to think about when
 * extending this class. 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "utf8_utils.h"
#include "errors.h"

using namespace Portage;
using namespace std;

#ifndef NOICU

#include <iostream>
#include <unicode/utypes.h>
#include <unicode/uchriter.h>

UTF8Utils::UTF8Utils()
   : u16(1)
   , output(1)
   , ecode(U_ZERO_ERROR)
{}

bool UTF8Utils::status(string* msg)
{
   if (msg)
      *msg = u_errorName(ecode);
   return U_SUCCESS(ecode);
}

bool UTF8Utils::convToU16(const string& in)
{
   ecode = U_ZERO_ERROR;
   Uint need = (in.size() + 8) * 4; // wild guess
   if (u16.size() < need)
      u16.resize(need);
   
   u_strFromUTF8(&u16[0], u16.size(), NULL, in.c_str(), -1, &ecode);

   return U_SUCCESS(ecode);
}

bool UTF8Utils::convFromU16(string& out)
{
   ecode = U_ZERO_ERROR;
   Uint need = u_strlen(&u16[0]) * 2 + 1; // wild guess
   if (output.size() < need)
      output.resize(need);
      
   u_strToUTF8(&output[0], output.size(), NULL, &u16[0], -1, &ecode);

   if (U_FAILURE(ecode))
      return false;

   out = &output[0];
   return true;
}

void UTF8Utils::truncateU16AfterFirst()
{
   Uint len = u_strlen(&u16[0]);
   UCharCharacterIterator it(&u16[0], len);
   Uint e = it.move(1, UCharCharacterIterator::kStart);
   if (e < len) u16[e] = 0;
}

// EJJ, April 2012: the following methods all used to return "in" (i.e., a
// reference to in) in case of error.  This was bad, as highlighted by
// Klocwork, because "in" is actually a local variable: a const ref argument
// might be a temporary, it is not necessarily a real variable in the caller's
// environment.  Plus, it makes the methods counterintuitive: the caller would
// typically expect they can use out after calling these methods, instead of
// using their return values.  Indeed, I have at some point found instances in
// Portage where we use out, not the return value.  So now we use 
//    { if (&out != &in) out = in; return out; }
// in case of error, which copies in to out, and makes sure all possible ways
// to use these methods work correctly and intuitively.

string& UTF8Utils::toUpper(const string& in, string& out)
{
   if (!convToU16(in))
      { if (&out != &in) out = in; return out; }

   u_strToUpper(&u16[0], u16.size(), &u16[0], -1, NULL, &ecode);
   if (U_FAILURE(ecode))
      { if (&out != &in) out = in; return out; }

   if (!convFromU16(out))
      { if (&out != &in) out = in; return out; }

   return out;
}

string& UTF8Utils::toLower(const string& in, string& out)
{
   if (!convToU16(in))
      { if (&out != &in) out = in; return out; }

   u_strToLower(&u16[0], u16.size(), &u16[0], -1, NULL, &ecode);
   if (U_FAILURE(ecode))
      { if (&out != &in) out = in; return out; }

   if (!convFromU16(out))
      { if (&out != &in) out = in; return out; }

   return out;
}

// This is sort of convoluted, but so is the library.

string& UTF8Utils::capitalize(const string& in, string& out)
{
   if (!convToU16(in))
      { if (&out != &in) out = in; return out; }

   truncateU16AfterFirst();
   string first;
   if (!convFromU16(first))
      { if (&out != &in) out = in; return out; }
   toUpper(first, first);

   if (U_FAILURE(ecode))
      { if (&out != &in) out = in; return out; }

   out = first + in.substr(first.size());

   return out;
}

string& UTF8Utils::decapitalize(const string& in, string& out)
{
   if (!convToU16(in))
      { if (&out != &in) out = in; return out; }

   truncateU16AfterFirst();
   string first;
   if (!convFromU16(first))
      { if (&out != &in) out = in; return out; }
   toLower(first, first);

   if (U_FAILURE(ecode))
      { if (&out != &in) out = in; return out; }

   out = first + in.substr(first.size());

   return out;
}

#else // if NOICU

static void ICUNotCompiled() {
   error(ETFatal,
      "Compilation with ICU was disabled at build time.\n"
      "To use casemapping on utf8 data, install ICU, edit the ICU variable in\n"
      "PORTAGEshared/src/Makefile.user-conf and recompile.");
}

UTF8Utils::UTF8Utils() {}
string& UTF8Utils::toUpper(const string& in, string& out)
{
   ICUNotCompiled();
   return out;
}

string& UTF8Utils::toLower(const string& in, string& out)
{
   ICUNotCompiled();
   return out;
}

string& UTF8Utils::capitalize(const string& in, string& out)
{
   ICUNotCompiled();
   return out;
}

string& UTF8Utils::decapitalize(const string& in, string& out)
{
   ICUNotCompiled();
   return out;
}

#endif // NOICU
