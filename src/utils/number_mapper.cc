/**
 * @author Samuel Larkin
 * @file number_mapper.cc - Replaces digits of a number into a token (@)
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include "number_mapper.h"
#include "errors.h"
#include <iostream>

using namespace Portage;
using namespace std;

NumberMapper::baseNumberMapper* NumberMapper::getMapper(const string& map_type, const char token) {
   if (map_type == "simpleNumber") {
      return new simpleNumberMapper(token);
   }
   else if (map_type == "prefixNumber") {
      return new prefixNumberMapper(token);
   }
   else
      error(ETFatal, "Unknown number mapper: %s", map_type.c_str());

   return NULL;
}


NumberMapper::baseNumberMapper::baseNumberMapper(const char token)
: token(token)
{}

NumberMapper::baseNumberMapper::~baseNumberMapper()
{}

void NumberMapper::baseNumberMapper::map(const string& in, string& out)
{
   out = in; 
}

void NumberMapper::baseNumberMapper::operator()(const string& in, string& out)
{
   map(in, out);
}


NumberMapper::simpleNumberMapper::simpleNumberMapper(const char token)
: baseNumberMapper(token)
{
}

void NumberMapper::simpleNumberMapper::map(const string& in, string& out) {
   workspace = in;
   for (Uint i(0); i<workspace.size(); ++i) {
      const char letter = workspace[i];
      if (isdigit(letter)) {
         workspace[i] = token;
      }
      else if (ispunct(letter)) {
         workspace[i] = letter;
      }
      else {
         out = in;
         return;
      }
   }

   out = workspace;
}


NumberMapper::prefixNumberMapper::prefixNumberMapper(const char token)
: baseNumberMapper(token)
{
}

void NumberMapper::prefixNumberMapper::map(const string& in, string& out)
{
   const char letter = in[0];
   if (!(isdigit(letter) || ispunct(letter))) {
      out = in;
      return;
   }

   workspace = in;
   for (Uint i(0); i<workspace.size(); ++i) {
      const char letter = workspace[i];
      if (isdigit(letter)) {
         workspace[i] = token;
      }
      else if (ispunct(letter)) {
         workspace[i] = letter;
      }
      else {
         // Once we found a non-digit we allow only non-digit
         ++i;
         for (; i<workspace.size(); ++i) {
            const char letter = workspace[i];
            if (isdigit(letter)) {
               out = in;
               return;
            }
         }
      }
   }

   out = workspace;
}
